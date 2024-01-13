cmake_minimum_required(VERSION 3.27.4)
include(FetchContent)

project(dreams)

include(CPack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Freetype REQUIRED)

file(GLOB_RECURSE sources src/*.cpp src/*.h)
file(GLOB_RECURSE opt_sources opt/*.cpp opt/*.h)
file(GLOB_RECURSE shaders shaders/*.vert shaders/*.frag)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.12.2
)
FetchContent_Declare(
  libspng
  GIT_REPOSITORY https://github.com/randy408/libspng.git
  GIT_TAG        v0.7.4
)
FetchContent_Declare(
  VulkanMemoryAllocator-Hpp
  GIT_REPOSITORY https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(entt libspng VulkanMemoryAllocator-Hpp)

target_compile_options(spng PRIVATE -O3)

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_link_libraries(optimized_components PRIVATE VulkanMemoryAllocator-Hpp)
target_compile_options(optimized_components PRIVATE -O3)

add_executable(dreams ${sources})

target_include_directories(dreams PRIVATE include/)

target_link_libraries(dreams PRIVATE Vulkan::Vulkan)
target_link_libraries(dreams PRIVATE VulkanMemoryAllocator-Hpp)
target_link_libraries(dreams PRIVATE glfw)
target_link_libraries(dreams PRIVATE spdlog::spdlog)
target_link_libraries(dreams PRIVATE EnTT::EnTT)
target_link_libraries(dreams PRIVATE dl)
target_link_libraries(dreams PRIVATE spng)
target_link_libraries(dreams PRIVATE Freetype::Freetype)
target_link_libraries(dreams PRIVATE optimized_components)

target_include_directories(dreams PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(dreams PRIVATE ${GLM_INCLUDE_DIRS})

find_program(GLSLC glslc)
find_program(GLSLang_Validator glslangValidator)
if(NOT GLSLC AND NOT GLSLang_Validator)
  message(FATAL_ERROR "No GLSL compiler found!")
endif()

if(GLSLang_Validator)
  set(GLSL_COMPILER ${GLSLang_Validator})
  set(GLSL_COMPILER_FLAGS "-V")
else()
  set(GLSL_COMPILER ${GLSLC})
  set(GLSL_COMPILER_FLAGS "")
endif()
message(STATUS "Found GLSL compiler: ${GLSL_COMPILER}")

add_subdirectory(shaders/)
add_subdirectory(assets/)

add_dependencies(dreams shaders)

install(TARGETS dreams DESTINATION ${CMAKE_INSTALL_BINDIR})
