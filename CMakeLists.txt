cmake_minimum_required(VERSION 3.10)

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

add_subdirectory(external/entt)
add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/libspng)
target_compile_options(spng PRIVATE -O3)

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_include_directories(optimized_components PRIVATE external/VulkanMemoryAllocator-Hpp/)
target_compile_options(optimized_components PRIVATE -O3)

add_executable(dreams ${sources})

target_include_directories(dreams PRIVATE include/)

target_link_libraries(dreams PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(dreams PRIVATE VulkanMemoryAllocator)
target_link_libraries(dreams PRIVATE glfw)
target_link_libraries(dreams PRIVATE spdlog::spdlog)
target_link_libraries(dreams PRIVATE EnTT::EnTT)
target_link_libraries(dreams PRIVATE dl)
target_link_libraries(dreams PRIVATE spng)
target_link_libraries(dreams PRIVATE Freetype::Freetype)
target_link_libraries(dreams PRIVATE optimized_components)

target_include_directories(dreams PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(dreams PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(dreams PRIVATE external/VulkanMemoryAllocator-Hpp/)
target_include_directories(dreams PRIVATE external/libspng/spng)

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
