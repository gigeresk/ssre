

#include <torch/script.h> // One-stop header.
#include <torch/torch.h> // Do I need this?  Didn't come with the example file.  TODO try without
#include <iostream>
#include <memory>

// #include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp> // opencv 3.2
#include <opencv2/imgproc/imgproc.hpp> //colour

#define _DEBUG 1

#ifdef _DEBUG // or #ifndef NDEBUG
#define LOG_MSG(...) printf(__VA_ARGS__) // Or simply LOG_MSG(msg) printf(msg)
#else
#define LOG_MSG(...)                     // Or LOG_MSG(msg)
#endif

/* TODO
  -cuda
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

  
  //img_to_tensor = ToTensor()
  // module.
  
  std::string image_path = "../resize_witness.jpg";
  //https://github.com/pytorch/pytorch/issues/12506
  
  cv::Mat bgr_image = cv::imread(image_path); // BGR
  cv::Mat ycbcr_image;

  cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb); // BGR to YCrCb
  std::vector<cv::Mat> chan;
  cv::split(ycbcr_image,chan);
  cv::Mat Y  = chan[0]; //1080x1920 (all) (wait, should be half)
  cv::Mat Cb = chan[1];
  cv::Mat Cr = chan[2]; //TODO optimize
  /* converting to 32F causes all sorts of problems...
   */

  //resize and downsize cb
  cv::Mat out_img_cb_tmp = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  cv::Mat cb_shrink_tmp;
  cv::resize(Cb, out_img_cb_tmp, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );
   cv::resize(out_img_cb_tmp, cb_shrink_tmp, cv::Size(Y.cols,Y.rows),0.5,0.5, cv::INTER_CUBIC );
  cv::Mat Y_tmp;
  cv::Mat Cr_tmp;
  Y.convertTo(Y_tmp, CV_32F);
  Y_tmp.convertTo(Y_tmp, CV_8U);
  cb_shrink_tmp.convertTo(cb_shrink_tmp, CV_32F);
  cb_shrink_tmp.convertTo(cb_shrink_tmp, CV_8U);
  Cr.convertTo(Cr_tmp, CV_32F);
  Cr_tmp.convertTo(Cr_tmp, CV_8U);
  //to debug
  cv::Mat original_out;
  std::vector<cv::Mat> original_channels = { Y_tmp, cb_shrink_tmp, Cr_tmp };
  cv::merge(original_channels, original_out);
  cv::Mat result;
  cv::cvtColor(original_out, result, cv::COLOR_YCrCb2BGR);
  imwrite( "../original_out.jpg", result );
  //end debug (delete above)



  //convert Y channel to tensor
  //TODO difference between torch::from_blob and at::from_blob
  at::Tensor tensor_image = torch::from_blob(Y.data, {1, 1, Y.rows, Y.cols}, at::kByte); //TODO is kByte appropriate?
  tensor_image = tensor_image.to(at::kFloat); //will convert the 8-bit values into 32-bit floating points just as if you wrote static_cast<float>(b) where b is a byte

  /*MODEL*/
  std::vector<torch::jit::IValue> inputs;
  inputs.emplace_back(tensor_image); //TODO vs emplace back?
  at::Tensor output = module->forward(inputs).toTensor();

  // out_img_y = out[0].detach().numpy():
  at::Tensor detached_output = output[0].detach();
  //DELETE: presumably the model has executed

  int size[4] = {1, 1,Y.rows*2 ,Y.cols *2};
  const std::vector< int > size_vec = {Y.rows*2 ,Y.cols *2};
  cv::Mat *cv_mat = new cv::Mat(4, size,CV_32F); //TODO cv_32f? //cv::Scalar(0)?
  cv::Mat out_img_y = cv::Mat(size_vec, CV_32F);

  // out_img_y.resize(1, 4, sizes)
  //TODO WANT TO HAVE CV_MAT BE DYNAMIC (NOT ON STACK) also   /// http://www.bnikolic.co.uk/blog/pytorch-detach.html
  std::cout << "cv::mat2 size " << out_img_y.size() << std::endl;
  std::cout << "cv::mat2 dims " << out_img_y.dims << std::endl;
  std::cout << "output sizes " << detached_output.sizes() << std::endl;

  // auto bruh = out_img_y.at<float>(0);
  // std::cout << "content " << detached_output << std::endl; //prints way too much

  std::cout << "tensor # elem: " << detached_output.numel() << std::endl; //eg 8294400
  // std::memcpy(detached_output.data_ptr(), cv_mat->data, sizeof(float)*detached_output.numel()); //numel=number of elements
  std::memcpy((void *) out_img_y.data, detached_output[0].data_ptr(), 4*detached_output.numel()); //numel=number of elements
  //TODO important: detached_output[0] vs detached_output (above line) 
//  std::cout << "content " << out_img_y << std::endl;
  //keep one of above...
  std::cout << "out_img_y size " << out_img_y.size() << std::endl;
  std::cout << "cv::mat size " << cv_mat->size() << std::endl;
  // out_img_y *= 255.0:

//hmm.. the values are already between 0 and 255 apparently
  // *cv_mat = *cv_mat * 255;
  // out_img_y = out_img_y * 255;

  // out_img_y = out_img_y.clip(0, 255)
  // double rval1 = cv::threshold(out_img_y, out_img_y, 255.0, 255.0, cv::THRESH_TRUNC); //truncate
  // double rval2 = cv::threshold(-out_img_y, out_img_y, 0, 0, CV_THRESH_TRUNC); //??


  //hmm.. the values are already between 0 and 255 apparently
  double rval1 = cv::threshold(out_img_y, out_img_y, 255.0, 255.0, cv::THRESH_TRUNC); //truncate
  // double rval2 = cv::threshold(-out_img_y, out_img_y, 0, 0, CV_THRESH_TRUNC); //??
  //TODO ensure nothing is below 0!


  // std::cout << "content " << out_img_y << std::endl; //prints way too much
  // out_img_y = Image.fromarray(np.uint8(out_img_y[0]), mode='L')
  

  // imwrite( "../cpp-out.jpg", out_img_y );

  std::cout << "cv::mat2 size " << out_img_y.size() << std::endl;
  
  // out_img_cb = cb.resize(out_img_y.size, Image.BICUBIC)
  
  cv::Mat out_img_cb = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  //TODO verify... is that the right size, etc.?
  cv::resize(Cb, out_img_cb, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );

  // out_img_cr = cr.resize(out_img_y.size, Image.BICUBIC)

  cv::Mat out_img_cr = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  cv::resize(Cr, out_img_cr, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );
  //TODO is this resizing? I thought this was initial size...

  // out_img = Image.merge('YCbCr', [out_img_y, out_img_cb, out_img_cr]).convert('RGB')

  cv::Mat YCbCr_output;// = new Mat;
  std::cout << "" << std::endl;
  std::cout << "out_img_y size " << out_img_y.size() << std::endl;
  std::cout << "out_img_cb size " << out_img_cb.size() << std::endl;
  std::cout << "out_img_cr size " << out_img_cr.size() << std::endl;
  //uhh
  out_img_cb.convertTo(out_img_cb, CV_32F); //TODO not ideal
  out_img_cr.convertTo(out_img_cr, CV_32F); //TODO not ideal
  //depth is datatype
  std::cout << "out_img_y depth " << out_img_y.depth() << std::endl; //5
  std::cout << "out_img_cb depth " << out_img_cb.depth() << std::endl; //0
  std::cout << "out_img_cr depth " << out_img_cr.depth() << std::endl; //0

  // need to convert (back) to 8-bit unsigned
  out_img_y.convertTo(out_img_y, CV_8U);
  out_img_cb.convertTo(out_img_cb, CV_8U);
  out_img_cr.convertTo(out_img_cr, CV_8U);
  std::vector<cv::Mat> new_channels = { out_img_y, out_img_cb, out_img_cr };
  cv::merge(new_channels, YCbCr_output);


  cv::Mat final_result;

  cv::cvtColor(YCbCr_output, final_result, cv::COLOR_YCrCb2BGR);
  // out_img.save(opt.output_filename)
  // print('output image saved to ', opt.output_filename)

  #ifdef _DEBUG
  LOG_MSG("Writing Y,Cb,Cr,YCbCr and RGB result to file");
  imwrite( "../Y-out.jpg", out_img_y );
  imwrite( "../Cb-out.jpg", out_img_cb );
  imwrite( "../Cr-out.jpg", out_img_cr );
  imwrite( "../YCbCr-out.jpg", YCbCr_output );
  imwrite( "../final_result.jpg", final_result );
  #endif
}