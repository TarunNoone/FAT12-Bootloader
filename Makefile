.PHONY: build

build:
	mkdir -p build/
	nasm src/main.asm -f bin -o build/main.bin
	cp build/main.bin build/floppy.img
	truncate -s 1440k build/floppy.img

clean:
	rm -r build/
