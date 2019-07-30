/*
VS: DO NOT BUILD IN DEBUG, ONLY RELEASE: https://github.com/pytorch/pytorch/issues/15919
*/

#include <torch/script.h> // One-stop header.
#include <iostream>
#include <memory>

#include <opencv2/imgcodecs.hpp> // opencv 3.2
#include <opencv2/imgproc/imgproc.hpp> //colour

#ifdef _WIN32
#include <conio.h>
#ifdef ENGINE
#include "ClientCommunicator.h"
#endif
#endif // _WIN32

#define _DEBUG 1

#ifdef _DEBUG // or #ifndef NDEBUG
#define LOG_MSG(...) printf(__VA_ARGS__) // Or simply LOG_MSG(msg) printf(msg)
#else
#define LOG_MSG(...)                     // Or LOG_MSG(msg)
#endif

/* TODO
  -cuda
  -want to have out_img_y be on heap or on stack
  -intelligently use CV_32F and CV_8U
  -difference between torch::from_blob and at::from_blob??
  -detach: http://www.bnikolic.co.uk/blog/pytorch-detach.html (heap/stack)
  bookmarks
  -https://github.com/pytorch/pytorch/issues/12506s
 */

using clk = std::chrono::high_resolution_clock;
using module_ptr = std::shared_ptr<torch::jit::script::Module>;

clk::time_point initial_time;
clk::time_point load_time;
clk::time_point upscaling_time;
clk::time_point final_time;

std::vector<cv::Mat> ycrcb_channels(3);

int loadImage(std::string image_path, std::vector<uchar> *buf){

    cv::Mat bgr_image = cv::imread(image_path, cv::IMREAD_COLOR );
    // std::vector<uchar>* buf;
    try{
        cv::imencode(".jpg", bgr_image, *buf);
    }
    catch(...){
        std::cerr << "Could not encode image into memory";
        return 1;
    }
    return 0;
}

inline module_ptr loadModule(const char* const moduleName)
{
    // Deserialize the ScriptModule from a file using torch::jit::load().
    std::shared_ptr<torch::jit::script::Module> module = torch::jit::load(moduleName, torch::kCUDA);

    assert(module != nullptr);
    std::cout << "ok, loaded module\n";

    return module;
}

inline void super_resolve(cv::Mat& ycbcr_image, module_ptr& module, cv::Mat& final_result)
{
    //std::vector<cv::Mat> chan;
    cv::split(ycbcr_image, ycrcb_channels);
    cv::Mat Y = ycrcb_channels[0];
    cv::Mat Cb = ycrcb_channels[1];
    cv::Mat Cr = ycrcb_channels[2];

    //convert Y channel to tensor
    at::Tensor tensor_image = torch::from_blob(Y.data, { 1, 1, Y.rows, Y.cols }, at::kByte); //TODO is kByte appropriate?
    tensor_image = tensor_image.to(at::kFloat); //will convert the 8-bit values into 32-bit floating points just as if you wrote static_cast<float>(b) where b is a byte

    /*MODEL*/
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor_image.to(torch::kCUDA));

    load_time = clk::now();

    at::Tensor output = module->forward(inputs).toTensor().to(torch::kCPU);

    upscaling_time = clk::now();

    at::Tensor detached_output = output[0].detach();

    const std::vector< int > size_vec = { Y.rows * 2 ,Y.cols * 2 };
    cv::Mat out_img_y = cv::Mat(size_vec, CV_32F);

    std::memcpy((void*)out_img_y.data, detached_output[0].data_ptr(), 4 * detached_output.numel()); //TODO important: detached_output[0] vs detached_output (above line) 
    //keep one of above...

    // Seem not to need to clip between 0.0 and 255.0

    cv::Mat out_img_cb = cv::Mat(Y.cols * 2, Y.rows * 2, CV_32F);
    cv::resize(Cb, out_img_cb, cv::Size(Y.cols * 2, Y.rows * 2), 2.0, 2.0, cv::INTER_CUBIC);

    cv::Mat out_img_cr = cv::Mat(Y.cols * 2, Y.rows * 2, CV_32F);
    cv::resize(Cr, out_img_cr, cv::Size(Y.cols * 2, Y.rows * 2), 2.0, 2.0, cv::INTER_CUBIC);

    cv::Mat YCbCr_output;

    // need to convert Y (back) to 8-bit unsigned
    out_img_y.convertTo(out_img_y, CV_8U);

    std::vector<cv::Mat> new_channels = { out_img_y, out_img_cb, out_img_cr };
    cv::merge(new_channels, YCbCr_output);

#ifdef ENGINE // Unity can easily handle RGB24
    cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2RGB);
#else
    cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2BGR);
#endif // ENGINE

    final_time = clk::now();

#if defined(_DEBUG) && !defined(ENGINE) 
    std::cerr << "Super-resolved dimensions: " << out_img_y.size() << std::endl;
    LOG_MSG("Writing Y,Cb,Cr,YCbCr and RGB result to file\n");
    //todo do I need to make sure output/ exists?
    imwrite("output/Y-out.jpg", out_img_y);
    imwrite("output/Cb-out.jpg", out_img_cb);
    imwrite("output/Cr-out.jpg", out_img_cr);
    imwrite("output/YCbCr-out.jpg", YCbCr_output);
    imwrite("output/final_result.jpg", final_result);
#endif // DEBUG
}

int main(int argc, const char* const argv[]) {

    initial_time = clk::now();

    if (argc != 3) {
        std::cerr << "usage: example-app <path-to-exported-script-module> <path-to-image> \n";
        return -1;
    }

    // Deserialize the ScriptModule from a file using torch::jit::load().
    module_ptr module = loadModule(argv[1]);
    
    cv::Mat ycbcr_image;
    cv::Mat final_result;

#ifndef ENGINE
    std::vector<uchar> buf;
    if (loadImage(argv[2], &buf)) {
        std::cerr << "Could not load image from memory";
        return 1;
    }

    cv::Mat bgr_image = cv::imdecode(buf, cv::IMREAD_COLOR);
    cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb);

    super_resolve(ycbcr_image, module, final_result);

    std::chrono::duration<double> load_timespan = std::chrono::duration_cast<std::chrono::duration<double>>(load_time - initial_time);
    std::chrono::duration<double> upscaling_timespan = std::chrono::duration_cast<std::chrono::duration<double>>(upscaling_time - load_time);
    std::chrono::duration<double> final_timespan = std::chrono::duration_cast<std::chrono::duration<double>>(final_time - upscaling_time);
    std::chrono::duration<double> total_timespan = std::chrono::duration_cast<std::chrono::duration<double>>(final_time - initial_time);
    std::cout << "Loading took " << load_timespan.count() << " seconds" << std::endl;
    std::cout << "Upscaling took " << upscaling_timespan.count() << " seconds" << std::endl;
    std::cout << "Finalizing took " << final_timespan.count() << " seconds" << std::endl;
    std::cout << "Entire program took " << total_timespan.count() << " seconds" << std::endl;
#else
    long  numDecodedBytes = 4 * (1920/2) * (1080/2);
    char* decodedBytes = new char[numDecodedBytes];

    long  numUpscaledBytes = 3 * 1920 * 1080;
    ClientCommunicator communicator(numDecodedBytes, numUpscaledBytes, decodedBytes);

    while (true)
    {
        communicator.GetSRInputs();
        cv::Mat bgr_image;
        cv::Mat rgba_bitmap = cv::Mat(1080/2, 1920/2, CV_8UC4, decodedBytes);
        cv::cvtColor(rgba_bitmap, bgr_image, cv::COLOR_RGBA2BGR);
        cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb);

        super_resolve(ycbcr_image, module, final_result);

        communicator.ReturnSROutputs((LPTCH) final_result.data);
    }
#endif // !ENGINE

#ifdef _WIN32
    std::cout << "Press any key to exit...";
    int throwaway = getch(); // Wait for any keypress, throw away value
#endif // _WIN32

    return 0;
}