org 0x7C00
bits 16

; Skip over non-code data
jmp short main
nop

; BPB
oem_identifier:         db "MSWIN4.1"
bytes_per_sector:       dw 512
sectors_per_cluster:    db 1
reserved_sectors:       dw 1
fat_count:              db 2
dir_entries_count:      dw 0E0h
total_sectors:          dw 2880
media_descriptor:       db 0F0h
sectors_per_fat:        dw 9
sectors_per_track:      dw 18
heads:                  dw 2
hidden_sectors:         dw 0
large_sector_count:     dw 0

; EBPB aka. Extended Boot Record
drive_number:           db 0
reserved_windows_nt:    db 0
signature:              db 0x29
volume_id:              db 12h, 34h, 56h, 78h
volume_label:           db "Hello World"
system_identifier:      db "FAT12   "

main:
    mov ax, 0
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7C00

    mov si, msg_hello_world
    call puts

    cli  
    hlt  

puts:

.loop:
    lodsb
    test al, al
    jz .done

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .loop

.done:
    ret 

.halt:
    jmp .halt

msg_hello_world: db "Hello World!", 0X0D, 0X0A, 0

times 510 - ($ - $$) db 0  
dw 55AAh                                                                    
