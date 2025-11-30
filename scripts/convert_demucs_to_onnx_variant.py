#!/usr/bin/env python

import sys
import torch
from torch import nn
from torch.nn import functional as F
import argparse
from pathlib import Path
from demucs.pretrained import get_model
from demucs.htdemucs import HTDemucs
from einops import rearrange

class HTDemucsONNX(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model
        # We access attributes directly from self.model to avoid copying everything
        
    def forward(self, mix, mag):
        # Replicating HTDemucs.forward logic, skipping _spec/_magnitude and _ispec
        
        x = mag
        B, C, Fq, T = x.shape

        # Normalization
        mean = x.mean(dim=(1, 2, 3), keepdim=True)
        std = x.std(dim=(1, 2, 3), keepdim=True)
        x = (x - mean) / (1e-5 + std)

        # Time branch input
        xt = mix
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
                x = rearrange(x, "b c (f t)-> b c f t", f=f)
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

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert Demucs PyTorch models to ONNX')
    parser.add_argument("dest_dir", type=str, help="destination path for the converted model")
    
    args = parser.parse_args()
    dir_out = Path(args.dest_dir)
    dir_out.mkdir(parents=True, exist_ok=True)

    print("Loading htdemucs...")
    model = get_model("htdemucs")
    
    if isinstance(model, HTDemucs):
        core_model = model
    elif hasattr(model, 'models') and isinstance(model.models[0], HTDemucs):
        core_model = model.models[0]
    else:
        raise TypeError("Unsupported model type")

    core_model.eval()
    
    # Wrap it
    onnx_model = HTDemucsONNX(core_model)
    onnx_model.eval()

    # Dummy Inputs
    # Waveform: [1, 2, Length]
    dummy_waveform = torch.randn(1, 2, 343980)
    
    # Pad waveform as per model requirements
    training_length = int(core_model.segment * core_model.samplerate)
    dummy_waveform = F.pad(dummy_waveform, (0, training_length - dummy_waveform.shape[-1]))
    
    # Compute Mag Spec (Input 2)
    z = core_model._spec(dummy_waveform)
    dummy_mag = core_model._magnitude(z)
    
    dummy_input = (dummy_waveform, dummy_mag)
    
    onnx_file_path = dir_out / "htdemucs.onnx"
    
    print(f"Exporting to {onnx_file_path}...")
    
    torch.onnx.export(
        onnx_model,
        dummy_input,
        onnx_file_path,
        export_params=True,
        opset_version=14, # 14 is safe for complex
        do_constant_folding=True,
        input_names=['mix', 'mag'],
        output_names=['stems'],
        dynamic_axes={
            'mix': {2: 'time'},
            'mag': {3: 'time'},
            'stems': {4: 'time'}
        }
    )
    print("Success!")
