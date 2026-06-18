int fs_create_file(char* filename, char* content);
void fs_read_file(char* recieved_file_name, uint64_t buffer);
void fs_ls();
void fs_create_dir(char* dirName);
void fs_change_dir(char* recieved_dir_name);