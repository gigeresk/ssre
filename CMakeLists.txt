cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(super-resolution-engine LANGUAGES CXX)

# Current LibTorch (2.x) requires C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SSRE_ENGINE  "Build the Windows shared-memory engine mode used by the Unity client" OFF)
option(SSRE_VERBOSE "Print timings and dump intermediate Y/Cb/Cr images to output/"      ON)

# Default search locations: libtorch/ and opencv/ extracted into the project root.
# Override with -DCMAKE_PREFIX_PATH="C:/path/to/libtorch;C:/path/to/opencv/build"
list(APPEND CMAKE_PREFIX_PATH
    "${CMAKE_SOURCE_DIR}/libtorch"
    "${CMAKE_SOURCE_DIR}/opencv/build"
)

find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

message(STATUS "LibTorch version : ${Torch_VERSION}")
message(STATUS "OpenCV version   : ${OpenCV_VERSION}")

add_executable(super_resolve super_resolve.cpp ClientCommunicator.h)
target_include_directories(super_resolve PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(super_resolve PRIVATE ${TORCH_LIBRARIES} ${OpenCV_LIBS})
target_compile_features(super_resolve PRIVATE cxx_std_20)

if(SSRE_VERBOSE)
    target_compile_definitions(super_resolve PRIVATE SSRE_VERBOSE=1)
else()
    target_compile_definitions(super_resolve PRIVATE SSRE_VERBOSE=0)
endif()

if(SSRE_ENGINE)
    if(NOT WIN32)
        message(FATAL_ERROR "SSRE_ENGINE uses Win32 shared memory and is Windows-only")
    endif()
    target_compile_definitions(super_resolve PRIVATE ENGINE=1)
endif()

if(WIN32)
    # Keep <Windows.h> from defining min/max macros that collide with LibTorch/OpenCV.
    target_compile_definitions(super_resolve PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(MSVC)
    target_compile_options(super_resolve PRIVATE /utf-8 /Zc:__cplusplus)

    # CUDA builds of LibTorch on Windows: make sure torch_cuda.dll is actually
    # linked so torch::cuda::is_available() can return true.
    if(TARGET torch_cuda)
        target_link_options(super_resolve PRIVATE "/INCLUDE:?warp_size@cuda@at@@YAHXZ")
    endif()

    # The Release LibTorch package links the release CRT (/MD); a Debug build of
    # this program links /MDd and crashes at runtime. Unless you've downloaded the
    # separate *debug* LibTorch package, only allow release-CRT configurations.
    option(SSRE_LIBTORCH_DEBUG "LibTorch is the debug package (allows Debug builds)" OFF)
    if(NOT SSRE_LIBTORCH_DEBUG)
        get_property(_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if(_multi)
            set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "" FORCE)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(FATAL_ERROR
                "Debug build with Release LibTorch will crash (CRT mismatch). Use "
                "-DCMAKE_BUILD_TYPE=Release or RelWithDebInfo, or download the debug LibTorch "
                "package and pass -DSSRE_LIBTORCH_DEBUG=ON.")
        endif()
    endif()

    # Copy LibTorch and OpenCV DLLs next to the executable so it runs in place.
    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET super_resolve POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${TORCH_DLLS}
                $<TARGET_RUNTIME_DLLS:super_resolve>
                $<TARGET_FILE_DIR:super_resolve>
        COMMAND_EXPAND_LISTS
    )
endif()
