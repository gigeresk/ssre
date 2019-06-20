cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(super_resolve)
add_executable(super_resolve super_resolve.cpp)

set(CMAKE_PREFIX_PATH "/home/fgjtombs/fydp/libtorch/share/cmake/Torch/;/usr/local/include/opencv4/;/usr/local/lib")
find_package(Torch REQUIRED)

find_package(OpenCV REQUIRED)

target_link_libraries(super_resolve ${OpenCV_LIBS})


INCLUDE_DIRECTORIES( "/usr/local/include/opencv4/;/usr/local/lib" )
message("DEBUG")
message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")




target_link_libraries(super_resolve ${OpenCV_LIBS} ${TORCH_LIBRARIES}  )

set_property(TARGET super_resolve PROPERTY CXX_STANDARD 11)
