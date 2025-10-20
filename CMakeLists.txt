cmake_minimum_required(VERSION 3.18)


if (DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Path to vcpkg CMake toolchain file")
  message(STATUS "Using CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
  message(STATUS "Using VCPKG_ROOT: ${VCPKG_ROOT}")
else()
  message(WARNING "VCPKG_ROOT environment variable not set")
endif()


project(renderer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
  
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(Eigen3 NO_MODULE REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-skia CONFIG REQUIRED)


pybind11_add_module(renderer
  src/bindings.cpp
  src/render.cpp
  src/path.cpp
  src/shaper.cpp
  src/lib_opts/freetype.cpp
  src/lib_opts/skia.cpp
)

target_link_libraries(renderer
  PRIVATE
    Freetype::Freetype
    harfbuzz::harfbuzz
    Eigen3::Eigen
    fmt::fmt
    unofficial::skia::skia
) 
