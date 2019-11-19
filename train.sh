#!/bin/bash

if command -v python3 &>/dev/null; then
    echo "--- Python 3 dependency installed"
else
    echo "--- Aborting! --- Python 3 is not installed"
    exit 1
fi

echo "--- Running default training regime"
# Remove --cuda if not working with a gpu
python3 main.py --upscale_factor 2 --cuda --batchSize 4 --testBatchSize 100 --nEpochs 10 --lr 0.001 
if [ $? -ne 0 ]; then
    echo "--- Aborting! --- Error with training images"
    exit 1
fi
