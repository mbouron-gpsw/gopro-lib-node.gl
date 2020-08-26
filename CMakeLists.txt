cmake_minimum_required(VERSION 3.10)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option (GRAPHICS_BACKEND_METAL "build metal backend" OFF)
option (GRAPHICS_BACKEND_DIRECT3D12 "build directx12 backend" OFF)
option (GRAPHICS_BACKEND_VULKAN "build vulkan backend" OFF)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include_directories(external/win64/include)
    link_directories(external/win64/lib)
endif()

include_directories(ngfx ngfx/src)

function(build_test name)
file (GLOB_RECURSE TEST_SOURCE_FILES test/${name}/*.cpp test/${name}/*.h test/${name}/*.mm)
if (GRAPHICS_BACKEND_METAL)
add_executable(${name} ${TEST_SOURCE_FILES} ${APP_BACKEND_SOURCE_FILES} ngfx/porting/appkit/Main.storyboard)
set(RESOURCE_FILES ngfx/porting/appkit/Info.plist ngfx/porting/appkit/Main.storyboard)
set_target_properties(${name} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_FRAMEWORK_IDENTIFIER org.gopro.ngfx
    RESOURCE "${RESOURCE_FILES}"
    MACOSX_BUNDLE_INFO_PLIST ngfx/porting/appkit/Info.plist
)
else()
add_executable(${name} ${TEST_SOURCE_FILES})
endif()
target_link_libraries(${name} ngfx)
endfunction()

add_subdirectory(ngfx)
add_subdirectory(nodegl)
