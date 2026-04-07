#include <typedefs.h>

char* k_strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    
    *dest = '\0';
    
    return original_dest;
}

int k_strcmp(const char* s1, const char* s2) {

    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int k_strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }

    // If we ran out of 'n', it's a match (up to that point)
    if (n == 0) return 0;

    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int k_strlen(char* s) {
    int len = 0;
    while( s[len] != '\0'){ len++; }
    return len;
}

char k_str_to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

void str_trim(char* str) {
    if (!str) return;

    char* start = str;
    
    // 1. Skip leading whitespaces
    // (Assuming space, tab, newline are whitespaces)
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n')) {
        start++;
    }

    // 2. Move the trimmed content to the beginning of the buffer
    if (start != str) {
        char* temp = str;
        char* src = start;
        while (*src) {
            *temp++ = *src++;
        }
        *temp = '\0'; // Null terminate the new shorter string
    }

    // 3. Remove trailing whitespaces
    int len = k_strlen(str);
    char* end = str + len - 1;
    
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

int str_split(char* str, char delim, char** tokens) {
    if (!str) return 0;

    int idx = 0;
    
    // 1. Set the first token to the start of the string
    tokens[idx++] = str;

    char* p = str;
    while (*p) {
        if (*p == delim) {
            *p = '\0';           // Replace delimiter with null terminator
            
            // The NEXT character is the start of the new token
            // We store the address of (p + 1) into our tokens array
            tokens[idx++] = p + 1; 
        }
        p++;
    }
    
    return idx; // Return the count so the caller knows how many tokens exist
}

void str_pad(char* str, char pad_char, int n) {
    // Basic safety check for null pointers or invalid target lengths
    if (str == NULL || n <= 0) {
        return;
    }

    int current_len = k_strlen(str);

    // If the string is already at or beyond the target length, do nothing
    if (current_len >= n) {
        return;
    }

    // Pad the string starting from the original null terminator
    for (int i = current_len; i < n; i++) {
        str[i] = pad_char;
    }

    // Always ensure the new string is properly null-terminated
    str[n] = '\0';
}

void str_append(char* dest, const char* src) {
    // Safety check for null pointers
    if (dest == NULL || src == NULL) {
        return;
    }

    // 1. Traverse 'dest' until we find its null terminator ('\0')
    while (*dest != '\0') {
        dest++;
    }

    // 2. Copy characters from 'src' to the end of 'dest'
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }

    // 3. Explicitly add the null terminator to close the new string
    *dest = '\0';
}