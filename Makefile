CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET = code
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
