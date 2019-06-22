#!/bin/bash
# arg 1 is the directory of the training video file, arg 2 is the directory of the testing video file

if command -v python3 &>/dev/null; then
    echo "--- Python 3 dependency installed"
else
    echo "--- Aborting! --- Python 3 is not installed"
    exit 1
fi

echo "--- Creating training images from $1"
bash ./dataset/video_slice.sh $1
if [ $? -ne 0 ]; then
    echo "--- Aborting! --- Error with creating training images"
    exit 1
fi

echo "--- Creating testing images from $2"
bash ./dataset/video_slice.sh $2
if [ $? -ne 0 ]; then
    echo "--- Aborting! --- Error with creating testing images"
    exit 1
fi

echo "--- Running default training regime"
python3 main.py --upscale_factor 2 --batchSize 4 --testBatchSize 100 --nEpochs 30 --lr 0.001
if [ $? -ne 0 ]; then
    echo "--- Aborting! --- Error with creating testing images"
    exit 1
fi