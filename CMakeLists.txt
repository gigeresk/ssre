cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(super-resolution-engine)

# Paths to LibTorch and OpenCV — override via -DCMAKE_PREFIX_PATH or env
# e.g. cmake .. -DCMAKE_PREFIX_PATH="/path/to/libtorch;/path/to/opencv/build"
list(APPEND CMAKE_PREFIX_PATH
    "${CMAKE_SOURCE_DIR}/libtorch/share/cmake/Torch"
    "${CMAKE_SOURCE_DIR}/opencv/build"
)

find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)

message(STATUS "LibTorch version : ${Torch_VERSION}")
message(STATUS "OpenCV version   : ${OpenCV_VERSION}")
message(STATUS "OpenCV includes  : ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(super_resolve super_resolve.cpp)

target_link_libraries(super_resolve
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
    opencv_highgui
    ${TORCH_LIBRARIES}
)

set_property(TARGET super_resolve PROPERTY CXX_STANDARD 14)
