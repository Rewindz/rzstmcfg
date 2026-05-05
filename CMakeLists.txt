cmake_minimum_required(VERSION 3.24)

project(rzstmcfg LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7
    GIT_SHALLOW TRUE
)

FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip
)

FetchContent_MakeAvailable(imgui glfw json)

find_package(OpenGL REQUIRED)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(libimgui STATIC)
target_sources(libimgui PRIVATE ${IMGUI_SOURCES})
target_include_directories(libimgui PUBLIC ${imgui_SOURCE_DIR} ${glfw_SOURCE_DIR}/include)


add_executable(rzstmcfg WIN32)
target_compile_features(rzstmcfg PRIVATE cxx_std_23)
target_link_libraries(rzstmcfg PRIVATE
 libimgui
 glfw
 OpenGL::GL
)
target_sources(rzstmcfg PRIVATE
    src/main.cpp
)
target_include_directories(rzstmcfg PRIVATE
    libs
    ${json_SOURCE_DIR}/single_include
)