lexer:
	g++ --std=c++17 src/lexer/lexer.cpp -o bin/lexer.out

parser:
	g++ --std=c++17 src/lexer/lexer.cpp -o bin/parser.out
clean:
	rm -f ./bin/**.out
