cmake_minimum_required(VERSION 3.0)  # 最低 CMake 版本要求
project(rk_cam VERSION 1.0 LANGUAGES C CXX)          # 项目名称和版本

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

add_subdirectory(smolrtsp)
# 1. 可执行文件定义
# -------------------------------------------------
# 源文件列表（自动匹配当前目录下的文件）
file(GLOB SRCS 
    main.cpp
    encoder.cpp
    camera.cpp
    streamer_encoder.c
)

# link_directories(smolrtsp/build)

# 生成可执行目标
add_executable(rk_cam ${SRCS})

# 2. 编译选项配置
# -------------------------------------------------
# 全局警告选项（兼容 GCC/Clang/MSVC）
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rk_cam PRIVATE -Wall -g)
elseif(MSVC)
    target_compile_options(rk_cam PRIVATE /W4 /Zi)
endif()

# 包含路径（等效于 -I 选项）
target_include_directories(rk_cam PRIVATE
    /usr/local/include/rockchip
    smolrtsp/include
)

# 3. 链接配置
# -------------------------------------------------
# 库搜索路径（等效于 -L 选项）
target_link_directories(rk_cam PRIVATE /usr/local/lib)

# 动态库链接（等效于 -l 选项）
target_link_libraries(rk_cam PRIVATE
    pthread
    rockchip_mpp
    smolrtsp
)