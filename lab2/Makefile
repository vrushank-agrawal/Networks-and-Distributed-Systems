CXX             = g++
LD              = g++

CXXFLAGS        = -Wall -std=c++23 -Iheaders -D_GNU_SOURCE
LDFLAGS         = -Wall -std=c++23

CPPFILES        = $(wildcard *.cpp)
MAINOBJS        = $(CPPFILES:.cpp=.o)
ALLEXEC         = main

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(ALLEXEC)

main: $(MAINOBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAINOBJS)

clean:
	rm -f *.o $(ALLEXEC)
