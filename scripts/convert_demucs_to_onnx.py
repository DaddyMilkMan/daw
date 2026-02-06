import torch
import torch.nn as nn
import torch.nn.functional as F
from demucs.pretrained import get_model
from demucs.htdemucs import HTDemucs
from einops import rearrange
import onnx
import os
import sys
import argparse
from pathlib import Path

# --- Export Strategy 1: Spectrogram Core (input/output are complex spectrograms flattened to float) ---
# This strategy exports the core processing of Demucs, skipping the initial STFT
# and final iSTFT. The ONNX model will take a flattened complex spectrogram
# ([B, 4, F, T]) and output a flattened complex spectrogram ([B, Sources, 4, F, T]).
class DemucsONNXSpectrogramCore(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model
        self.model.eval()

        # Monkey-patch _spec and _ispec to be identity functions
        # This makes the model's forward method accept/return spectrograms directly
        self.model._spec = lambda x: x
        self.model._ispec = lambda x: x
    
    def forward(self, x):
        # Input x: [Batch, 4, Freq, Time]
        # x contains (L_re, L_im, R_re, R_im) interleaved for 2 channels (stereo)
        
        # Reshape to [Batch, 2 (Channels), 2 (Re/Im), Freq, Time]
        x_reshaped = x.view(x.shape[0], 2, 2, x.shape[2], x.shape[3])
        
        # Convert to complex: [Batch, 2, Freq, Time]
        z = torch.complex(x_reshaped[:, :, 0], x_reshaped[:, :, 1])
        
        # Call the patched Demucs model.forward()
        # Since _spec is patched, model.forward will treat 'z' as its spectrogram input.
        # The internal logic (magnitude, normalization, transformer) will then proceed.
        out_z_complex = self.model(z)
        
        # out_z_complex: [Batch, Sources, Channels, Freq, Time] (Complex)
        # We need to flatten the complex dimension to 4 float channels for ONNX output
        
        # Extract real and imag parts: [Batch, Sources, Channels, Freq, Time]
        out_real = out_z_complex.real
        out_imag = out_z_complex.imag
        
        # Interleave them for ONNX output: [Batch, Sources, 4, Freq, Time]
        # where dim 2 is (L_re, L_im, R_re, R_im) for each source
        
        output_list = []
        for ch in range(out_real.shape[2]): # Iterate over stereo channels (0, 1)
            output_list.append(out_real[:, :, ch]) # Real part of channel ch
            output_list.append(out_imag[:, :, ch]) # Imag part of channel ch
            
        # Stack along a new dimension to get [Batch, Sources, 4, Freq, Time]
        output_flattened_complex = torch.stack(output_list, dim=2)
        
        return output_flattened_complex

# --- Export Strategy 2: Waveform and Magnitude Spectrogram In (input/output are time domain and mag spec) ---
# This strategy exports a model that takes time-domain waveform and a magnitude
# spectrogram. It's closer to the original Demucs API.
class HTDemucsONNXWaveformMagIn(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model
        # We access attributes directly from self.model to avoid copying everything
        
    def forward(self, mix_waveform, mix_mag_spec):
        # Replicating HTDemucs.forward logic, skipping _spec/_magnitude and _ispec
        
        x = mix_mag_spec # Magnitude spectrogram input
        B, C, Fq, T = x.shape

        # Normalization (Magnitude path)
        mean = x.mean(dim=(1, 2, 3), keepdim=True)
        std = x.std(dim=(1, 2, 3), keepdim=True)
        x = (x - mean) / (1e-5 + std)

        # Time branch input (Waveform path)
        xt = mix_waveform
        meant = xt.mean(dim=(1, 2), keepdim=True)
        stdt = xt.std(dim=(1, 2), keepdim=True)
        xt = (xt - meant) / (1e-5 + stdt)

        saved = []
        saved_t = []
        lengths = []
        lengths_t = []

        for idx, encode in enumerate(self.model.encoder):
            lengths.append(x.shape[-1])
            inject = None
            if idx < len(self.model.tencoder):
                lengths_t.append(xt.shape[-1])
                tenc = self.model.tencoder[idx]
                xt = tenc(xt)
                if not tenc.empty:
                    saved_t.append(xt)
                else:
                    inject = xt
            x = encode(x, inject)
            if idx == 0 and self.model.freq_emb is not None:
                frs = torch.arange(x.shape[-2], device=x.device)
                emb = self.model.freq_emb(frs).t()[None, :, :, None].expand_as(x)
                x = x + self.model.freq_emb_scale * emb
            saved.append(x)

        if self.model.crosstransformer:
            if self.model.bottom_channels:
                b, c, f, t = x.shape
                x = rearrange(x, "b c f t-> b c (f t)")
                x = self.model.channel_upsampler(x)
                x = rearrange(x, "b c (f t)-> b c f t", f=f)
                xt = self.model.channel_upsampler_t(xt)

            x, xt = self.model.crosstransformer(x, xt)

            if self.model.bottom_channels:
                x = rearrange(x, "b c f t-> b c (f t)")
                x = self.model.channel_downsampler(x)
                x = rearrange(x, "b c f t-> b c f t", f=f)
                xt = self.model.channel_downsampler_t(xt)

        for idx, decode in enumerate(self.model.decoder):
            skip = saved.pop(-1)
            x, pre = decode(x, skip, lengths.pop(-1))
            
            offset = self.model.depth - len(self.model.tdecoder)
            if idx >= offset:
                tdec = self.model.tdecoder[idx - offset]
                length_t = lengths_t.pop(-1)
                if tdec.empty:
                    pre = pre[:, :, 0]
                    xt, _ = tdec(pre, None, length_t)
                else:
                    skip = saved_t.pop(-1)
                    xt, _ = tdec(xt, skip, length_t)

        S = len(self.model.sources)
        x = x.view(B, S, -1, Fq, T)
        x = x * std[:, None] + mean[:, None]
        
        # Return the raw output (CaC spectrogram)
        return x

def export_model(output_path: Path, model_name: str, export_type: str):
    print(f"Loading {model_name} model...")
    model = get_model(model_name)
    model.cpu()
    model.eval()

    if export_type == "spectrogram_core":
        wrapper = DemucsONNXSpectrogramCore(model)
        
        # Dummy Input for Spectrogram Core: [Batch, 4 (L_re, L_im, R_re, R_im), Freq, Time]
        # Freq = 4096/2 + 1 = 2049 (for default 4096 n_fft)
        # Time = arbitrary
        dummy_input = torch.randn(1, 4, 2049, 100)
        input_names = ['mix_spectrogram']
        output_names = ['stems_spectrogram']
        dynamic_axes = {
            'mix_spectrogram': {3: 'time'},
            'stems_spectrogram': {4: 'time'}
        }
        
    elif export_type == "waveform_mag_in":
        if not isinstance(model, HTDemucs):
            raise TypeError(f"Model {model_name} is not HTDemucs, cannot use 'waveform_mag_in' export type.")
        
        wrapper = HTDemucsONNXWaveformMagIn(model)
        
        # Dummy Inputs for Waveform/Magnitude In:
        # mix_waveform: [Batch, 2 (Stereo), Length (Time Domain)]
        # mix_mag_spec: [Batch, 2 (Stereo), Freq, Time]
        
        dummy_waveform = torch.randn(1, 2, 44100 * 10) # 10 seconds of stereo audio
        
        # Pad waveform as per model requirements (ensure it's a multiple of segment length)
        # The segment length is model.segment * model.samplerate
        training_length = int(model.segment * model.samplerate)
        current_length = dummy_waveform.shape[-1]
        
        if current_length < training_length:
            dummy_waveform = F.pad(dummy_waveform, (0, training_length - current_length))
        
        # Compute Mag Spec from a dummy complex spectrogram (this is just for ONNX graph tracing)
        # In actual usage, this 'mag' input would come from an external STFT.
        # It's challenging to get dummy_mag from dummy_waveform without _spec
        # So we will create it directly.
        
        # Approximate Freq and Time dims for dummy_mag
        # Assume default n_fft=4096, hop_length=1024.
        # Freq dim = n_fft / 2 + 1 = 2049
        # Time dim = ceil(length / hop_length) = ceil((44100*10) / 1024) = 431.x -> 432
        
        # If we need accurate `mag` dim, we need to replicate _spec logic or use a precomputed one.
        # Let's use a dummy that matches expected shape.
        n_fft = model.n_fft if hasattr(model, 'n_fft') else 4096
        hop_length = model.hop_length if hasattr(model, 'hop_length') else 1024
        
        freq_dim = n_fft // 2 + 1
        time_dim = (current_length + hop_length - 1) // hop_length # ceil division
        
        dummy_mag_spec = torch.randn(1, 2, freq_dim, time_dim)
        
        dummy_input = (dummy_waveform, dummy_mag_spec)
        input_names = ['mix_waveform', 'mix_mag_spec']
        output_names = ['stems_mag_spec']
        dynamic_axes = {
            'mix_waveform': {2: 'time_waveform'},
            'mix_mag_spec': {3: 'time_spectrogram'},
            'stems_mag_spec': {4: 'time_spectrogram'}
        }
    else:
        raise ValueError(f"Unknown export type: {export_type}")

    print(f"Exporting to {output_path}...")
    
    torch.onnx.export(
        wrapper,
        dummy_input,
        output_path,
        input_names=input_names,
        output_names=output_names,
        dynamic_axes=dynamic_axes,
        opset_version=14 # Opset 14 is a good balance for modern ops
    )
    print("Export complete.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert Demucs PyTorch models to ONNX')
    parser.add_argument("output_dir", type=str, help="Destination directory for the converted ONNX model")
    parser.add_argument("--model", type=str, default="htdemucs", 
                        help="Demucs model name to export (e.g., htdemucs, mdx_extra_q)")
    parser.add_argument("--export_type", type=str, default="spectrogram_core",
                        choices=["spectrogram_core", "waveform_mag_in"],
                        help="Type of ONNX export strategy. 'spectrogram_core' takes complex spectrograms. 'waveform_mag_in' takes time-domain waveform and magnitude spectrogram.")
    
    args = parser.parse_args()
    
    output_dir_path = Path(args.output_dir)
    output_dir_path.mkdir(parents=True, exist_ok=True)
    
    model_filename = f"{args.model}_{args.export_type}.onnx"
    output_onnx_path = output_dir_path / model_filename
    
    try:
        export_model(output_onnx_path, args.model, args.export_type)
        print(f"Successfully exported {args.model} with {args.export_type} strategy to {output_onnx_path}")
    except Exception as e:
        print(f"Error exporting model: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)