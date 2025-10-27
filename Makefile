# Compiler
CXX = g++

# Compiler flags
# -std=c++17: Use the C++17 standard
# -O3:        Enable high-level optimizations
# -pthread:   Link with the pthreads library for std::thread support
# -Wall:      Enable all compiler warnings
# -MMD -MP:   Generate dependency files (for header changes)
CXXFLAGS = -std=c++17 -O3 -pthread -Wall -MMD -MP

# --- Project Structure ---

# Name of the final executables
TARGET = IndexGen
TEST_TARGET = Testing

# Source, Include, and Build directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Add the include directory to the compiler's search path
# This is the crucial part that lets #include "Candidates/file.hpp" work
CXXFLAGS += -I$(INCLUDE_DIR)

# --- Automatic File Discovery ---

# Find all .cpp files in SRC_DIR and its subdirectories (excluding testing.cpp)
SOURCES = $(filter-out $(SRC_DIR)/IndexGen.cpp $(SRC_DIR)/testing.cpp, $(shell find $(SRC_DIR) -name "*.cpp"))

# Main target sources (IndexGen + common sources)
MAIN_SOURCES = $(SRC_DIR)/IndexGen.cpp $(SOURCES)

# Testing target sources (testing.cpp + common sources)
# For testing, we only need LinearCodes and its dependencies
TEST_SOURCES = $(SRC_DIR)/testing.cpp \
               $(SRC_DIR)/Candidates/LinearCodes.cpp \
               $(SRC_DIR)/Candidates/GF4.cpp \
               $(SRC_DIR)/Candidates/GenMat.cpp \
               $(SRC_DIR)/Utils.cpp

# Create lists of object files in the BUILD_DIR
MAIN_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(MAIN_SOURCES))
TEST_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

# Create lists of dependency files
MAIN_DEPS = $(MAIN_OBJECTS:.o=.d)
TEST_DEPS = $(TEST_OBJECTS:.o=.d)

# --- Build Rules ---

# The default goal: build the main executable
.PHONY: all
all: $(TARGET)

# Alternative target: build the testing executable
.PHONY: testing
testing: $(TEST_TARGET)

# Rule to link the main executable from the object files
$(TARGET): $(MAIN_OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link the testing executable from the object files
$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "Linking $(TEST_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule to compile a .cpp from src/ into a .o in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(@D) # Create subdirectories in build/ (e.g., build/Candidates/)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# --- Cleanup ---

# 'make clean' will remove the entire build directory AND both executables
.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)/* $(TARGET) $(TEST_TARGET)

# --- Dependency Handling ---

# Include all the .d dependency files
# The '-' tells 'make' not to error if the file doesn't exist (e.g., on first build)
-include $(MAIN_DEPS)
-include $(TEST_DEPS)

# --- Help Target ---

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all       - Build the main IndexGen executable (default)"
	@echo "  testing   - Build the Testing executable for LinearCodes testing"
	@echo "  clean     - Remove all build artifacts and executables"
	@echo "  help      - Show this help message"
