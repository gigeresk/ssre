import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import sarrmodel
from sarrmodel import Net

import torch

'''
Convert a Python-trained model (.pth) into TorchScript, loadable from C++ with torch::jit::load.
See https://pytorch.org/tutorials/advanced/cpp_export.html

TorchScript files are tied loosely to the LibTorch version: re-export with the same (or older)
PyTorch version as the LibTorch you build super_resolve against. Files exported with PyTorch 1.3
or earlier ("legacy format") cannot be loaded by any current LibTorch.

Example usage
python3 util/to_torchscript.py --model models/model_epoch_30.pth --output traced-script-module.pt
'''

# The checkpoints in models/ are whole pickled Net objects that were saved when the
# class lived in a module named "model"; alias it so torch.load can find the class.
sys.modules.setdefault('model', sarrmodel)


def load_net(path, device):
    checkpoint = torch.load(path, map_location=device, weights_only=False)
    if isinstance(checkpoint, torch.nn.Module):
        state_dict = checkpoint.state_dict()
    elif isinstance(checkpoint, dict) and 'model_state_dict' in checkpoint:
        state_dict = checkpoint['model_state_dict']
    else:
        state_dict = checkpoint

    # conv4 has upscale_factor**2 output channels.
    upscale_factor = int(round(state_dict['conv4.weight'].shape[0] ** 0.5))
    net = Net(upscale_factor=upscale_factor).to(device)
    net.load_state_dict(state_dict)
    net.eval()
    return net, upscale_factor


def to_torchscript():
    parser = argparse.ArgumentParser(description='Export a super-resolution model to TorchScript')
    parser.add_argument('--model', type=str, required=True, help='model file (.pth) to convert')
    parser.add_argument('--output', type=str, default='traced-script-module.pt', help='TorchScript output file')
    parser.add_argument('--input_image', type=str, default=None,
                        help='optional image to trace with (the network is fully convolutional, so any size works)')
    parser.add_argument('--cuda', action='store_true', help='trace on CUDA (the saved module can still be moved to CPU)')
    opt = parser.parse_args()

    device = torch.device('cuda' if opt.cuda else 'cpu')
    net, upscale_factor = load_net(opt.model, device)

    if opt.input_image:
        from PIL import Image
        import numpy as np
        y = Image.open(opt.input_image).convert('YCbCr').split()[0]
        example = torch.from_numpy(np.asarray(y, dtype=np.float32) / 255.0)[None, None].to(device)
    else:
        example = torch.rand(1, 1, 64, 64, device=device)

    with torch.no_grad():
        traced = torch.jit.trace(net, example)
    traced.save(opt.output)
    print('Wrote {0}x TorchScript model {1} (PyTorch {2})'.format(upscale_factor, opt.output, torch.__version__))


if __name__ == '__main__':
    to_torchscript()
