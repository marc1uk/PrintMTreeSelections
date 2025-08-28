SRC_DIR     = src
INC_DIR     = include

CPPFLAGS    =  -I$(INC_DIR)
CPPFLAGS   += `root-config --cflags`
LDFLAGS     = `root-config --libs --evelibs` 
CXXFLAGS    = -g -std=c++11 -Wall -fdiagnostics-color=always -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable -MMD -MP


LIBS = -lSkroot -latmpdroot  -lDataDefinition  -lidod_xtlk_root  -lloweroot  -lmcinfo  -lsofttrgroot  -ltqrealroot
LDFLAGS += -L/disk03/lowe12/warwick/skrootlibs $(LIBS)
CPPFLAGS += -I/disk03/lowe12/warwick/skrootlibs/include -I/disk03/lowe12/warwick/skrootlibs/include/include -I/disk03/lowe12/warwick/skrootlibs/include/managers

all: print_cuts draw_distros

print_cuts: src/print_selections.cpp src/MTreeCut.cpp src/MTreeSelection.cpp src/MTreeReader.cpp src/Constants.cpp
	g++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o print_selections

draw_distros: src/draw_distributions.cpp
	g++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o draw_distributions
