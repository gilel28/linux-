CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -fPIC
INCLUDES = -I.

LIBNAME = functions

LIB_SRC := $(filter-out main%.cpp, $(wildcard *.cpp))
LIB_OBJ := $(LIB_SRC:.cpp=.o)
LIB_SO := lib$(LIBNAME).so

MAINS := $(wildcard main*.cpp)
BINARIES := $(MAINS:.cpp=.out)

LDFLAGS = -L. -l$(LIBNAME) -Wl,-rpath='$$ORIGIN'

all: $(LIB_SO) $(BINARIES)

$(LIB_SO): $(LIB_OBJ)
	$(CXX) -shared -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.out: %.cpp $(LIB_SO)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf build *.o *.out *.so

