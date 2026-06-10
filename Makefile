all: biffc ircompile bfopt

biffc:
	g++ biffc.cpp lexer.cpp ast.cpp parser.cpp codegen.cpp compiler.cpp -Wall -o biffc.out

ircompile:
	g++ ircompile.cpp ir.cpp -o ircompile.out

bfopt:
	g++ bfopt.cpp -Wall -Werror -o bfopt.out
