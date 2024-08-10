cmake_minimum_required(VERSION 3.28)
project(mlp)

set(CMAKE_CXX_STANDARD 17)

add_executable(mlp-cli src/cli/main.cpp)

target_include_directories(mlp-cli PUBLIC src src/cli src/mlp extern)

target_include_directories(mlp-cli PUBLIC /opt/local/include)
target_include_directories(mlp-cli PUBLIC /usr/local/include)
target_link_directories(mlp-cli PUBLIC /opt/local/lib)
target_link_directories(mlp-cli PUBLIC /usr/local/lib)
target_link_libraries(mlp-cli PUBLIC lo rtaudio)