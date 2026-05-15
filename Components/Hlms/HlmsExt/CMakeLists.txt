#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Ogre HLMS Extended implementation for OpenGL 3.3+ and D3D11 build

PROJECT(${OGRE_NEXT}HlmsExt)

FIND_PACKAGE(unofficial-shaderc CONFIG REQUIRED)
FIND_PACKAGE(spirv_cross_core CONFIG REQUIRED)
FIND_PACKAGE(spirv_cross_glsl CONFIG REQUIRED)

file(
    GLOB HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(
    GLOB SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

add_definitions( -DOgreHlmsExt_EXPORTS )

ogre_add_library(${OGRE_NEXT}HlmsExt ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}HlmsExt PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}HlmsExt
	${OGRE_NEXT}Main
	unofficial::shaderc::shaderc
	spirv-cross-core
	spirv-cross-glsl
)

ogre_config_framework(${OGRE_NEXT}HlmsExt)
ogre_config_component(${OGRE_NEXT}HlmsExt)

# set c++17 standard to use modern features from std
target_compile_features(${OGRE_NEXT}HlmsExt PRIVATE cxx_std_17)

install (DIRECTORY include/
        DESTINATION include/${OGRE_NEXT_PREFIX}/Hlms/HlmsExt
        FILES_MATCHING PATTERN "*.h"
        )
