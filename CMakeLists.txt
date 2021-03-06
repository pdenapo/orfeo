project(Orfeo)
 
set(ORFEO_MAJOR_VERSION 1)
set(ORFEO_MINOR_VERSION 6)
set(ORFEO_PATCH_VERSION 0)
set(ORFEO_VERSION
  ${ORFEO_MAJOR_VERSION}.${ORFEO_MINOR_VERSION}.${ORFEO_PATCH_VERSION})

cmake_minimum_required(VERSION 3.0)

add_subdirectory(kkstrtext-0.1) 
add_subdirectory(kkconsui-0.1)
add_subdirectory(src)
add_subdirectory(po)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/soundcard.h HAVE_SYS_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (soundcard.h HAVE_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/soundcard.h HAVE_SYS_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (soundcard.h HAVE_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)

find_package(LibXml2)
IF (LIBXML2_FOUND)
 SET(HAVE_LIBXML2 1)
ENDIF (LIBXML2_FOUND)



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
message(STATUS "CMAKE_MODULE_PATH is" ${CMAKE_MODULE_PATH})
find_package(Vorbis)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

