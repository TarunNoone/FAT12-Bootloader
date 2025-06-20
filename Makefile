.PHONY: all

all: bin/floppy.img
	# Build all

reader: bin/fat_12_disk_reader
	bin/fat_12_disk_reader bin/floppy.img

bin/floppy.img: src/main.asm
	mkdir -p bin/
	
	# Create the Bootloader Binary
	nasm src/main.asm -f bin -o bin/bootloader.bin
	
	# Create a text file
	echo "Lorem ipsum dolar sit amet" > bin/lore.txt
	
	# Create a 1440kB file for floppy disk image	
	dd if=/dev/zero of=bin/floppy.img bs=512 count=2880
	
	# Format the disk image as FAT-12 File System
	mkfs.fat -F 12 -n "HELLO DRIVE" bin/floppy.img
	
	# Copy a file into the FAT-12 File System
	mcopy -i bin/floppy.img bin/lore.txt "::lore.txt"
	
	# Copy the bootloader to the first 512 Bytes of Floppy Disk Image
	dd if=bin/bootloader.bin of=bin/floppy.img conv=notrunc	

bin/fat_12_disk_reader: src/fat_12_disk_reader.c

	# Compile the fat_12_disk_reader
	gcc -o bin/fat_12_disk_reader src/fat_12_disk_reader.c

clean:
	rm -r bin/

