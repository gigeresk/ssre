

#include <torch/script.h> // One-stop header.
#include <torch/torch.h> // Do I need this?  Didn't come with the example file.  TODO try without
#include <iostream>
#include <memory>

// #include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp> // opencv 3.2
#include <opencv2/imgproc/imgproc.hpp> //colour

// #define _DEBUG 1

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

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: example-app <path-to-exported-script-module>\n";
    return -1;
  }

  // Deserialize the ScriptModule from a file using torch::jit::load().
  std::shared_ptr<torch::jit::script::Module> module = torch::jit::load(argv[1]);

  assert(module != nullptr);
  std::cout << "ok, loaded module\n";

  std::string image_path = "../resize_witness.jpg";
  
  cv::Mat bgr_image = cv::imread(image_path); // BGR
  cv::Mat ycbcr_image;

  cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb);
  std::vector<cv::Mat> chan;
  cv::split(ycbcr_image,chan);
  cv::Mat Y  = chan[0];
  cv::Mat Cb = chan[1];
  cv::Mat Cr = chan[2]; //TODO optimize

  //convert Y channel to tensor
  at::Tensor tensor_image = torch::from_blob(Y.data, {1, 1, Y.rows, Y.cols}, at::kByte); //TODO is kByte appropriate?
  tensor_image = tensor_image.to(at::kFloat); //will convert the 8-bit values into 32-bit floating points just as if you wrote static_cast<float>(b) where b is a byte

  /*MODEL*/
  std::vector<torch::jit::IValue> inputs;
  inputs.emplace_back(tensor_image); //TODO pushback vs emplace back?
  at::Tensor output = module->forward(inputs).toTensor();

  at::Tensor detached_output = output[0].detach();

  const std::vector< int > size_vec = {Y.rows*2 ,Y.cols *2};
  cv::Mat out_img_y = cv::Mat(size_vec, CV_32F);

  std::memcpy((void *) out_img_y.data, detached_output[0].data_ptr(), 4*detached_output.numel()); //TODO important: detached_output[0] vs detached_output (above line) 
  //keep one of above...

  // Seem not to need to clip between 0.0 and 255.0

  cv::Mat out_img_cb = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  cv::resize(Cb, out_img_cb, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );

  cv::Mat out_img_cr = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  cv::resize(Cr, out_img_cr, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );

  cv::Mat YCbCr_output;

  // need to convert Y (back) to 8-bit unsigned
  out_img_y.convertTo(out_img_y, CV_8U);

  std::vector<cv::Mat> new_channels = { out_img_y, out_img_cb, out_img_cr };
  cv::merge(new_channels, YCbCr_output);

  cv::Mat final_result;

  cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2BGR);

  #ifdef _DEBUG
  std::cerr << "Super-resolved dimensions: "<< out_img_y.size() << std::endl;
  LOG_MSG("Writing Y,Cb,Cr,YCbCr and RGB result to file");
  imwrite( "../Y-out.jpg", out_img_y );
  imwrite( "../Cb-out.jpg", out_img_cb );
  imwrite( "../Cr-out.jpg", out_img_cr );
  imwrite( "../YCbCr-out.jpg", YCbCr_output );
  imwrite( "../final_result.jpg", final_result );
  #endif
}