CXX = g++ --std=c++17 # Specify your C++ compiler here (if different)
CXXFLAGS = -g  # Compiler flags (warnings, debugging symbols)

# Object files
lexer.o: src/lexer/lexer.cpp src/lexer/lexer.h
	$(CXX) -c src/lexer/lexer.cpp

parser.o: src/parser/parser.cpp src/parser/parser.h src/lexer/lexer.h  # Dependency on lexer.h
	$(CXX) -c src/parser/parser.cpp

# Executable
my_program: lexer.o parser.o
	$(CXX) -o my_program lexer.o parser.o

# Clean up object files
clean: 
	rm -f *.o

