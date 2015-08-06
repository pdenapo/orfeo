
cmake_minimum_required(VERSION 3.0)

include_directories(abstract . ../kkstrtext-0.1 ../kkconsui-0.1/include ${PROJECT_BINARY_DIR} )

add_executable(orpheus  cdtrack.cc cdtrack.h main.cc mixerctl.cc
	       mixerctl.h mp3track.cc mp3track.h oggtrack.cc
	       oggtrack.h orpheusconf.cc orpheusconf.h playlist.cc 
	       playlist.h streamtrack.cc streamtrack.h uitext.cc 
	       uitext.h vcedit.c vcedit.h abstract/track.h
	       abstract/userinterface.h flactrack.h flactrack.cc)

find_package(Curses)

set (EXTRA_LIBS ${EXTRA_LIBS} kkconsui kkstrtext ${CURSES_LIBRARIES} )

target_link_libraries (orpheus  ${EXTRA_LIBS})

install (TARGETS orpheus DESTINATION bin) 