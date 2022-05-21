CXX_FLAGS = -fsanitize=address -Wall -g -std=c++11

main: *.o
	@echo you can turn off the DEBUG definition in src/stock.cpp to disable messages
	g++ $^ -o $@ $(CXX_FLAGS)

%.o: %.cpp
	g++ -c $^ $(CXX_FLAGS)

clean:
	rm -rf *.o main
