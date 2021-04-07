.DEFAULT_GOAL := compiler

CXX_OPTIONS = -lm -std=c++11

sysy.tab:
	yacc -d -v -o build/sysy.tab.cpp src/sysy.y

lex.yy.cpp: sysy.tab
	lex --noline -o build/lex.yy.cpp src/sysy.l

srcfiles:
	cp -t build src/*.cpp

compiler: srcfiles sysy.tab lex.yy.cpp
	g++ ${CXX_OPTIONS} -o build/compiler -Isrc build/*.cpp

clean:
	rm build/*
