cmake_minimum_required(VERSION 3.15)
project(lark_cpp_im LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# 依赖（vcpkg 工具链）
find_package(CURL CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(lark_cpp
  src/core/transport.cpp
  src/core/token_manager.cpp
  src/ws/frame_codec.cpp
  src/ws/ws_client.cpp
  src/im/im_service.cpp
)

target_include_directories(lark_cpp PUBLIC include)
target_link_libraries(lark_cpp PUBLIC CURL::libcurl Boost::boost Boost::system nlohmann_json::nlohmann_json)
target_compile_options(lark_cpp PUBLIC -UBOOST_ASIO_NO_DEPRECATED -UASIO_NO_DEPRECATED)

add_executable(lark_cpp_example examples/main.cpp)
target_link_libraries(lark_cpp_example PRIVATE lark_cpp)
