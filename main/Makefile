# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -I../include

# Source files
SRCS = main.cpp ../glad.c

# Output executable
OUT = my_program

# Libraries
LIBS = -lglfw -lGL

# Rules
all: $(OUT)

$(OUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
