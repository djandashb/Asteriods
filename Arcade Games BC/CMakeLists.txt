cmake_minimum_required(VERSION 3.24)

project(RetroArcadeCollection VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED)
find_package(SDL3_image REQUIRED CONFIG REQUIRED)

add_executable(retro_arcade
    src/Game.cpp
    src/main.cpp
)

target_include_directories(retro_arcade PRIVATE src)
target_link_libraries(retro_arcade PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

if(APPLE)
    target_include_directories(retro_arcade SYSTEM PRIVATE
        "${CMAKE_OSX_SYSROOT}/usr/include/c++/v1"
    )
    set_target_properties(retro_arcade PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

add_custom_command(TARGET retro_arcade POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:retro_arcade>/assets
)
