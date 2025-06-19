.PHONY: build

build:
	mkdir -p build/
	
	# Create the Bootloader Binary
	nasm src/main.asm -f bin -o build/bootloader.bin
	
	# Create a block of text
	nasm src/text_file.asm -f bin -o build/text_file.bin	
	
	# Create a 1440kB file for floppy disk image	
	dd if=/dev/zero of=build/floppy.img bs=512 count=2880
	
	# Format the disk image as FAT-12 File System
	mkfs.fat -F 12 -n "HELLO DRIVE" build/floppy.img
	
	# Copy the bootloader to the first 512 Bytes of Floppy Disk Image
	dd if=build/bootloader.bin of=build/floppy.img conv=notrunc	
	
	# Copy a file into the FAT-12 File System
	mcopy -i build/floppy.img build/text_file.bin "::text_file.bin"

clean:
	rm -r build/
