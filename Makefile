.DEFAULT_GOAL := compiler

CXX_OPTIONS = -g -lm -std=c++11 -Wno-reorder -Wno-format-zero-length -Wno-unused -Wno-return-type
#CXX_OPTIONS = -lm -Wall -Wextra -Wno-reorder -Wno-format-zero-length -Wno-unused -Wno-return-type -Ofast -std=c++17 -fstack-protector-all -ftrapv -fsanitize=address -fsanitize=undefined

sysy.tab:
	mkdir -p build/front
	yacc -d -o build/front/sysy.tab.cpp src/front/sysy.y

lex.yy.cpp: sysy.tab
	mkdir -p build/front
	lex --noline -o build/front/lex.yy.cpp src/front/sysy.l

srcfiles:
	cp -t build -R src/*

compiler: srcfiles sysy.tab lex.yy.cpp
	g++ ${CXX_OPTIONS} -o build/compiler -Ibuild/front -Ibuild/back $$(find build -type f -iname *.cpp)

clean:
	rm -rf build
	mkdir build