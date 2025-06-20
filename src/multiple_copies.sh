#!/bin/bash
for i in {1..223}; do
    mcopy -i bin/floppy.img bin/lore.txt "::lore${i}.txt"
done
