filename = cal

compile:
	nasm -f elf $(filename).asm

link:
	ld -m elf_i386 $(filename).o -o $(filename)

build:
	make compile
	make link

run:
	./$(filename)

go:
	make build && make run

debug:
	nasm -f	elf -g $(filename).asm
	make link
	gdb $(filename)

clean:
	rm $(filename)
