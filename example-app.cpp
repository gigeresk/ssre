#include <torch/script.h> // One-stop header.
#include <torch/torch.h> // Do I need this?  Didn't come with the example file.  TODO try without
#include <iostream>
#include <memory>

// #include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp> // opencv 3.2
#include <opencv2/imgproc/imgproc.hpp> //colour

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
  
  std::string image_path = "../resize_fallout.jpg";
  //https://github.com/pytorch/pytorch/issues/12506
  
  cv::Mat bgr_image = cv::imread(image_path); // BGR
  cv::Mat ycbcr_image;

  cv::cvtColor(bgr_image, ycbcr_image, cv::COLOR_BGR2YCrCb); // BGR to YCrCb
  cv::Mat chan[3];
  cv::split(ycbcr_image,chan);
  cv::Mat Y  = chan[0]; //1080x1920 (all) (wait, should be half)
  cv::Mat Cb = chan[1];
  cv::Mat Cr = chan[2]; //TODO optimize
  
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
  cv::Mat cv_mat2 = cv::Mat(size_vec, CV_32F);

  // cv_mat2.resize(1, 4, sizes)
  //TODO WANT TO HAVE CV_MAT BE DYNAMIC (NOT ON STACK) also   /// http://www.bnikolic.co.uk/blog/pytorch-detach.html
  std::cout << "cv::mat2 size " << cv_mat2.size() << std::endl;
    std::cout << "cv::mat2 dims " << cv_mat2.dims << std::endl;
  std::cout << "output sizes " << detached_output.sizes() << std::endl;

  // auto bruh = cv_mat2.at<float>(0);
  // std::cout << "content " << detached_output << std::endl; //prints way too much

  std::cout << "tensor # elem: " << detached_output.numel() << std::endl; //eg 8294400
  // std::memcpy(detached_output.data_ptr(), cv_mat->data, sizeof(float)*detached_output.numel()); //numel=number of elements
  std::memcpy((void *) cv_mat2.data, detached_output[0].data_ptr(), 4*detached_output.numel()); //numel=number of elements
  //TODO important: detached_output[0] vs detached_output (above line) 
//  std::cout << "content " << cv_mat2 << std::endl;
  //keep one of above...
  std::cout << "cv::mat2 size " << cv_mat2.size() << std::endl;
  std::cout << "cv::mat size " << cv_mat->size() << std::endl;
  // out_img_y *= 255.0:

//hmm.. the values are already between 0 and 255 apparently
  // *cv_mat = *cv_mat * 255;
  // cv_mat2 = cv_mat2 * 255;

  // out_img_y = out_img_y.clip(0, 255)
  // double rval1 = cv::threshold(cv_mat2, cv_mat2, 255.0, 255.0, cv::THRESH_TRUNC); //truncate
  // double rval2 = cv::threshold(-cv_mat2, cv_mat2, 0, 0, CV_THRESH_TRUNC); //??


  //hmm.. the values are already between 0 and 255 apparently
  double rval1 = cv::threshold(cv_mat2, cv_mat2, 255.0, 255.0, cv::THRESH_TRUNC); //truncate
  // double rval2 = cv::threshold(-cv_mat2, cv_mat2, 0, 0, CV_THRESH_TRUNC); //??
  //TODO ensure nothing is below 0!


  // std::cout << "content " << cv_mat2 << std::endl; //prints way too much
  // out_img_y = Image.fromarray(np.uint8(out_img_y[0]), mode='L')
  

  imwrite( "../cpp-out.jpg", cv_mat2 );

  std::cout << "cv::mat2 size " << cv_mat2.size() << std::endl;
  
  // out_img_cb = cb.resize(out_img_y.size, Image.BICUBIC)
  
  cv::Mat out_img_cb = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  //TODO verify... is that the right size, etc.?
  resize(Cb, out_img_cb, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );

  // out_img_cr = cr.resize(out_img_y.size, Image.BICUBIC)

  cv::Mat out_img_cr = cv::Mat(Y.cols*2,Y.rows*2, CV_32F);
  cv::resize(Cr, out_img_cr, cv::Size(Y.cols*2,Y.rows*2),2.0,2.0, cv::INTER_CUBIC );
  //TODO is this resizing? I thought this was initial size...

  // out_img = Image.merge('YCbCr', [out_img_y, out_img_cb, out_img_cr]).convert('RGB')

  cv::Mat YCbCr_output;// = new Mat;
  std::cout << "" << std::endl;
  std::cout << "cv_mat2 size " << cv_mat2.size() << std::endl;
  std::cout << "out_img_cb size " << out_img_cb.size() << std::endl;
  std::cout << "out_img_cr size " << out_img_cr.size() << std::endl;
  //uhh
  out_img_cb.convertTo(out_img_cb, CV_32F); //TODO not ideal
  out_img_cr.convertTo(out_img_cr, CV_32F); //TODO not ideal
  //depth is datatype
  std::cout << "cv_mat2 depth " << cv_mat2.depth() << std::endl; //5
  std::cout << "out_img_cb depth " << out_img_cb.depth() << std::endl; //0
  std::cout << "out_img_cr depth " << out_img_cr.depth() << std::endl; //0
  std::vector<cv::Mat> new_channels = { cv_mat2, out_img_cb, out_img_cr };
  cv::merge(new_channels, YCbCr_output);

  // out_img.save(opt.output_filename)
  // print('output image saved to ', opt.output_filename)

  imwrite( "../cpp-out.jpg", YCbCr_output );
  int a = 1;
}