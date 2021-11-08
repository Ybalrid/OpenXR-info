cmake_minimum_required(VERSION 3.19)
find_package(Vulkan)
project(OpenXR-info)
file(GLOB project-sources CONFIGURE_DEPENDS src/*)
add_subdirectory(${CMAKE_SOURCE_DIR}/OpenXR-SDK)
add_executable(OpenXR-info ${project-sources})

target_link_libraries(OpenXR-info PRIVATE openxr_loader)

target_include_directories(OpenXR-info PRIVATE
	${CMAKE_SOURCE_DIR}/json/include
	${CMAKE_SOURCE_DIR}/XREW/generated/
	${Vulkan_INCLUDE_DIRS}
)
