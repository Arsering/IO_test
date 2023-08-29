cmake_minimum_required (VERSION 3.5)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_FLAGS "-fxray-instrument")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -lpthread -O0")
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)
 
project(IO_test)
 
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
 
aux_source_directory(${PROJECT_SOURCE_DIR} ALL_SRCS) # 添加当前目录下所有的源文件
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

# 包含静态库路径
link_directories(${LIBRARY_OUTPUT_PATH})
# 链接静态库
link_libraries(fio_mine)

add_executable(${PROJECT_NAME} ${ALL_SRCS})	# 指定生成目标，注意这里要用${ALL_SRCS}!!!

target_link_libraries(${PROJECT_NAME} boost_filesystem)


