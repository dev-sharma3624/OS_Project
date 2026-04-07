#include <file_system/fat32.h>
#include <libs/k_string.h>
#include <file_system/fs_interface.h>

#define ROOT_DIR_CLUSTER_NUMBER 2

static uint64_t current_dir_cluster = ROOT_DIR_CLUSTER_NUMBER;
static char current_dir_name[50];

int fs_create_file(char* recieved_file_name, char* content){
    char* name_and_ext[2];
    char fileName[12];
    str_split(recieved_file_name, '.', name_and_ext);
    str_trim(name_and_ext[0]);
    str_trim(name_and_ext[1]);

    k_strcpy(fileName, name_and_ext[0]);
    str_pad(fileName, ' ', 8);
    str_append(fileName, name_and_ext[1]);

    int result = fat32_create_file(fileName, content, k_strlen(content), current_dir_cluster, FILE_ATTR);
    return result;
}

void fs_read_file(char* filename){
    fat32_read_file(current_dir_cluster, filename);
}

void fs_ls(){
    fat32_list_all_entries(current_dir_cluster);
}

void fs_current_dir(char* buffer){

    if(current_dir_cluster == ROOT_DIR_CLUSTER_NUMBER){ // root directory
        *buffer = "root";
        return;
    }else{
        buffer = current_dir_name;
    }
}

void fs_create_dir(char* recieved_dir_name){
    char dirName[11];
    k_strcpy(dirName, recieved_dir_name);
    str_pad(dirName, " ", 10);
    dirName[10] = " ";
    fat32_create_dir(dirName, current_dir_cluster);
}

void fs_change_dir(char* dir_name){

}