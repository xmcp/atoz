.DEFAULT_GOAL := compiler

CXX_OPTIONS = -lm -std=c++11

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
