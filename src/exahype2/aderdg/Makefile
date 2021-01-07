# remove when done

CC     = g++
CFLAGS = -DGPUCallableMethod="" -DDimensions=2

PEANO_HOME = /home/dominic/Documents/code/ExaHyPE2/Peano
INCLUDES = -I$(PEANO_HOME)/src/

SOURCES = $(shell find . -name "*AoS.cpp")
SOURCES += KernelUtils.cpp Generic.cpp
OBJECTS = $(SOURCES:.cpp=.o)

compile: $(OBJECTS)
	@echo "building"
	@echo $(SOURCES)

$(OBJECTS): %.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS)
