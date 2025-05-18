cmake_minimum_required(VERSION 3.0)  # 最低 CMake 版本要求
project(rk_cam VERSION 1.0 LANGUAGES C CXX)          # 项目名称和版本

set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_ARCH "aarch64" CACHE STRING "target aarch to build for")
include(ExternalProject)

# 定义 Submodule 的构建步骤
ExternalProject_Add(
  mpp_submodule
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/mpp/build/linux/${TARGET_ARCH}/
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "./make-Makefiles.bash"
  INSTALL_COMMAND ""  # 不执行安装
  BUILD_IN_SOURCE TRUE  # 在源码目录构建
  BUILD_ALWAYS FALSE     # 每次主项目构建时重新构建
)

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
    mpp/inc
    smolrtsp/include
)

# 3. 链接配置
# -------------------------------------------------
# 库搜索路径（等效于 -L 选项）
target_link_directories(rk_cam PRIVATE mpp/build/linux/arm/mpp)

# 动态库链接（等效于 -l 选项）
target_link_libraries(rk_cam PRIVATE
    pthread
    ${CMAKE_SOURCE_DIR}/mpp/build/linux/${TARGET_ARCH}/mpp/librockchip_mpp.a
    smolrtsp
)