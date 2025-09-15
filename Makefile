# Compiler
CXX = g++

# Compiler flags
# -std=c++11: Use the C++11 standard
# -O3:        Enable high-level optimizations
# -pthread:   Link with the pthreads library for std::thread support
# -Wall:      Enable all compiler warnings (good practice)
CXXFLAGS = -std=c++11 -O3 -pthread -Wall

# Name of the final executable
TARGET = dna_code_generator.exe

# Find all .cpp files in the current directory
SOURCES = $(wildcard *.cpp)

# Generate corresponding object file names (e.g., main.cpp -> main.o)
OBJECTS = $(SOURCES:.cpp=.o)

# The default goal: build the executable
all: $(TARGET)

# Rule to link the executable from the object files
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to compile a .cpp source file into a .o object file
# This is a pattern rule that works for all your .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Rule to clean up the directory
clean:
	rm -f $(OBJECTS) $(TARGET)

# Declare 'all' and 'clean' as phony targets since they are not actual files
.PHONY: all clean