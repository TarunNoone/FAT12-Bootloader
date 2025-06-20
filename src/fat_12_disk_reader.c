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
    uint16_t hidden_sectors;
    uint16_t large_sector_count;
} BootDrive;

FILE *image_file = NULL;
BootDrive boot_drive;

void read_disk_img(char *image_path) {
    image_file = fopen(image_path, "rb");
    if (image_file == NULL) {
        printf("Error: Could not open image file %s\n", image_path);
    }
}

void read_info() {
    fread(&boot_drive, sizeof(BootDrive), 1, image_file);

    // I want to read the struct byte-wise.
    // So I get a pointer to the struct.
    // But I need the pointer to advance by 1 Byte at a time.
    // So convert the pointer to a uint8_t pointer.
    uint8_t *drive_reader = (uint8_t *)&boot_drive;

    for(int i = 0; i < sizeof(boot_drive); i++) {
        // Cool stuff with pointer dereferncing.
        // printf("%02X ", *(drive_reader + i));
        // You know what else does the same thing...array indexing.
        // In C, arr[i] is literally same as *(arr + i).
        // So we can do this neat thing:
        // So tecnically *p = *(p + 0) = p[0]

        printf("%02X ", drive_reader[i]);
    }

    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image_file_path>\n", argv[0]);
        return 1;
    }
    
    char *image_path = argv[1];
    printf("\nImage file path: %s\n\n", image_path);

    read_disk_img(image_path);
    read_info();
    
    return 0;
}
