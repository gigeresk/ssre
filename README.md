# Super Resolution Engine

Real-time super-resolution using a C++ inference engine (LibTorch + OpenCV) paired with a PyTorch training pipeline.

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| [LibTorch](https://pytorch.org/get-started/locally/) | **1.4.0** | C++ distribution — download the cxx11 ABI build |
| [OpenCV](https://opencv.org/releases/) | **4.2.0** | Build from source or use a pre-built release |
| CUDA | 10.x | Optional — CPU-only build works without it |
| Python | 3.x | Required for training scripts |

Download LibTorch 1.4.0: https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.4.0%2Bcpu.zip
(swap `cpu` for `cu101` if building with CUDA 10.1)

Extract both `libtorch/` and `opencv/` into the project root — they are excluded from git due to size.

There are sub-READMEs in `util/`, `dataset/`

## Build Notes (C++ engine)

```bash
mkdir build && cd build
cmake ..
make
```

By default CMakeLists.txt looks for `libtorch/` and `opencv/` relative to the project root.
To use system-installed versions, pass their paths explicitly:

```bash
cmake .. -DCMAKE_PREFIX_PATH="/path/to/libtorch;/path/to/opencv/build"
```

Run the built binary:

```bash
./build/super_resolve <args>
```




Below is the README for the pytorch example code (python)
# Superresolution using an efficient sub-pixel convolutional neural network

This example illustrates how to use the efficient sub-pixel convolution layer described in  ["Real-Time Single Image and Video Super-Resolution Using an Efficient Sub-Pixel Convolutional Neural Network" - Shi et al.](https://arxiv.org/abs/1609.05158) for increasing spatial resolution within your network for tasks such as superresolution.

```
usage: main.py [-h] --upscale_factor UPSCALE_FACTOR [--batchSize BATCHSIZE]
               [--testBatchSize TESTBATCHSIZE] [--nEpochs NEPOCHS] [--lr LR]
               [--cuda] [--threads THREADS] [--seed SEED]

PyTorch Super Res Example

optional arguments:
  -h, --help            show this help message and exit
  --upscale_factor      super resolution upscale factor
  --batchSize           training batch size
  --testBatchSize       testing batch size
  --nEpochs             number of epochs to train for
  --lr                  Learning Rate. Default=0.01
  --cuda                use cuda
  --threads             number of threads for data loader to use Default=4
  --seed                random seed to use. Default=123
```
This example trains a super-resolution network on the [BSD300 dataset](https://www2.eecs.berkeley.edu/Research/Projects/CS/vision/bsds/), using crops from the 200 training images, and evaluating on crops of the 100 test images. A snapshot of the model after every epoch with filename model_epoch_<epoch_number>.pth

## Example Usage:

### Train

`python main.py --upscale_factor 3 --batchSize 4 --testBatchSize 100 --nEpochs 30 --lr 0.001`

### Super Resolve
`python super_resolve.py --input_image dataset/BSDS300/images/test/16077.jpg --model model_epoch_500.pth --output_filename out.png`


### Custom Scripts

#### Producing Training Images

To produce training images for the CNN, simply place a training video in the `dataset/training` directory
and a testing video in the `dataset/testing` directory, then run the following:

`./dataset/video_slice.sh path_to_training_video path_to_testing_video`
`./dataset/video_slice.sh path_to_training_video path_to_testing_video`

#### Begin Training

To begin the training process using default parameters, first ensure you have training images in the `dataset/training`
and `dataset/testing` directories. Then, run the following:

`./train.sh`

#### Super Resolve

To super resolve using the CNN, first ensure you have a desired model you can use, then place your target images in the
`/input` directory and run the following:

`./upscale.sh path_to_desired_model_file`