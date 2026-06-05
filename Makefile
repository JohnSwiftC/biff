ircompile:
	g++ ircompile.cpp ir.cpp -Wall -Werror -o ircompile.out

bfopt:
	g++ bfopt.cpp -Wall -Werror -o bfopt.out

biffc:
	g++ biffc.cpp lexer.cpp -Wall -Werror -o biffc.out
