ircompile:
	g++ ircompile.cpp ir.cpp -o ircompile.out

bfopt:
	g++ bfopt.cpp -o bfopt.out

biffc:
	g++ biffc.cpp lexer.cpp -o biffc.out
