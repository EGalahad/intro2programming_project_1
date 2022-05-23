CXX_FLAGS := -fsanitize=address -Wall -g -std=c++11
DEBUG_FLAGS := -DDEBUG -DDEBUG_BUY -DDEBUG_MEMORY

main: *.o
	@echo ========== you can uncomment the DEBUG definitions in src/stock.h to enable messages
	@echo ========== or you can enable it here in Makefile
	g++ $^ -o $@ $(CXX_FLAGS) $(DEBUG_FLAGS)

# stock.o: stock.cpp stock.h
# 	g++ -c $< $(CXX_FLAGS) $(DEBUG_FLAGS)

%.o: %.cpp
	g++ -c $^ $(CXX_FLAGS) $(DEBUG_FLAGS)

clean:
	rm -rf *.o main
