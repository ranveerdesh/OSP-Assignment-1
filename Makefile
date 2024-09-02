# Compiler
CXX = g++
CXXFLAGS = -std=c++1y -Wall -Werror -Ipthread

# Targets
TARGETS = mmcopier mscopier

# Source files
SRC_MM = mmcopier.cpp
SRC_MS = mscopier.cpp

# Object files
OBJ_MM = $(SRC_MM:.cpp=.o)
OBJ_MS = $(SRC_MS:.cpp=.o)

# Default target: build all
all: $(TARGETS)

# Build mmcopier
mmcopier: $(OBJ_MM)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build mscopier
mscopier: $(OBJ_MS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean up
clean:
	rm -f $(OBJ_MM) $(OBJ_MS) $(TARGETS)

# Pattern rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all clean
