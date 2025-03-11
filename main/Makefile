# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -I../include

# Source files
SRCS = main.cpp ../glad.c ../stb_image.cpp

# Output executable
OUT = my_program

# Libraries
LIBS = -lglfw -lassimp -limgui

# Rules
all: $(OUT)

$(OUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
