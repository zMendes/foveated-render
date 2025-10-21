# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Iinclude -Iexternal/include/

# Source files
INCLUDE_PUPIL = 0

SRCS = $(wildcard src/*.cpp) external/glad.c external/stb_image.cpp
ifeq ($(INCLUDE_PUPIL),0)
SRCS := $(filter-out src/pupil.cpp,$(SRCS))
endif

# Output executable
OUT = build/my_program

# Libraries
LIBS = -lglfw -lassimp -limgui -lonnxruntime -lmpi -lzmq -lavcodec -lavformat -lavutil -lswscale

# Rules
all: $(OUT)

$(OUT): $(SRCS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
