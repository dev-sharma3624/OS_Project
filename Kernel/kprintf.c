#include "kprintf.h"
#include "BasicRenderer.h"

void kReverse(char* s){
    char temp;

    int len = 0;
    while( s[len] != '\0'){ len++; }

    for(int i = 0, j = len - 1; i < j; i++, j--){
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void convertIntToAscii(int n, char s[]){
    int i = 0, sign = n;

    if(n == -2147483648){
        convertIntToAscii(n + 1, s);

        int len = 0;
        while(s[len] != '\0') len++;

        s[len - 1] += 1;
        return;
    }

    if(n < 0){
        n = -n;
    }

    do{
        s[i++] = n % 10 + '0';
        n /= 10;
    }while ( n > 0);

    if( sign < 0){
        s[i++] = '-';
    }

    s[i] = '\0';

    kReverse(s);
    
}

void convertHexToAscii(unsigned long n, char s[]){
    int i = 0;
    unsigned long m = n;
    char hexDigits[] = "0123456789ABCDEF";

    do{
        s[i++] = hexDigits[ m % 16];
        m /= 16;
    }while(m > 0);

    s[i++] = 'x';
    s[i++] = '0';
    s[i] = '\0';

    kReverse(s);
}

void kPrintf(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    char temp[32];

    for(const char* p = fmt; *p != '\0'; p++){
        
        if(*p != '%'){
            BasicRenderer_PutChar(*p);
            continue;
        }

        switch (*++p)
        {
        case 'c':
            BasicRenderer_PutChar(va_arg(args, int));
            break;

        case 's':
            BasicRenderer_Print(va_arg(args, char*));
            break;

        case 'd':
        case 'i':
            convertIntToAscii(va_arg(args, int), temp);
            BasicRenderer_Print(temp);
            break;

        case 'x':
            convertHexToAscii(va_arg(args, unsigned int), temp);
            BasicRenderer_Print(temp);
            break;

        case 'p':
            convertHexToAscii(va_arg(args, unsigned long), temp);
            BasicRenderer_Print(temp);
            break;

        case '%':
            BasicRenderer_PutChar('%');
            break;
        
        default:
            BasicRenderer_PutChar('%');
            BasicRenderer_PutChar(*p);
            break;
        }
    }
    va_end(args);
}