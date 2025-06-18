main.out: main.o c_func.o cpp_func.o asm_utils.o
	g++ -o main.out main.o c_func.o cpp_func.o asm_utils.o -m32

c_func.o: c_func.c
	gcc -o c_func.o -m32 -c c_func.c

cpp_func.o: cpp_func.cpp
	g++ -o cpp_func.o -m32 -c cpp_func.cpp 

main.o: main.cpp
	g++ -o main.o -m32 -c main.cpp

asm_utils.o: asm_utils.asm
	nasm -o asm_utils.o -f elf32 asm_utils.asm
clean:
	rm *.o

run: main.out
	./main.out