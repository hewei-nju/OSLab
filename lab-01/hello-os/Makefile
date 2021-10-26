# 初始化实验环境
init:
	./build_lab_enviroment.sh


# 将boot.asm编译为boot.bin, 然后将boot.asm编译为纯二进制文件
build:
	nasm boot.asm -o boot.bin
	nasm -f bin boot.asm


# 将boot.bin文件内容写入软盘a.img中
# 进行此命令前已经完成软盘a.img的构建
write:
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc


# 启动
# 进行此命令前，已经完成bochsrc配置文件的编写
run:
	bochs -f bochsrc


# 清除boot.bin文件
clean:
	rm boot.bin
