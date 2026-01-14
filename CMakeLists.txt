cmake_minimum_required(VERSION 3.10)
project(MyMinecraft)

# 设置 C++ 标准为 C++17 (现代特性支持)
set(CMAKE_CXX_STANDARD 17)

# 1. 包含头文件路径
# 告诉编译器去 dependencies/include 找头文件
include_directories(${CMAKE_SOURCE_DIR}/dependencies/include)

# 2. 收集源代码
# 包含 src 下的所有 .cpp 和 .c 文件 (main.cpp 和 glad.c)
file(GLOB SOURCES "src/*.cpp" "src/*.c")

# 3. 创建可执行文件
add_executable(MyMinecraft ${SOURCES})

# 4. 链接库文件
# 告诉链接器去 dependencies/lib 找库文件
link_directories(${CMAKE_SOURCE_DIR}/dependencies/lib)

# 链接具体的库
if(WIN32)
    # Windows 下使用静态库 libglfw3.a
    target_link_libraries(MyMinecraft 
        ${CMAKE_SOURCE_DIR}/dependencies/lib/libglfw3.a
        opengl32 
        gdi32
    )
elseif(UNIX)
    # Linux/Mac 可能需要的库 (预留)
    target_link_libraries(MyMinecraft glfw GL dl X11 pthread)
endif()