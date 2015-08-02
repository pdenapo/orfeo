
cmake_minimum_required(VERSION 3.0)

include_directories(include ../kkstrtext-0.1)

add_library(kkconsui src/abstractui.cc src/cmenus.cc src/colorschemer.cc 
	    src/conscommon.cc  src/dialogbox.cc src/fileselector.cc 
	    src/horizontalbar.cc src/linkedlist.cc 
	    src/screenarea.cc src/textbrowser.cc src/texteditor.cc 
	    src/textinputline.cc src/textwindow.cc src/treeview.cc 
	    include/abstractui.h include/cmenus.h
	    include/colorschemer.h include/conf.h include/conscommon.h
	    include/dialogbox.h include/fileselector.h
	    include/horizontalbar.h include/linkedlist.h
	    include/screenarea.h include/textbrowser.h
	    include/texteditor.h include/textinputline.h
	    include/textwindow.h include/treeview.h
)

