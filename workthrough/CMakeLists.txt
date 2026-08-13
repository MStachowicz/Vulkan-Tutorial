cmake_minimum_required(VERSION 3.29)

project(VulkanTutorialWorkthrough LANGUAGES CXX)

add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/../attachments"
  "${CMAKE_BINARY_DIR}/official"
)

find_package(glfw3 REQUIRED)

add_executable(tutorial_workthrough main.cpp)
set_target_properties(tutorial_workthrough PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/workthrough"
)
target_link_libraries(tutorial_workthrough PRIVATE Vulkan::cppm glfw)
target_compile_definitions(tutorial_workthrough PRIVATE
  VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)

if(ENABLE_CPP20_MODULE)
  target_compile_definitions(tutorial_workthrough PRIVATE USE_CPP20_MODULES=1)
endif()