CXX = g++ -std=c++11
TARGET = mmu
OBJ = main.o

CXXFLAGS = -c -Wall

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
.PHONY: clean
clean:
	rm -rf *.o $(TARGET)