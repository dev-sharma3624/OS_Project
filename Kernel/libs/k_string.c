

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