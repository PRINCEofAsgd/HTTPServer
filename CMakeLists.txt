# 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称和使用的编程语言
project(HTTPSERVER CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# 设置构建输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ========== 核心编译部分 ==========
# 1. 排除 main.cpp 和 main_http.cpp，把其他所有源文件编译成静态库
file(GLOB ALL_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/http/*.cpp ${CMAKE_SOURCE_DIR}/src/core/*.cpp ${CMAKE_SOURCE_DIR}/src/redis/*.cpp ${CMAKE_SOURCE_DIR}/src/mysql/*.cpp)
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".+/main\.cpp$")
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".+/main_http\.cpp$")

# 2. 编译静态库
add_library(httpserver_lib STATIC ${ALL_SOURCE_FILES})

# 3. 构建Echo服务器程序
file(GLOB MAIN_FILE ${CMAKE_SOURCE_DIR}/src/main_echo.cpp)
if (MAIN_FILE)
    add_executable(echoserver ${MAIN_FILE})
    target_link_libraries(echoserver httpserver_lib)
    target_link_libraries(echoserver pthread hiredis mysqlclient)
else()
    message(WARNING "未找到 main_echo.cpp！")
endif()

# 4. 构建HTTP服务器程序
file(GLOB MAIN_HTTP_FILE ${CMAKE_SOURCE_DIR}/src/main_http.cpp)
if (MAIN_HTTP_FILE)
    add_executable(httpserver ${MAIN_HTTP_FILE})
    target_link_libraries(httpserver httpserver_lib)
    target_link_libraries(httpserver pthread hiredis mysqlclient)
else()
    message(WARNING "未找到 main_http.cpp！")
endif()

# 5. 构建客户端程序
file(GLOB CLIENT_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/client/*.cpp)
if (CLIENT_SOURCE_FILES)
    add_executable(client ${CLIENT_SOURCE_FILES})
    target_link_libraries(client httpserver_lib)
    target_link_libraries(client pthread hiredis mysqlclient)
else()
    message(WARNING "未找到客户端源文件！")
endif()
