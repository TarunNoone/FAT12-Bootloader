// accept fat-12 img file as command line argument

#include <stdio.h>
#include <stdint.h>

// BIOS Parameter Block aka. Boot Record
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

// Extended BIOS Parameter Block aka. Extended Boot Record
typedef struct {
    uint8_t drive_number;
    uint8_t reserved_windows_nt;
    uint8_t signature;
    uint32_t volume_id;
    char volume_label[11];
    char system_identifier[8];
    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed)) ExtendedBootRecord;

typedef struct {
    char file_name[11]; 
    uint8_t attribute;
    uint8_t reserved_windows_nt;
    uint8_t creation_time_in_hundredth_secs;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t last_accessed_date;
    uint16_t always_zero;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t first_cluster_number;
    uint32_t file_size_in_bytes;
} __attribute__((packed)) DirectoryEntry;

FILE *image_file = NULL;

BootRecord boot_record;
ExtendedBootRecord extended_boot_record;

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

void print_string(char *label, char *str, int size) {
    printf("%s: \t", label);
    for(int i = 0; i < size; i++) {
        printf("%c", str[i]);
    }
    printf("\n");
}

void read_boot_drive_section() {
    // First Sector of a drive contain the Boot Drive information for BIOS.
    // FAT 12 considers first Sector as Reserved Section.
    // This section contains the BPB and EBPB information for FAT12 File System.

    fread(&boot_record, sizeof(BootRecord), 1, image_file);
    fread(&extended_boot_record, sizeof(ExtendedBootRecord), 1, image_file);

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
    print_hex("BOOT SIGNATURE      ", (uint8_t *) &extended_boot_record.boot_signature       ,  2);
    printf("\n");
}

void read_file_allocation_table_section() {
    // After the Reserved Section is the File Allocation Table Section.
    // There are 2 FAT tables here usually. This is intended for redudancy.
    // Each FAT table contains 9 sectors.
    
    // This is too much data to read without much use right now.
    // Instead using fseek to skip over the FAT table section..
    int fat_table_size = boot_record.fat_count * boot_record.sectors_per_fat * boot_record.bytes_per_sector;
    // uint8_t *fat_tables = (uint8_t *) malloc(sizeof(uint8_t) * fat_table_size);
    // fread(fat_tables, fat_table_size, 1, image_file);

    // Skip over the FAT table section.
    fseek(image_file, fat_table_size, SEEK_CUR);
}

void print_directory_entry(DirectoryEntry *entry) {
    print_string("FILE NAME                         ", (char *) &entry->file_name                            , 11);
    print_hex   ("ATTRIBUTE                         ", (uint8_t *) &entry->attribute                         ,  1);
    print_hex   ("RESERVED WINDOWS NT               ", (uint8_t *) &entry->reserved_windows_nt               ,  1);
    print_hex   ("CREATION TIME IN HUNDREDTH SECS   ", (uint8_t *) &entry->creation_time_in_hundredth_secs   ,  1);
    print_hex   ("CREATED TIME                      ", (uint8_t *) &entry->created_time                      ,  2);
    print_hex   ("CREATED DATE                      ", (uint8_t *) &entry->created_date                      ,  2);
    print_hex   ("LAST ACCESSED DATE                ", (uint8_t *) &entry->last_accessed_date                ,  2);
    print_hex   ("ALWAYS ZERO                       ", (uint8_t *) &entry->always_zero                       ,  2);
    print_hex   ("LAST MODIFIED TIME                ", (uint8_t *) &entry->last_modified_time                ,  2);
    print_hex   ("LAST MODIFIED DATE                ", (uint8_t *) &entry->last_modified_date                ,  2);
    print_hex   ("FIRST CLUSTER NUMBER              ", (uint8_t *) &entry->first_cluster_number              ,  2);
    print_hex   ("FILE SIZE IN BYTES                ", (uint8_t *) &entry->file_size_in_bytes                ,  4);
    printf("\n");
}
void read_root_directory() {
    DirectoryEntry entry;
    for(int i=0; i < 3; i++) {
        fread(&entry, sizeof(DirectoryEntry), 1, image_file);
        print_directory_entry(&entry);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image_file_path>\n", argv[0]);
        return 1;
    }
    
    char *image_path = argv[1];
    printf("\nImage file path: %s\n\n", image_path);

    open_disk_img(image_path);
    read_boot_drive_section();
    read_file_allocation_table_section();
    read_root_directory();
    
    return 0;
}
