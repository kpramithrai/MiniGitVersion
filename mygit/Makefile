# The compiler we are using
CXX = g++

# Compiler flags: C++17 standard and warnings
CXXFLAGS = -std=c++17 -Wall

# Libraries to link against
LDLIBS = -lssl -lcrypto

# The name of the final executable file
TARGET = mygit

# The rule to build our target
$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(LDLIBS)

# A rule to clean up generated files
clean:
	rm -f $(TARGET)