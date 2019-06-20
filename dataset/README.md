## mp4 -> individual frames (ffmpeg)
To turns 30fps video into a directory full of PNGs (one for every frame), follow this example:
`mkdir borderlands-png-full`
`ffmpeg -i 'Borderlands 2 June 9 qHD 30fps.mp4' -r 30 borderlands-png-full/$filename%03d.png`

# frames -> half resolution frames (imagemagick)
from borderlands-png-full and with imagemagick installed
`mkdir ../borderlands-png-half`
`mogrify -resize 50% -path ../borderlands-png-half/ *.png`

#TODO
split this (these frames) into test and train in an automated fashion
train=full res but to train on
test = full res but to test on	

do we need iids_test.txt, iids_train.txt ? probably...
For the BSDS300, these contain a \n-separated list of image file prefixes
