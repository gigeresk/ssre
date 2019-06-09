#include <torch/script.h> // One-stop header.
#include <torch/torch.h> // Do I need this?  Didn't come with the example file.  TODO try without
#include <iostream>
#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
// #include <opencv2/cv.hpp>
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
  
    std::string image_path = "../magick_witness.jpg";

    cv::Mat image = cv::imread(image_path,0);
  //at::Tensor tensor_image = torch::from_blob(image.data, {1, 3, image.rows, image.cols}, at::kByte);
  /*tensor_image = tensor_image.to(at::kFloat);
  */
  //input = img_to_tensor(y).view(1, -1, y.size[1], y.size[0])

  // out = model(input)
  // out = out.cpu()
  // out_img_y = out[0].detach().numpy()
  // out_img_y *= 255.0
  // out_img_y = out_img_y.clip(0, 255)
  // out_img_y = Image.fromarray(np.uint8(out_img_y[0]), mode='L')

  // out_img_cb = cb.resize(out_img_y.size, Image.BICUBIC)
  // out_img_cr = cr.resize(out_img_y.size, Image.BICUBIC)
  // out_img = Image.merge('YCbCr', [out_img_y, out_img_cb, out_img_cr]).convert('RGB')

  // out_img.save(opt.output_filename)
  // print('output image saved to ', opt.output_filename)


}



/*
#include <torch/torch.h>
#include <iostream>

int main() {
  torch::Tensor tensor = torch::rand({2, 3});
  std::cout << tensor << std::endl;
}*/
