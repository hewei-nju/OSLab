file1 = Fat12Reader
file2 = print
img = images/b
newImg = images/b

mount:
	@sudo mount $(img).img $(img)

newImg:
	@mkfs.fat -C $(newImg).img 1440

hex:
	@hexdump -C $(img)

compile:
	@nasm -f elf -o $(file2).o $(file2).asm -g


run:
	@make compile
	@g++ -m32 -g -o $(file1) $(file1).cpp $(file2).o -std=c++11
	@./$(file1) $(newImg).img


clean:
	rm $(file1) *.o
