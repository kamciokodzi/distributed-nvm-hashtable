cmake_minimum_required (VERSION 3.5)

list (APPEND CMAKE_MODULE_PATH /seastar/cmake)    #c-ares.cmake folder location
set(fmt_DIR "/seastar/build/release/_cooking/stow/fmt/lib64/cmake/fmt") #fmt-congif.cmake folder location
set(Seastar_DIR "/seastar/build/release") #SeastarConfig.cmake and libseastar.a folder location

project (SeastarExample)

find_package (Seastar REQUIRED)

add_executable (example
  main.cc)

target_link_libraries (example
  PRIVATE Seastar::seastar)