# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(reaction_timer VERSION 1.0 DESCRIPTION "project" LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
add_compile_options(-Wall -Werror -Wpedantic -Wextra -g)
# add_compile_options(-fdiagnostics-color)
add_compile_options(-fno-diagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)
add_subdirectory(lgpio)
add_subdirectory(lcd)
