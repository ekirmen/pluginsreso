cmake_minimum_required(VERSION 3.12)
project(AutoMaskPro VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/W3 /MP)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

find_package(ffgl_sdk REQUIRED)

add_library(ffgl-plugin-unmultiply STATIC)
add_library(ffgl::plugin::unmultiply ALIAS ffgl-plugin-unmultiply)

target_sources(ffgl-plugin-unmultiply PRIVATE 
    AutoMaskPro.h 
    AutoMaskPro.cpp
)

target_include_directories(ffgl-plugin-unmultiply PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ffgl-plugin-unmultiply PRIVATE 
    ffgl::sdk
)

if(WIN32)
    target_compile_definitions(ffgl-plugin-unmultiply PRIVATE
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(ffgl-plugin-unmultiply PROPERTIES
    OUTPUT_NAME "AutoMaskPro"
    PREFIX "lib"
)

install(
    TARGETS     ffgl-plugin-unmultiply
    EXPORT      ffgl-plugin-unmultiply-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES       AutoMaskPro.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/AutoMaskPro
)