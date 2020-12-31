# remove when done

CC     = g++
CFLAGS = -DGPUCallableMethod

PEANO_HOME = /home/dominic/Documents/code/ExaHyPE2/Peano
INCLUDES = -I$(PEANO_HOME)/src/tarch/la

SOURCES = $(shell find . -name "*AoS.cpp")
OBJECTS = $(SOURCES:.cpp=.o)

$(OBJECTS): %.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

compile: $(OBJECTS)
	@echo "building"
	@echo $(SOURCES)
