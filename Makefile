SRC_DIR     = src
INC_DIR     = include

CPPFLAGS    =  -I$(INC_DIR)
CPPFLAGS   += `root-config --cflags`
LDFLAGS     = `root-config --libs --evelibs` 
CXXFLAGS    = -g -std=c++11 -Wall -fdiagnostics-color=always -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable -MMD -MP


LIBS = -lSkroot -latmpdroot  -lDataDefinition  -lidod_xtlk_root  -lloweroot  -lmcinfo  -lsofttrgroot  -ltqrealroot
LDFLAGS += -L/home/moflaher/skrootlibs $(LIBS)
CPPFLAGS += -I/home/moflaher/skrootlibs/include -I/home/moflaher/skrootlibs/include/include -I/home/moflaher/skrootlibs/include/managers

all: demo

demo: src/print_selections.cpp src/MTreeCut.cpp src/MTreeSelection.cpp src/MTreeReader.cpp src/Constants.cpp
	g++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o print_selections
