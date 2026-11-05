cmake_minimum_required(VERSION 3.10)
project(VMTest)   

add_executable(VMTest VMTest.cpp)
target_link_libraries(VMTest PRIVATE RendererEngine)