# Super Resolution Engine

Real-time super-resolution using a C++ inference engine (LibTorch + OpenCV) paired with a PyTorch training pipeline.

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| [LibTorch](https://pytorch.org/get-started/locally/) | **2.x** (tested 2.14.1) | C++ distribution; requires a C++20 compiler |
| [OpenCV](https://opencv.org/releases/) | **4.x** (tested 4.6 on Linux, 4.14 on Windows) | Use 4.13+ with Visual Studio 2026 |
| CMake | 3.21+ | |
| Compiler | MSVC 2022 / 2026, GCC 11+, Clang 14+ | |
| CUDA | optional | Use a CUDA build of LibTorch; the engine picks CUDA automatically when available |
| Python | 3.x | Training scripts and TorchScript export |

Extract `libtorch/` and `opencv/` into the project root (they are git-ignored), or point
`CMAKE_PREFIX_PATH` at them.

## Build: Windows (MSVC)

1. Download **LibTorch → Windows → C++ → Release** from https://pytorch.org/get-started/locally/
   (e.g. `libtorch-win-shared-with-deps-2.14.1+cpu.zip`) and extract it so you have `libtorch\share\cmake\Torch`.
2. Download the OpenCV Windows release (`opencv-4.14.0-windows.exe`), run it, and extract into the project
   root so you have `opencv\build\OpenCVConfig.cmake`.
3. Either open the folder in Visual Studio (it picks up `CMakePresets.json`; choose **Windows x64 Release**), or
   from a *Developer PowerShell for VS*:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
build\Release\super_resolve.exe model.pt input\Witcher3_demo-240.png
```

The LibTorch and OpenCV DLLs are copied next to the `.exe` automatically.

**Release only.** The standard LibTorch download links the release CRT, so a Debug build crashes at runtime.
CMake therefore only offers Release/RelWithDebInfo. For a true Debug build, download the separate
*debug* LibTorch package and configure with `-DSSRE_LIBTORCH_DEBUG=ON`.

For the Unity shared-memory mode, add `-DSSRE_ENGINE=ON` (or use the `windows-engine` preset).

## Build: Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$(python3 -c 'import torch;print(torch.utils.cmake_prefix_path)')"
cmake --build build -j
./build/super_resolve model.pt input/Witcher3_demo-240.png [cpu|cuda]
```

(A pip-installed `torch` works as the LibTorch here; system OpenCV via `libopencv-dev`.)

## Model files

`super_resolve` loads a **TorchScript** file, not a `.pth`. Export one with the same PyTorch version as your LibTorch:

```bash
python util/to_torchscript.py --model models/model_epoch_30.pth --output model.pt
```

The checked-in `util/traced-script-module.pt` is in the pre-1.4 "legacy" TorchScript format and cannot be loaded
by any current LibTorch; re-export it as above. The engine is hard-wired for 2x models (`models/model_epoch_*.pth`,
`archive/witcher3_2x`); the `borderlands2` and `BSDS300` checkpoints are 3x.

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