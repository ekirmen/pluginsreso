cmake_minimum_required(VERSION 3.12)
project(UnMultiply VERSION 1.0.0 LANGUAGES CXX)

add_library(ffgl-plugin-unmultiply STATIC)
add_library(ffgl::plugin::unmultiply ALIAS ffgl-plugin-unmultiply)

target_sources(ffgl-plugin-unmultiply PRIVATE
    UnMultiply.cpp
    UnMultiply.h
)

target_link_libraries(ffgl-plugin-unmultiply PRIVATE
    ffgl::sdk
)

set_target_properties(ffgl-plugin-unmultiply PROPERTIES
    OUTPUT_NAME "UnMultiply"
    PREFIX ""
)
