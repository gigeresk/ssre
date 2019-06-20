cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(example-app)
add_executable(example-app example-app.cpp)

set(CMAKE_PREFIX_PATH "/home/fgjtombs/fydp/libtorch/share/cmake/Torch/;/usr/local/include/opencv4/;/usr/local/lib")
find_package(Torch REQUIRED)

find_package(OpenCV REQUIRED)

target_link_libraries(example-app ${OpenCV_LIBS})


INCLUDE_DIRECTORIES( "/usr/local/include/opencv4/;/usr/local/lib" )
message("DEBUG")
message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")




target_link_libraries(example-app ${OpenCV_LIBS} ${TORCH_LIBRARIES}  )

set_property(TARGET example-app PROPERTY CXX_STANDARD 11)
