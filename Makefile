.DEFAULT_GOAL := compiler

sysy.tab.cpp sysy.tab.hpp:
	yacc --no-lines -d -o build/sysy.tab.cpp src/sysy.y

lex.yy.cpp: sysy.tab.hpp
	lex --noline -o build/lex.yy.cpp src/sysy.l

srcfiles:
	cp -t build src/*.cpp

compiler: srcfiles sysy.tab.cpp lex.yy.cpp
	g++ -Og -lm -std=c++11 -o build/compiler -Isrc build/*.cpp