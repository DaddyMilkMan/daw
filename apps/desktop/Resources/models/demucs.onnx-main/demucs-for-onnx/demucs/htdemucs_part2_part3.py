            # which is used when the freq. and time branch separate.
            offset = self.depth - len(self.tdecoder)
            if idx >= offset:
                tdec = self.tdecoder[idx - offset]
                length_t = lengths_t.pop(-1)
                if tdec.empty:
                    assert pre.shape[2] == 1, pre.shape
                    pre = pre[:, :, 0]
                    xt, _ = tdec(pre, None, length_t)
                else:
                    skip = saved_t.pop(-1)
                    xt, _ = tdec(xt, skip, length_t)
        # Let's make sure we used all stored skip connections.
        assert len(saved) == 0
        assert len(lengths_t) == 0
        assert len(saved_t) == 0
        S = len(self.sources)
        x = x.view(B, S, -1, Fq, T)
        x = x * std[:, None] + mean[:, None]
        if self.use_train_segment:
            if self.training:
                xt = xt.view(B, S, -1, length)
            else:
                xt = xt.view(B, S, -1, training_length)
        else:
            xt = xt.view(B, S, -1, length)
        xt = xt * stdt[:, None] + meant[:, None]
        # again, skipping the istft step for outside of the network
        return x, xt
        #zout = self._mask(z, x)
        #if self.use_train_segment:
        #    if self.training:
        #        x = self._ispec(zout, length)
        #    else:
        #        x = self._ispec(zout, training_length)
        #else:
        #    x = self._ispec(zout, length)
        #x = xt + x
        #if length_pre_pad:
        #    x = x[..., :length_pre_pad]
        #return x
