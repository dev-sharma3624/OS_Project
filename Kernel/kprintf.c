#include "kprintf.h"
#include "BasicRenderer.h"

void kReverse(char* s){
    char temp;

    int len = 0;
    while( s[len] != '\0'){ len++; }

    /* Since length is calculated excluding the null terminator '\0', it is never swapped and stays at end */

    for(int i = 0, j = len - 1; i < j; i++, j--){
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void convertIntToAscii(int n, char s[]){
    int i = 0, sign = n;

    // checking for overflow number since for a int max positive number is +2147483647 which is 1 less than the number we're checking for
    // logic requires to convert a negative number to positive and then add a negative sign ahead for number -> text tranformation
    // direct converting -2147483648 to positive will cause overflow because an integer variable can only store value upto +2147483647
    if(n == -2147483648){
        convertIntToAscii(n + 1, s);

        int len = 0;
        while(s[len] != '\0') len++;

        s[len - 1] += 1;
        return;
    }

    if(n < 0){ // converting to positive if negative
        n = -n;
    }

    /*
    Do-while because condition checks for n > 0 but if the number is 0, then loop will not run even once for a while-loop or for-loop
    */
    do{
        /*
        for e.g:
        n = 243, n % 10 = 3
        '0' = 48 (decimal), 0x30 (hexa) in ASCII
        3 + 48 = 51 => decimal of '3' in ASCII
        0x03 + 0x30 = 0x33 => hexadecimal of '3' in ASCII
        */
        s[i++] = n % 10 + '0';

        n /= 10; // truncating the last number. 243/10 = 24
    }while ( n > 0);

    if( sign < 0){ // adding the sign back if the number was negative
        s[i++] = '-';
    }

    s[i] = '\0'; // null terminator

    kReverse(s);
    
}

void convertHexToAscii(unsigned long n, char s[]){
    int i = 0;
    unsigned long m = n;
    char hexDigits[] = "0123456789ABCDEF";

    do{
        /*
        for e.g:
        n = 106, n % 16 = 10
        hexDigits[10] = A

        number remains = 106/16 => 6

        n % 16 = 6
        hexDigits[6] = 6

        final string before reverse = A6x0
        after reverse = 0x6A (decimal equivalent = 106)
        */
        s[i++] = hexDigits[ m % 16];

        m /= 16;
    }while(m > 0);

    s[i++] = 'x';
    s[i++] = '0';
    s[i] = '\0';

    kReverse(s);
}

void kPrintf(const char* fmt, ...){
    va_list args; // holds the state of the argument traversal
    va_start(args, fmt); // initializes va_list, offset pointer to the exact next address after the last know arg which in this case is fmt

    char temp[32]; //buffer required for conversions

    for(const char* p = fmt; *p != '\0'; p++){
        
        if(*p != '%'){ // if not format specifier then simply print the character
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