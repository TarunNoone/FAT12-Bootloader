// accept fat-12 img file as command line argument

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// BIOS Parameter Block aka. Boot Record
typedef struct {
    uint8_t jmp_short_nop[3];
    unsigned char oem_identifier[8];
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
    unsigned char volume_label[11];
    unsigned char system_identifier[8];
    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed)) ExtendedBootRecord;

typedef struct {
    unsigned char file_name[11]; 
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
} __attribute__((packed)) StandardDirectoryEntry;

typedef struct {
    uint8_t sequence_number; // This is 1-based index.
    unsigned char name_1[10]; // It's easier to print chars, than uint16_t // uint16_t name_1[5];
    uint8_t attribute; // always 0x0F for LFN entries
    uint8_t long_entry_type;
    uint8_t checksum;
    unsigned char name_2[12];
    uint8_t always_zero;
    unsigned char name_3[4];
} __attribute__((packed)) LongFileNameEntry;

typedef union {
    StandardDirectoryEntry standard_entry;
    LongFileNameEntry lfn_entry;
} RootDirectoryEntry;

typedef uint8_t Sector[512];

FILE *image_file = NULL;

int sectors_in_reserved_section;
int sectors_in_fat_section;
int sectors_in_root_directory;
int sectors_in_data_section;

int first_fat_sector_index;
int file_desc_fat_section_offset;

int first_data_sector_index;
int file_desc_data_section_offset;

BootRecord boot_record;
ExtendedBootRecord extended_boot_record;

void open_disk_img(unsigned char *image_path) {
    image_file = fopen(image_path, "rb");
    if (image_file == NULL) {
        printf("Error: Could not open image file %s\n", image_path);
    }
}

void print_hex(unsigned char *label, uint8_t *ptr, int size) {
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

void print_decimal(unsigned char *label, uint32_t number) {
    printf("%s: \t", label);
    printf("%d\n", number);
}

void print_string(unsigned char *label, unsigned char *str, int size) {
    printf("%s: \t", label);
    int null_count = 0;

    for(int i = 0; i < size; i++) {
        if(str[i] == 0x00) {
            null_count++;
        }
        printf("%c", str[i]);
    }
    printf("\n");
    // printf("Null count: %d\n", null_count);
}

void print_long_file_name(unsigned char *label, unsigned char *str, int size) {
    printf("%s: \t", label);
    int null_count = 0;
    int padding_count = 0;

    for(int i = 0; i < size; i++) {
        // Skip 0x00. It's a NULL character.
        // Skip 0xFF. It's just padding.
        // It needs to be casted to unsigned char.
        // Otherwise, it'll be implicitly casted to int, which is 32-bit wide.
        // So it'll become FF FF FF FF
        // Better to use unsigned char everywhere to avoid type issues.
        if(str[i] == 0xFF) {
            padding_count++;
            // printf(" + ");
            continue;
        }

        if(str[i] == 0x00) {
            null_count++;
            // printf(" - ");
            continue;
        }

        // For debugging what the char is.
        // if(('a' <= str[i] && str[i] <= 'z') || str[i] == '_' || str[i] == '.') {
        //     printf("%c", str[i]);
        // } else {
        //     printf(" %02X ", str[i]);
        // }

        printf("%c", str[i]);
    }
    printf("\n");

    // Just for checking if the null and padding chars are being detected.
    // printf("Null count: %d\n", null_count);
    // printf("Padding count: %d\n", padding_count);
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

    // Bytes are read from the volume / storage in units of sectors.
    // So it's better to know how many sectors each sections have.
    // Ideally these should be clusters_in_x_section, but since custer = sector in this, it's fineeee for now.
    sectors_in_reserved_section = boot_record.reserved_sectors;
    sectors_in_fat_section = boot_record.fat_count * boot_record.sectors_per_fat;

    // Round up to nearest sector count.
    sectors_in_root_directory = ((boot_record.root_dir_entries_count * sizeof(RootDirectoryEntry)) + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
    sectors_in_data_section = boot_record.total_sectors - (sectors_in_reserved_section + sectors_in_fat_section + sectors_in_root_directory);

    // Calculate the offset of FAT Section, Data Section wrt. the start of the Floppyy Disk Image.
    first_fat_sector_index = sectors_in_reserved_section;
    file_desc_fat_section_offset = sectors_in_reserved_section * boot_record.bytes_per_sector;

    first_data_sector_index = sectors_in_reserved_section + sectors_in_fat_section + sectors_in_root_directory;
    file_desc_data_section_offset = first_data_sector_index * boot_record.bytes_per_sector;

    printf("Sectors in Reserved Section                     : %d\n", sectors_in_reserved_section);
    printf("Sectors in FAT Section                          : %d\n", sectors_in_fat_section);
    printf("Sectors in Root Directory                       : %d\n", sectors_in_root_directory);
    printf("Sectors in Data Section                         : %d\n", sectors_in_data_section);
    printf("Total Sectors                                   : %d\n", boot_record.total_sectors);
    printf("File Descriptor Data Sector Offset in Bytes     : %d\n", file_desc_data_section_offset);

    printf("\n");
}

void read_file_allocation_table_section() {
    // After the Reserved Section is the File Allocation Table Section.
    // There are 2 FAT tables here usually. This is intended for redudancy.
    // Each FAT table contains 9 sectors.

    // This is too much data to read into memory.
    // Skip over the FAT table section.
    fseek(image_file, boot_record.fat_count * boot_record.sectors_per_fat * boot_record.bytes_per_sector, SEEK_CUR);
}

int too_many_prints = 0;
void print_standard_directory_entry(StandardDirectoryEntry *entry) {
    if(too_many_prints > 5) return;

    print_string    ("FILE NAME                         ", (unsigned char *) &entry->file_name                   , 11);
    print_hex       ("ATTRIBUTE                         ", (uint8_t *) &entry->attribute                         ,  1);
    print_hex       ("RESERVED WINDOWS NT               ", (uint8_t *) &entry->reserved_windows_nt               ,  1);
    print_hex       ("CREATION TIME IN HUNDREDTH SECS   ", (uint8_t *) &entry->creation_time_in_hundredth_secs   ,  1);
    print_hex       ("CREATED TIME                      ", (uint8_t *) &entry->created_time                      ,  2);
    print_hex       ("CREATED DATE                      ", (uint8_t *) &entry->created_date                      ,  2);
    print_hex       ("LAST ACCESSED DATE                ", (uint8_t *) &entry->last_accessed_date                ,  2);
    print_hex       ("ALWAYS ZERO                       ", (uint8_t *) &entry->always_zero                       ,  2);
    print_hex       ("LAST MODIFIED TIME                ", (uint8_t *) &entry->last_modified_time                ,  2);
    print_hex       ("LAST MODIFIED DATE                ", (uint8_t *) &entry->last_modified_date                ,  2);
    print_decimal   ("FIRST CLUSTER NUMBER              ", entry->first_cluster_number                               );
    print_decimal   ("FILE SIZE IN BYTES                ", entry->file_size_in_bytes                                 );
    printf("\n");
    too_many_prints++;
}

uint8_t get_next_cluster_number(int active_cluster_number) {

    // Sector fat_table[boot_record.sectors_per_fat];
    // fseek(image_file, file_desc_fat_section_offset, SEEK_SET);
    // fread(&fat_table, sizeof(Sector) * boot_record.sectors_per_fat, 1,image_file);

    // // print_hex("FAT TABLE", (uint8_t *) &fat_table, sizeof(Sector) * boot_record.sectors_per_fat);

    // printf("Size of short dtype: %zu \n", sizeof(unsigned short));

    // (9 sectors / FAT_table * 512 bytes / sector * 8 bits / bytes) / (12 bits / FAT entry) = 3072 FAT entries / FAT_table
    uint8_t fat_table[boot_record.sectors_per_fat * boot_record.bytes_per_sector];

    fseek(image_file, file_desc_fat_section_offset, SEEK_SET);
    fread(&fat_table, boot_record.sectors_per_fat * boot_record.bytes_per_sector, 1, image_file);
    
    uint16_t table_value;
    int fat12_table_index = (active_cluster_number + active_cluster_number / 2) % (boot_record.sectors_per_fat * boot_record.bytes_per_sector);

    if(fat12_table_index % 2 == 1) {
        table_value = *((uint16_t *)(&fat_table[fat12_table_index])) >> 4;
    } else {
        table_value = *((uint16_t *)(&fat_table[fat12_table_index])) & 0xFFF;
    }

    if(table_value >= 0xFF8) {
        printf("There are no more clusters in the chain.\n");
    } else if(table_value == 0xFF7) {
        printf("This is a bad cluster.\n");
    } else {
        printf("Next cluster number: %d\n", table_value);
    }
    printf("\n");
    return table_value;
}

void read_cluster(int active_cluster_number) {
    int cluster_offset_in_bytes;
    Sector cluster_data;

    cluster_offset_in_bytes = file_desc_data_section_offset + (active_cluster_number - 2) * boot_record.sectors_per_cluster * boot_record.bytes_per_sector;

    // Move the file pointer to the cluster location
    fseek(image_file, 
            cluster_offset_in_bytes,
            SEEK_SET
        );

    // Read the cluster data
    fread(&cluster_data, sizeof(Sector), 1, image_file);
    print_string("CLUSTER DATA  ", (unsigned char *) &cluster_data, sizeof(Sector));
    printf("\n");
}

// According the FAT12 OSDev Wiki Page, FAT considered storage as a series of clusters.
// So each data entry points to a cluster containing the data. This cluster is located in the Data Section.
// If the data can't fit in a single cluster, then it points to another cluster in the Data Section.
// Sort of like a linked list.

void read_data_in_this_entry(StandardDirectoryEntry *entry) {
    read_cluster(entry->first_cluster_number);
    uint16_t next_cluster_number = get_next_cluster_number(entry->first_cluster_number);

    if(next_cluster_number >= 0xFF8) {
        // printf("There are no more clusters in the chain.\n");
    } else if(next_cluster_number == 0xFF7) {
        // printf("This is a bad cluster.\n");
    } else {
        // printf("Next cluster number: %d\n", table_value);
        read_cluster(next_cluster_number);
    }
}

void read_root_directory_section() {
    const int bytes_name_1 = 2 * 5;
    const int bytes_name_2 = 2 * 6;
    const int bytes_name_3 = 2 * 2;
    const int bytes_per_lfn_entry = bytes_name_1 + bytes_name_2 + bytes_name_3;

    // Original I thought it could have 223 LFN entries + 1 Standard Entry that the LFN corresponds to.
    // But the sequence number uses bit-7 as a flag to indiciate last entry in the LFN chain.
    // So only 6 bits can be used for the actual sequence number.
    // Highest 6 bit sequence number is 63 or 0x3F.
    // Sequence number range: 1 - 63, i.e. total 63 numbers

    // TODO: There's a bug where reducing the buffer size messed up the Long File Name printing.
    // Using dynamic sizeof(string) is causing an issue with: memcpy, and offset calculation in memcpy
    // Switching to bytes_name_x fixed it.

    // This number is much lower at 20. Calculated through brute force.
    
    const int buffer_size = 20 * bytes_per_lfn_entry;
    unsigned char temporary_buffer[buffer_size];
    int long_file_name_size = 0;

    RootDirectoryEntry *entries = (RootDirectoryEntry *) malloc(sizeof(RootDirectoryEntry) * boot_record.root_dir_entries_count);
    fread(entries, sizeof(RootDirectoryEntry), boot_record.root_dir_entries_count, image_file);

    int unused_entries = 0;
    int empty_entries = 0;
    int lfn_entries = 0;
    int standard_entries = 0;

    for(int i=0; i < boot_record.root_dir_entries_count; i++) {

        // Step 1:
        // First byte tells if directory is empty / unused / has data.
        // LFN Entries Seq Number just happens to be exactly the byte we need to check.
        if(entries[i].standard_entry.file_name[0] == 0x00 || entries[i].lfn_entry.sequence_number == 0x00) {
            empty_entries++;
            continue;
        }

        // Step 2:
        // Check if the entry is unused.
        if(entries[i].standard_entry.file_name[0] == 0xE5 || entries[i].lfn_entry.sequence_number == 0xE5) {
            unused_entries++;
            continue;
        }

        // Step 3:
        // Check if the entry is a Long File Name entry.
        if(entries[i].standard_entry.attribute == 0x0F || entries[i].lfn_entry.attribute == 0x0F) {
            lfn_entries++;

            // Step 4:
            // Read the portion of long file name into temporary buffer

            // For some reason, the last entry has extra 64? Like: 1, 2, 3, 4, 5 + 64. Why???
            // 64 or 0x40 corresponds to the bit-7 of the sequence number.
            // This bit is used as a flag to indicate the last long file name entry.
            int is_last_lfn_entry = (0x40 & entries[i].lfn_entry.sequence_number) > 0;
            
            // Remove the flag for last entry from the sequence number
            // It keeps the sequence number based calculations simpler.
            if(is_last_lfn_entry) {
                entries[i].lfn_entry.sequence_number ^= 0x40; // Set the bit-7 to 0.
            }

            // printf("Sequence Number: %d\n", entries[i].lfn_entry.sequence_number);

            // print_string("NAME 1", entries[i].lfn_entry.name_1, 10);
            // print_string("NAME 2", entries[i].lfn_entry.name_2, 12);
            // print_string("NAME 3", entries[i].lfn_entry.name_3, 4);

            long_file_name_size += bytes_per_lfn_entry;

            memcpy(
                    (
                        temporary_buffer + 
                        (entries[i].lfn_entry.sequence_number - 1) * bytes_per_lfn_entry
                    ),
                    entries[i].lfn_entry.name_1, 
                    bytes_name_1
                );

            memcpy(
                    (
                        temporary_buffer + 
                        (entries[i].lfn_entry.sequence_number - 1) * bytes_per_lfn_entry + 
                        bytes_name_1                        
                    ),
                    entries[i].lfn_entry.name_2, 
                    bytes_name_2
                );
                    
            memcpy(
                    (
                        temporary_buffer + 
                        (entries[i].lfn_entry.sequence_number - 1) * bytes_per_lfn_entry + 
                        bytes_name_1 + 
                        bytes_name_2
                    ), 
                    entries[i].lfn_entry.name_3, 
                    bytes_name_3
                );

            continue;
        } else{
            // Step 5:
            // For a standard entry, check if there was any Long File Name stored in the temporary buffer
            // If it is, then that's then name associated with the the current standard entry.

            // Established through brute force that max number of entries used will be 20 for LFN.
            if(long_file_name_size > 0) {
                print_long_file_name("LONG FILE NAME    ", temporary_buffer, long_file_name_size);
                printf("Number of entries used: %d\n", long_file_name_size / bytes_per_lfn_entry);
                long_file_name_size = 0;
            }
            standard_entries++;
        }

        // Step 6:
        // Print the standard entry data.
        printf("Entry %d:\n", i);
        print_standard_directory_entry(&entries[i].standard_entry);

        // Step 7:
        // Read the data in this entry if the attribute is ARCHIEVE.
        if(entries[i].standard_entry.attribute == 0x20) {
            read_data_in_this_entry(&entries[i].standard_entry);
        }
    }

    printf("Total entries       : %d\n", unused_entries + empty_entries + lfn_entries + standard_entries);
    printf("Unused entries      : %d\n", unused_entries);
    printf("Empty entries       : %d\n", empty_entries);
    printf("LFN entries         : %d\n", lfn_entries);
    printf("Standard entries    : %d\n", standard_entries);
    printf("\n");    
}

void close_disk_img() {
    // Forgot that closing a file is a thing
    if(image_file != NULL) fclose(image_file);
}

int main(int argc, unsigned char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image_file_path>\n", argv[0]);
        return 1;
    }
    
    unsigned char *image_path = argv[1];
    printf("\nImage file path: %s\n\n", image_path);

    open_disk_img(image_path);
    read_boot_drive_section();
    read_file_allocation_table_section();
    read_root_directory_section();
    close_disk_img();
    
    return 0;
}
