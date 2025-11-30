import torch
import torch.nn as nn
from demucs.pretrained import get_model
import onnx
import os
import sys

# Wrapper to expose the core model (skipping STFT/iSTFT)
class DemucsONNXCore(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model
        # Ensure model is in eval mode
        self.model.eval()

    def forward(self, x):
        # x shape: [Batch, 4, Freq, Time]
        # This represents 2 channels (Stereo) * 2 (Real/Imag)
        # We need to convert this to the complex tensor Demucs expects.
        
        # 1. View as [Batch, 2, 2, Freq, Time] (Channels, Real/Imag, F, T)
        # Note: The C++ code packs it as:
        # Channel 0 Real, Channel 0 Imag, Channel 1 Real, Channel 1 Imag?
        # OR: Channel 0 Real, Channel 1 Real, Channel 0 Imag, Channel 1 Imag?
        #
        # In ONNXStemSeparator.cpp computeSTFT:
        # It fills outputTensor.
        # Channel Real Plane: offset = (ch * 2) * size
        # Channel Imag Plane: offset = (ch * 2 + 1) * size
        # So:
        # Index 0: Ch0 Real
        # Index 1: Ch0 Imag
        # Index 2: Ch1 Real
        # Index 3: Ch1 Imag
        #
        # So x is [Batch, 4, F, T] where dim 1 is (L_re, L_im, R_re, R_im).
        
        # Reshape to [Batch, 2, 2, F, T] -> [Batch, Channels, Complex, F, T]
        x_reshaped = x.view(x.shape[0], 2, 2, x.shape[2], x.shape[3])
        
        # Convert to complex: [Batch, 2, F, T]
        # complex(real, imag)
        z = torch.complex(x_reshaped[:, :, 0], x_reshaped[:, :, 1])
        
        # 2. Run Demucs Core
        # We need to mimic HTDemucs.forward but skip _spec and _ispec
        
        # From HTDemucs source (approximate):
        # mag = self._magnitude(z)
        # x = mag
        # ...
        
        # We can't easily call 'forward' because it calls _spec.
        # We need to call the internal methods.
        
        # HTDemucs logic:
        # 1. Magnitude
        mag = self.model._magnitude(z)
        
        # 2. Apply scaling
        # In v4, scaling is handled inside? 
        # Actually, let's look at how 'segment' works or 'forward'.
        # self.cac (Complex As Channels) is usually True for v4? No, v4 uses complex tensors.
        
        # Let's try to rely on the fact that we can monkey-patch _spec to be identity?
        # No, _spec takes time domain.
        
        # We will replicate the forward pass logic of HTDemucs here.
        # This is risky if versions change, but better than broken imports.
        
        x = mag
        
        # Mean/Std normalization
        mean = x.mean(dim=(1, 2, 3), keepdim=True)
        std = x.std(dim=(1, 2, 3), keepdim=True)
        x = (x - mean) / (1e-5 + std)
        
        # Forward through encoder/decoder
        # HTDemucs has 'crosstransformer'
        # It calls self.crosstransformer(x) ? No.
        
        # It seems HTDemucs is complex. 
        # Let's try to use the 'apply_model' or similar if available?
        # No.
        
        # Let's try a different approach:
        # Monkey patch _spec to return the input, and _ispec to return the output.
        # Then call model.forward().
        
        return self._forward_impl(z, mean, std)

    def _forward_impl(self, z, mean, std):
        # This is hard to replicate perfectly without source.
        # Let's use the monkey-patch strategy which is safer.
        pass

def export_model(output_path):
    print("Loading htdemucs model...")
    model = get_model('htdemucs')
    model.cpu()
    model.eval()
    
    # Monkey Patching Strategy
    # We want model(x_complex) to run the core and return y_complex
    # Original: forward(mix) -> _spec(mix) -> z -> ... -> out_z -> _ispec(out_z) -> out
    
    # We will replace _spec with Identity (or close to it)
    # And _ispec with Identity.
    
    # But _spec expects waveform, we are passing complex tensor.
    # So we define:
    
    # We will replace _spec with Identity (or close to it)
    # And _ispec with Identity.
    
    model._spec = lambda x: x # Pass through complex input
    model._ispec = lambda x: x # Pass through complex output
    
    # Now we wrap it to handle the Float->Complex conversion for ONNX
    wrapper = DemucsONNXCore(model)
    # We override the _forward_impl to actually call model.forward
    # But wait, model.forward does normalization on 'mag'.
    # If we pass 'z' (complex) to forward, it calculates mag(z).
    # This is exactly what we want!
    
    # So:
    # 1. Wrapper takes Float [B, 4, F, T]
    # 2. Converts to Complex [B, 2, F, T]
    # 3. Calls model.forward(complex_z)
    # 4. model.forward calls _spec(complex_z) -> returns complex_z (patched)
    # 5. model.forward computes mag, norm, runs transformer...
    # 6. model.forward gets out_z
    # 7. model.forward calls _ispec(out_z) -> returns out_z (patched)
    # 8. Wrapper gets out_z (Complex [B, Sources, 2, F, T])
    # 9. Wrapper converts to Float [B, Sources, 4, F, T]
    
    def forward_patched(self, x):
        # x: [B, 4, F, T]
        x_reshaped = x.view(x.shape[0], 2, 2, x.shape[2], x.shape[3])
        z = torch.complex(x_reshaped[:, :, 0], x_reshaped[:, :, 1])
        
        # Call patched model
        # Note: HTDemucs.forward usually expects 'mix' (Time domain).
        # But since we patched _spec to be identity, 'z' is passed through.
        # The internal logic: z = self._spec(mix) -> z is now our complex input.
        # Then mag = self._magnitude(z) -> Works on complex.
        out_z = self.model(z)
        
        # out_z is [B, Sources, 2, F, T] (Complex)
        # We need to flatten the complex dim to 4 channels for ONNX output
        # [B, Sources, 2, F, T] -> Real/Imag
        
        out_real = out_z.real
        out_imag = out_z.imag
        
        # Stack: [B, Sources, 2 (Re/Im), 2 (Ch), F, T] ?
        # Wait, out_z shape is [B, Sources, Channels, F, T]
        # Sources=4, Channels=2 (Stereo).
        
        # We want [B, Sources, 4 (L_r, L_i, R_r, R_i), F, T]
        # out_real: [B, S, 2, F, T]
        # out_imag: [B, S, 2, F, T]
        
        # We want to interleave them?
        # C++ code expects:
        # Ch0 Real, Ch0 Imag, Ch1 Real, Ch1 Imag
        
        # Stack along new dim
        # [B, S, 2, F, T]
        # Let's stack [real[:,:,0], imag[:,:,0], real[:,:,1], imag[:,:,1]]
        
        out_list = []
        for ch in range(2):
            out_list.append(out_real[:, :, ch])
            out_list.append(out_imag[:, :, ch])
            
        # Stack dim 2
        out = torch.stack(out_list, dim=2) # [B, S, 4, F, T]
        
        return out

    # Bind the method to the instance
    import types
    wrapper.forward = types.MethodType(forward_patched, wrapper)
    
    # Dummy Input
    # [1, 4, 2049, 336] (Approx 10s at 44.1k with hop 1024?)
    # Freq = 4096/2 + 1 = 2049
    # Time = Arbitrary
    dummy_input = torch.randn(1, 4, 2049, 100)
    
    print(f"Exporting to {output_path}...")
    
    torch.onnx.export(
        wrapper,
        dummy_input,
        output_path,
        input_names=['mix'],
        output_names=['stems'],
        dynamic_axes={
            'mix': {3: 'time'},
            'stems': {4: 'time'} # [B, S, 4, F, T] -> dim 4 is time
        },
        opset_version=14 # Need decent opset for complex support if used internally, but we output float
    )
    print("Export complete.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert.py <output_dir>")
        sys.exit(1)
        
    output_dir = sys.argv[1]
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "htdemucs.onnx")
    
    try:
        export_model(output_path)
    except Exception as e:
        print(f"Error exporting model: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

