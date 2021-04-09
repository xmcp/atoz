.DEFAULT_GOAL := compiler

#CXX_OPTIONS = -g -lm -std=c++11
CXX_OPTIONS = -Wall -Wextra -Wno-reorder -Wno-unused -Ofast -lm -std=c++17 -fstack-protector-all -ftrapv -fsanitize=address -fsanitize=undefined

sysy.tab:
	yacc -d -v -o build/sysy.tab.cpp src/sysy.y

lex.yy.cpp: sysy.tab
	lex --noline -o build/lex.yy.cpp src/sysy.l

srcfiles:
	cp -t build src/*.cpp
	cp -t build src/*.hpp

compiler: srcfiles sysy.tab lex.yy.cpp
	g++ ${CXX_OPTIONS} -o build/compiler -Ibuild build/*.cpp

clean:
	rm build/*
