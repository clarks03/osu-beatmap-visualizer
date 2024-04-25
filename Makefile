# Makefile for osu-parser

# Compiler
CXX := g++

# Compiler flags
CXXFLAGS := -Wall -Wextra -std=c++17

# Libraries
LIBS := -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lzip

# Source files
SRC := osu-parser.cpp

# Object Files
OBJ := $(SRC:.cpp=.o)

# Executable
EXEC := osu-parser

# Default rule
all: $(EXEC)

# Rule to compile source files into object fiels
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link object files into executable
$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# Clean rule
clean:
	rm -f $(OBJ) $(EXEC)