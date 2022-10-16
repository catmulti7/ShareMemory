cmake_minimum_required(VERSION 3.1)
project(ShareMemory)

set(CMAKE_CXX_STANDARD 20)
set(OpenCV_DIR /usr/local/share/OpenCV)
find_package(OpenCV REQUIRED)

include_directories(include)
include_directories(${OpenCV_INCLUDE_DIRS})


add_executable(write src/write.cpp)
add_executable(read src/read.cpp)

#target_link_libraries(template ${OpenCV_LIBRARIES})
target_link_libraries(write ${OpenCV_LIBRARIES})
target_link_libraries(read ${OpenCV_LIBRARIES})


#target_link_libraries(helloworld Ceres::ceres)