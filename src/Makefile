CXX_FLAGS = -fsanitize=address -Wall -g -std=c++11

main: *.o
	g++ $^ -o $@ $(CXX_FLAGS)

%.o: %.cpp
	g++ -c $^ $(CXX_FLAGS)

clean:
	rm -rf *.o main
