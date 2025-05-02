# 编译器设置
CXX := g++                            # 使用 g++ 编译器
TARGET := rk_cam                       # 最终生成的可执行文件名

# 源代码配置
SRC_DIR := .                          # 源代码目录（默认为当前目录）
SRCS := main.cpp 
OBJS :=  $(patsubst %.cpp,%.o,$(notdir $(SRCS)))           # 生成对应的 .o 文件列表

# 编译选项
CXXFLAGS := -Wall  -O2         # 常用警告和优化选项
CXXFLAGS += -I/usr/local/include      # 如果需要头文件，添加包含路径

# 链接选项
LDFLAGS := -L/usr/local/lib           # 库文件搜索路径
STATIC_LIBS := /usr/local/lib/librockchip_mpp.a  # 直接指定静态库绝对路径

# 其他依赖库（按需添加）
# LDLIBS := -lpthread -ldl            # 示例：添加动态库依赖

# 默认构建目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(STATIC_LIBS) -o $@
# 编译源文件生成对象文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理构建产物
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean