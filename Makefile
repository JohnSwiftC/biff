ircompile:
	g++ ircompile.cpp ir.cpp -o ircompile.out

bfopt:
	g++ bfopt.cpp -Wall -Werror -o bfopt.out

biffc:
	g++ biffc.cpp lexer.cpp parse.cpp -Wall -o biffc.out
