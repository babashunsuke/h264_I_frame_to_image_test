CXX      := g++
CXXFLAGS := -O2 -Wall `pkg-config --cflags opencv4 gstreamer-1.0 gstreamer-app-1.0`
LDFLAGS  := `pkg-config --libs opencv4 gstreamer-1.0 gstreamer-app-1.0`

HEADERS  := videodecode.h
OBJECTS  := main.o videodecode.o
OUTPUT   := test

all: $(OUTPUT)

COMPILE := $(CXX) -O2 -Wall -I. $(CXXFLAGS)

$(OUTPUT): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.c.o:
	$(COMPILE) -c $<

.cpp.o:
	$(COMPILE) -c $<

clean:
	\rm -f $(OBJECTS) $(OUTPUT)


