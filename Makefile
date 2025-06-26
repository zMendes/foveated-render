# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Iinclude -Iexternal/include/

# Source files
SRCS = $(wildcard src/*.cpp) external/glad.c external/stb_image.cpp

# Output executable
OUT = build/my_program

# Libraries
LIBS = -lglfw -lassimp -limgui -lonnxruntime -lmpi

# Rules
all: $(OUT)

$(OUT): $(SRCS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
