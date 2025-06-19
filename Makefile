.PHONY: build
build: bin/floppy.img 
	# Top level build target	

bin:
	mkdir -p bin/

bin/bootloader.bin: bin	src/main.asm
	
	# Create the Bootloader Binary
	nasm src/main.asm -f bin -o bin/bootloader.bin

bin/lore.txt: bin
	
	# Create a text file
	echo "Lorem ipsum dolar sit amet" > bin/lore.txt

bin/floppy.img: bin bin/bootloader.bin bin/lore.txt
	
	# Create a 1440kB file for floppy disk image	
	dd if=/dev/zero of=bin/floppy.img bs=512 count=2880
	
	# Format the disk image as FAT-12 File System
	mkfs.fat -F 12 -n "HELLO DRIVE" bin/floppy.img
	
	# Copy a file into the FAT-12 File System
	mcopy -i bin/floppy.img bin/lore.txt "::lore.txt"
	
	# Copy the bootloader to the first 512 Bytes of Floppy Disk Image
	dd if=bin/bootloader.bin of=bin/floppy.img conv=notrunc	

clean:
	rm -r bin/

