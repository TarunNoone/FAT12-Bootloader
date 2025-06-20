// accept fat-12 img file as command line argument

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t jmp_short_nop[3];
    char oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries_count;
    uint16_t total_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;
} __attribute__((packed)) BootRecord;

typedef struct {
    uint8_t drive_number;
    uint8_t reserved_windows_nt;
    uint8_t signature;
    uint32_t volume_id;
    char volume_label[11];
    char system_identifier[8];
} __attribute__((packed)) ExtendedBootRecord;

typedef struct {
    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed)) BootCode;

FILE *image_file = NULL;
BootRecord boot_record;
ExtendedBootRecord extended_boot_record;
BootCode boot_code;

void open_disk_img(char *image_path) {
    image_file = fopen(image_path, "rb");
    if (image_file == NULL) {
        printf("Error: Could not open image file %s\n", image_path);
    }
}

void print_hex(char *label, uint8_t *ptr, int size) {
    // I want to read the struct byte-wise.
    // So I get a pointer to the struct.
    // But I need the pointer to advance by 1 Byte at a time.
    // So convert the pointer to a uint8_t pointer.
    // uint8_t *drive_reader = (uint8_t *)&boot_drive;

    // Cool stuff with pointer dereferncing.
    // printf("%02X ", *(drive_reader + i));
    // You know what else does the same thing...array indexing.
    // In C, arr[i] is literally same as *(arr + i).
    // So we can do this neat thing:
    // So tecnically *p = *(p + 0) = p[0]

    printf("%s: \t", label);
    for(int i = 0; i < size; i++) {
        printf("%02X ", ptr[i]);
    }
    printf("\n");
}

void read_boot_drive_info() {
    fread(&boot_record, sizeof(BootRecord), 1, image_file);
    fread(&extended_boot_record, sizeof(ExtendedBootRecord), 1, image_file);
    fread(&boot_code, sizeof(BootCode), 1, image_file);
    // Chechking few fields to see if the struct is packed correctly.
    print_hex("JMP SHORT NOP       ", (uint8_t *) &boot_record.jmp_short_nop             , 3);
    print_hex("OEM IDENTIFIER      ", (uint8_t *) &boot_record.oem_identifier            , 8);
    print_hex("BYTES PER SECTOR    ", (uint8_t *) &boot_record.bytes_per_sector          , 2);
    print_hex("SECTORS PER CLUSTER ", (uint8_t *) &boot_record.sectors_per_cluster       , 1);
    printf("\n");

    print_hex("DRIVE NUMBER        ", (uint8_t *) &extended_boot_record.drive_number         ,  1);
    print_hex("VOLUME ID           ", (uint8_t *) &extended_boot_record.volume_id            ,  4);
    print_hex("VOLUME LABEL        ", (uint8_t *) &extended_boot_record.volume_label         , 11);
    print_hex("SYSTEM IDENTIFIER   ", (uint8_t *) &extended_boot_record.system_identifier    ,  8);
    printf("\n");

    print_hex("BOOT SIGNATURE      ", (uint8_t *) &boot_code.boot_signature, 2);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image_file_path>\n", argv[0]);
        return 1;
    }
    
    char *image_path = argv[1];
    printf("\nImage file path: %s\n\n", image_path);

    // open_disk_img(image_path);
    // read_boot_drive_info();

    // Debugging incorrect boot siganture
    // 4 bytes were missing in the BootDrive
    // uint16_t was used instead of uint32_t for the last 2 parameters.
    
    printf("BootRecord: %lu\n", sizeof(BootRecord));
    printf("ExtendedBootRecord: %lu\n", sizeof(ExtendedBootRecord));
    printf("BootCode: %lu\n", sizeof(BootCode));
    printf("BootRecord + ExtendedBootRecord: %lu\n", sizeof(BootRecord) + sizeof(ExtendedBootRecord));
    printf("BootRecord + ExtendedBootRecord + BootCode: %lu\n", sizeof(BootRecord) + sizeof(ExtendedBootRecord) + sizeof(BootCode));
    
    return 0;
}
