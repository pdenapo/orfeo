project(Orpheus)
 
set(ORPHEUS_MAJOR_VERSION 1)
set(ORPHEUS_MINOR_VERSION 6)
set(ORPHEUS_PATCH_VERSION 0)
set(ORPHEUS_VERSION
  ${ORPHEUS_MAJOR_VERSION}.${ORPHEUS_MINOR_VERSION}.${ORPHEUS_PATCH_VERSION})

cmake_minimum_required(VERSION 3.0)

add_subdirectory(kkstrtext-0.1) 
add_subdirectory(kkconsui-0.1)
add_subdirectory(src)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/soundcard.h HAVE_SYS_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (soundcard.h HAVE_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/soundcard.h HAVE_SYS_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (soundcard.h HAVE_SOUNDCARD_H) 
CHECK_INCLUDE_FILES (machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)


# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

