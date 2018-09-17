DEFINES=
INCLUDES=
DEBUGOPTS=-g
WARNOPTS=-Wall
OPTOPTS=-O0
MISCOPTS=-Wwritable-strings

CXXFLAGS=--std=c++14 $(DEFINES) $(INCLUDES) $(DEBUGOPTS) $(WARNOPTS) $(OPTOPTS) $(MISCOPTS)

USE_GCC=0
ifeq (0,$(USE_GCC))
CC=clang
C++=clang++
CXX=clang++
LD=clang++
else
CC=g++
C++=g++
CXX=g++
LD=g++
endif

all: sttr1

sttr1: sttr1.o
	$(CXX) -o sttr1 $^

sttr1.o: sttr1.cpp
	$(CXX) -c $(CXXFLAGS) $^

clean:
	rm -f sttr1 sttr1.o
