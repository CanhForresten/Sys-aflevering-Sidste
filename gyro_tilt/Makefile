CXX := g++

CXXFLAGS := -Wall -g -O2

SRC := src/gyro_tilt.cpp
INC := inc
OBJ := $(SRC:.cpp=.o)
BUILD_PATH = build/bin


TARGET := gyro_tilt


all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@
	mkdir build && mkdir build/bin
	mv src/*.o $(BUILD_PATH)
	mv $(TARGET) $(BUILD_PATH)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(INC) -c $< -o $@

clean:
	rm -r build
