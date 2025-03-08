cmake_minimum_required(VERSION 3.10)

# 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 添加 -O3 优化（仅限 Release 模式）
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")

project(HFT)

set(CMAKE_CXX_STANDARD 17)


find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
# 添加 WebSocket++ 头文件路径
include_directories(/usr/local/include)


# 添加可执行文件
add_executable(HFT main.cpp)

# 链接必要的库
target_link_libraries(HFT pthread OpenSSL::SSL OpenSSL::Crypto ${CURL_LIBRARIES})
