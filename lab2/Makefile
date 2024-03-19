CC				= gcc
LD_C			= gcc
CXX             = g++
LD              = g++

CFLAGS			= -Wall -std=c11 -D_GNU_SOURCE
LD_CFLAGS		= -Wall -std=c11
CXXFLAGS        = -Wall -std=c++20 -Iheaders -D_GNU_SOURCE
LDFLAGS         = -Wall -std=c++20

CFILES 			= $(wildcard *.c)
CPPFILES        = $(wildcard *.cpp)
MAINOBJS        = $(CPPFILES:.cpp=.o)
COBJS			= $(CFILES:.c=.o)
ALLEXEC         = clean process stopall

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(ALLEXEC)

stopall: $(COBJS)
	$(LD_C) $(LD_CFLAGS) -o $@ $(COBJS)


process: $(MAINOBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAINOBJS)

clean:
	rm -f *.o $(ALLEXEC)
