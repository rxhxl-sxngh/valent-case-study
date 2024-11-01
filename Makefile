CXX = g++
CXXFLAGS = -std=c++17 -Wall
TARGET = steel_calculator
SRCS = steel.cpp
HEADERS = include/csv_utils.hpp

.PHONY: all clean cleanoutput

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean: cleanoutput
	rm -f $(TARGET)

cleanoutput:
	rm -f output/*