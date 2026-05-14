cmake_minimum_required(VERSION 3.29)
project(Eliotopy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(${OpenCV_INCLUDE_DIRS})

set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED)

add_executable(Eliotopy main.cpp)
target_link_libraries(Eliotopy user32 gdi32 ${OpenCV_LIBS})
