/*
  Super-resolution inference engine (LibTorch + OpenCV).

  Usage: super_resolve <path-to-torchscript-module> <path-to-image> [cpu|cuda]

  MSVC: build in Release unless you are linking against the *debug* LibTorch
  package; mixing the two CRTs crashes at runtime.
*/

#include <torch/script.h>
#include <torch/cuda.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#ifdef ENGINE
#include "ClientCommunicator.h"
#endif
#endif // _WIN32

#ifndef SSRE_VERBOSE
#define SSRE_VERBOSE 1
#endif

#if SSRE_VERBOSE
#define LOG_MSG(...) std::printf(__VA_ARGS__)
#else
#define LOG_MSG(...)
#endif

/* TODO
  -want to have out_img_y be on heap or on stack
  -intelligently use CV_32F and CV_8U
  bookmarks
  -https://github.com/pytorch/pytorch/issues/12506
 */

using clk = std::chrono::high_resolution_clock;

static clk::time_point initial_time;
static clk::time_point load_time;
static clk::time_point upscaling_time;
static clk::time_point final_time;

static clk::time_point input_ready_time;
static clk::time_point output_ready_time;

static std::vector<cv::Mat> ycrcb_channels(3);

static int loadImage(const std::string& image_path, std::vector<uchar>* buf)
{
    cv::Mat bgr_image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (bgr_image.empty()) {
        std::cerr << "Could not read image " << image_path << std::endl;
        return 1;
    }
    try {
        cv::imencode(".jpg", bgr_image, *buf);
    }
    catch (...) {
        std::cerr << "Could not encode image into memory" << std::endl;
        return 1;
    }
    return 0;
}

static torch::jit::Module loadModule(const char* const moduleName, const torch::Device& device)
{
    // torch::jit::load returns the module by value (it has been a value type since LibTorch 1.2).
    torch::jit::Module module = torch::jit::load(moduleName, device);
    module.eval();
    std::cout << "ok, loaded module on " << device << "\n";
    return module;
}

static void super_resolve(cv::Mat& ycbcr_image, torch::jit::Module& module,
                          const torch::Device& device, cv::Mat& final_result)
{
    cv::split(ycbcr_image, ycrcb_channels);
    cv::Mat Y  = ycrcb_channels[0];
    cv::Mat Cb = ycrcb_channels[1];
    cv::Mat Cr = ycrcb_channels[2];

    // Convert the Y channel to a float tensor (from_blob does not copy; .to() does).
    at::Tensor tensor_image = torch::from_blob(Y.data, { 1, 1, Y.rows, Y.cols }, at::kByte);
    tensor_image = tensor_image.to(at::kFloat);

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor_image.to(device));

    load_time = clk::now();

    torch::NoGradGuard no_grad;
    c10::IValue output_ivalue = module.forward(inputs);

    const cv::Size out_size(Y.cols * 2, Y.rows * 2);

    cv::Mat out_img_cb;
    cv::resize(Cb, out_img_cb, out_size, 0, 0, cv::INTER_CUBIC);

    cv::Mat out_img_cr;
    cv::resize(Cr, out_img_cr, out_size, 0, 0, cv::INTER_CUBIC);

    at::Tensor output = output_ivalue.toTensor().to(torch::kCPU);

    upscaling_time = clk::now();

    // [1, 1, 2H, 2W] -> [2H, 2W], contiguous float32 so it can be memcpy'd.
    at::Tensor out_y = output[0][0].detach().to(at::kFloat).contiguous();
    if (out_y.size(0) != out_size.height || out_y.size(1) != out_size.width) {
        throw std::runtime_error("model output size does not match a 2x upscale of the input");
    }

    cv::Mat out_img_y(out_size, CV_32F);
    std::memcpy(out_img_y.data, out_y.data_ptr<float>(), sizeof(float) * out_y.numel());

    // Back to 8-bit (convertTo saturates, so no explicit clamp to [0,255] is needed).
    out_img_y.convertTo(out_img_y, CV_8U);

    cv::Mat YCbCr_output;
    std::vector<cv::Mat> new_channels = { out_img_y, out_img_cb, out_img_cr };
    cv::merge(new_channels, YCbCr_output);

#ifdef ENGINE // Unity can easily handle RGB24
    cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2RGB);
#else
    cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2BGR);
#endif

    final_time = clk::now();

#if SSRE_VERBOSE && !defined(ENGINE)
    std::cerr << "Super-resolved dimensions: " << out_img_y.size() << std::endl;
    LOG_MSG("Writing Y,Cb,Cr,YCbCr and RGB result to output/\n");
    std::filesystem::create_directories("output");
    cv::imwrite("output/Y-out.jpg", out_img_y);
    cv::imwrite("output/Cb-out.jpg", out_img_cb);
    cv::imwrite("output/Cr-out.jpg", out_img_cr);
    cv::imwrite("output/YCbCr-out.jpg", YCbCr_output);
    cv::imwrite("output/final_result.jpg", final_result);
#endif
}

int main(int argc, const char* const argv[])
{
    initial_time = clk::now();

    if (argc < 3 || argc > 4) {
        std::cerr << "usage: super_resolve <path-to-exported-script-module> <path-to-image> [cpu|cuda]\n";
        return -1;
    }

    torch::Device device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    if (argc == 4) {
        const std::string want = argv[3];
        if (want == "cpu") {
            device = torch::kCPU;
        } else if (want == "cuda") {
            if (!torch::cuda::is_available()) {
                std::cerr << "CUDA requested but not available in this LibTorch build\n";
                return 1;
            }
            device = torch::kCUDA;
        } else {
            std::cerr << "unknown device '" << want << "' (expected cpu or cuda)\n";
            return -1;
        }
    }

    torch::jit::Module module;
    try {
        module = loadModule(argv[1], device);
    }
    catch (const c10::Error& e) {
        std::cerr << "error loading the model: " << e.what() << std::endl;
        return 1;
    }

    cv::Mat ycbcr_image;
    cv::Mat final_result;

#ifndef ENGINE
    std::vector<uchar> buf;
    if (loadImage(argv[2], &buf)) {
        std::cerr << "Could not load image from memory\n";
        return 1;
    }

    cv::Mat bgr_image = cv::imdecode(buf, cv::IMREAD_COLOR);
    cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb);

    try {
        super_resolve(ycbcr_image, module, device, final_result);
    }
    catch (const std::exception& e) {
        std::cerr << "inference failed: " << e.what() << std::endl;
        return 1;
    }

    using secs = std::chrono::duration<double>;
    std::cout << "Loading took "        << secs(load_time - initial_time).count()   << " seconds" << std::endl;
    std::cout << "Upscaling took "      << secs(upscaling_time - load_time).count() << " seconds" << std::endl;
    std::cout << "Finalizing took "     << secs(final_time - upscaling_time).count()<< " seconds" << std::endl;
    std::cout << "Entire program took " << secs(final_time - initial_time).count()  << " seconds" << std::endl;
#else
    const long numDecodedBytes = 4 * (1920 / 2) * (1080 / 2);
    char* decodedBytes = new char[numDecodedBytes]; // owned and freed by ClientCommunicator

    const long numUpscaledBytes = 3 * 1920 * 1080;
    ClientCommunicator communicator(numDecodedBytes, numUpscaledBytes, decodedBytes);

    while (true)
    {
        communicator.GetSRInputs();

        input_ready_time = clk::now();

        cv::Mat bgr_image;
        cv::Mat rgba_bitmap = cv::Mat(1080 / 2, 1920 / 2, CV_8UC4, decodedBytes);
        cv::cvtColor(rgba_bitmap, bgr_image, cv::COLOR_RGBA2BGR);
        cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb);

        super_resolve(ycbcr_image, module, device, final_result);

        communicator.ReturnSROutputs(reinterpret_cast<char*>(final_result.data));

        output_ready_time = clk::now();

        using secs = std::chrono::duration<double>;
        std::cout << "Upscale: " << secs(upscaling_time - load_time).count() << std::endl;
        std::cout << "Total: "   << secs(output_ready_time - input_ready_time).count() << std::endl;
    }
#endif // !ENGINE

#ifdef _WIN32
    // Keep the console window open when launched by double-click, but don't
    // block when run from a script/CI with redirected input.
    if (_isatty(_fileno(stdin))) {
        std::cout << "Press any key to exit...";
        (void)_getch();
    }
#endif

    return 0;
}
