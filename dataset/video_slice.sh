#!/bin/bash
# arg 1 is the directory of the video file

if [ ! -f "$1" ]
then
    echo "Video file not found."
    exit 1
fi

BASEDIR=$(dirname "$1")
BASENAME=$(basename "$1")

echo --- Cleaning
rm -f $BASEDIR/*.png
rm -f $BASEDIR/*.jpg
rm -f $BASEDIR/*.jpeg

filename=$(echo "$BASENAME" | cut -f 1 -d '.')

echo --- Running Frame Slicing
eval "ffmpeg -hide_banner -loglevel info -i $1 -r 30 $BASEDIR/$filename-%03d.png"
if [ $? -ne 0 ]; then
    echo "--- Video slicing failed"
    exit 1
fi

echo --- Successfully converted video file to images