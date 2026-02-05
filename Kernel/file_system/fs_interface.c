#include <file_system/fat32.h>
#include <libs/k_string.h>
#include <file_system/fs_interface.h>

static uint64_t current_dir_cluster = 2;

int fs_create_file(char* filename, char* content){
    int result = fat32_create_file(filename, content, k_strlen(content), current_dir_cluster, FILE_ATTR);
    return result;
}

void fs_read_file(char* filename){
    fat32_read_file(current_dir_cluster, filename);
}

void fs_ls(){
    fat32_list_all_entries(current_dir_cluster);
}