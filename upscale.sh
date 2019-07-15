#!/bin/bash
# arg 1 points to the model you want to use to perform upscaling

if [ ! -f "$1" ]
then
    echo "--- Aborting! --- Model file not found."
    exit 1
fi

if command -v python3 &>/dev/null; then
    echo "--- Python 3 dependency installed"
else
    echo "--- Aborting! --- Python 3 is not installed"
    exit 1
fi

bash ./resize.sh
if [ $? -ne 0 ]; then
    echo "--- Aborting! --- Error with resizing images"
    exit 1
fi

echo "--- Cleaning output folder"
rm -f output/*

echo "--- Performing super resolution"
for f in target/*.png; do
    fullname=$(echo "$(basename "$f")" | cut -f 1 -d '.')
    basename=$(echo "$fullname" | cut -c 5-)
    python3 super_resolve.py --input_image target/$fullname.png --model $1 --output_filename output/ups_$basename.png
    if [ $? -ne 0 ]; then
        echo "--- Aborting! --- Error with performing super resolution"
    exit 1
fi
done

echo "--- Successfully finished super resolution"