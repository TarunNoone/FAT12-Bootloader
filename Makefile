.PHONY: all

all: bin/floppy.img
	# Build all

reader: bin/fat_12_disk_reader
	bin/fat_12_disk_reader bin/floppy.img

profiler: bin/fat_12_disk_reader
	/usr/bin/time -v bin/fat_12_disk_reader bin/floppy.img

bin/floppy.img: src/main.asm
	mkdir -p bin/
	
	# Create the Bootloader Binary
	nasm src/main.asm -f bin -o bin/bootloader.bin
	
	# Create a 1440kB file for floppy disk image	
	dd if=/dev/zero of=bin/floppy.img bs=512 count=2880
	
	# Format the disk image as FAT-12 File System
	mkfs.fat -F 12 -n "HELLO DRIVE" bin/floppy.img
	
	# Copy a file into the FAT-12 File System

	# Trying to make a long file for to check Long File Name entries.
	# Fun fact: There's an order for adjectives.
	# It's abbreviated to OSASCOMP : Opinion, Size, Age, Shape, Color, Material, Origin, Type

	# mcopy -i bin/floppy.img src/lore.txt "::lore.txt"
	# mcopy -i bin/floppy.img src/lore.txt "::legendary_colossal_archaic_spherical_crimson_draconian_obsidian_tome_lore.txt"
	# mcopy -i bin/floppy.img src/lore512.txt "::lore512.txt"
	# mcopy -i bin/floppy.img src/lore1024.txt "::lore1024.txt"
	mcopy -i bin/floppy.img -s src/myfolder "::/"

	# Copy multiple copies to fill the floppy disk image. Just for testing.
	# ./src/multiple_copies.sh
	
	# Copy the bootloader to the first 512 Bytes of Floppy Disk Image
	dd if=bin/bootloader.bin of=bin/floppy.img conv=notrunc	

bin/fat_12_disk_reader: src/fat_12_disk_reader.c

	# Compile the fat_12_disk_reader
	gcc -o bin/fat_12_disk_reader src/fat_12_disk_reader.c

clean:
	rm -r bin/

