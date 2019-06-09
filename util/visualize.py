import torch
import torch.nn as nn
import torch.optim as optim
import torch.utils.data as data
import torchvision.models as models
import torchvision.datasets as dset
import torchvision.transforms as transforms
from torch.autograd import Variable
from torchvision.models.vgg import model_urls
from torchviz import make_dot

import argparse
parser = argparse.ArgumentParser(description='PyTorch Super Res Example')
parser.add_argument('--model', type=str, required=True, help='model file to use')
opt = parser.parse_args()

print(opt)

model = torch.load(opt.model)

x = torch.zeros(4, 1, 224, 224, dtype=torch.float, requires_grad=False)
out = model(x)
dot = make_dot(out)  # plot graph of variable, not of a nn.Module
dot.save('a')
dot.view()
