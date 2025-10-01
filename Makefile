CXX = g++
CXXFLAGS = -std=c++14 -O2
TARGET = code
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
