cmake_minimum_required(VERSION 3.20)

project(rzstmcfg LANGUAGES CXX)


add_executable(rzstmcfg)
target_compile_features(rzstmcfg PRIVATE cxx_std_23)
target_sources(rzstmcfg PRIVATE
    src/main.cpp
)
target_include_directories(rzstmcfg PRIVATE
    libs
)