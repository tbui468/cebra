cmake_minimum_required(VERSION ${CMAKE_VERSION})
project(Cebra VERSION 1.0.0 LANGUAGES C)

if(MSVC)
  add_compile_options(/W4 /permissive /w14640)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_C_STANDARD 99)

add_subdirectory(src)
