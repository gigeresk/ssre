cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(example-app)

set(CMAKE_PREFIX_PATH "/home/fgjtombs/fydp/libtorch/share/cmake/Torch/;/usr/local/include/opencv4/;/usr/local/lib")
find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( "/usr/local/include/opencv4/;/usr/local/lib" )
message(${OpenCV_INCLUDE_DIRS})


add_executable(example-app example-app.cpp)

target_link_libraries(example-app opencv_core opencv_imgproc opencv_imgcodecs opencv_highgui ${TORCH_LIBRARIES}  )

set_property(TARGET example-app PROPERTY CXX_STANDARD 11)
