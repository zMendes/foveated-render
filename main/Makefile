# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -I../include

# Source files
SRCS = main.cpp gaze_data.cpp constants.cpp utils.cpp foveation.cpp gaze_predictor.cpp ../glad.c ../stb_image.cpp

# Output executable
OUT = my_program

# Libraries
LIBS = -lglfw -lassimp -limgui -lonnxruntime -lmpi

# Rules
all: $(OUT)

$(OUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
