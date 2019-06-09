import numpy 
import math
import cv2


#
import argparse
import numpy as np
import timeit

t0 = timeit.default_timer()

# Accept parameters from cmdline via `argparse`
parser = argparse.ArgumentParser(description='PSNR calculation')
parser.add_argument('-o', metavar='original', type=str, required=True, help='original, full-scale image')
parser.add_argument('-m', metavar='processed image', type=str, required=True, help='modified image of the same resolution')

args = parser.parse_args()

original = args.o
modified = args.m

original = cv2.imread(original)
contrast = cv2.imread(modified,1)
def psnr(img1, img2):
    mse = numpy.mean( (img1 - img2) ** 2 )
    if mse == 0:
        return 100
    PIXEL_MAX = 255.0

    return 20 * math.log10(PIXEL_MAX / math.sqrt(mse))

d=psnr(original,contrast)
print(d)
