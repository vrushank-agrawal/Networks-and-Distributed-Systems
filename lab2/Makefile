CXX             = g++
LD              = g++

CXXFLAGS        = -Wall -std=c++20 -Iheaders -D_GNU_SOURCE
LDFLAGS         = -Wall -std=c++20

CPPFILES        = $(wildcard *.cpp)
MAINOBJS        = $(CPPFILES:.cpp=.o)
ALLEXEC         = clean main

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(ALLEXEC)

main: $(MAINOBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAINOBJS)

clean:
	rm -f *.o $(ALLEXEC)
