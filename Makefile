all:
	nasm -o shellcode shellcode.asm
	nasm -o exit exit.asm
	nasm -o exit32 exit32.asm
	gcc -zexecstack -fno-stack-protector -o test test.c
	gcc -o papa-shango papa-shango.c

clean:
	rm -f papa-shango shellcode test exit exit32 *.o *~

