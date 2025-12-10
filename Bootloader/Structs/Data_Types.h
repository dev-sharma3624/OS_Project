// -------------------------------------------------------------------------
// 1. UEFI DATA TYPES (From Spec Chapter 2)
// -------------------------------------------------------------------------

typedef unsigned char     UINT8;
typedef unsigned short int    UINT16;
typedef unsigned int    UINT32;
typedef unsigned long long    UINT64;
typedef unsigned long long    UINTN;      // "Native" Integer (64-bit on x64 Systems)

typedef signed char      INT8;
typedef signed short int     INT16;
typedef signed int     INT32;
typedef signed long long     INT64;
typedef signed long long     INTN;

typedef void        VOID;
typedef unsigned short int    CHAR16;     // UEFI uses 2-byte characters (Unicode)
typedef unsigned char     BOOLEAN;


typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef UINT64 EFI_PHYSICAL_ADDRESS;


// -------------------------------------------------------------------------
// 2. UEFI SPECIFIC HANDLES (From Spec Chapter 2)
// -------------------------------------------------------------------------

typedef void* EFI_HANDLE;
typedef UINTN       EFI_STATUS; // Status is just a native number (returns 0 for Success)



// 1. Helper Macros (The "Sticky Notes" for humans)
// These define to NOTHING. The compiler deletes them, but VS Code is happy.
#define IN
#define OUT
#define OPTIONAL

// 2. The Calling Convention (The "Handshake")
// This tells the compiler how to pass data to the function.

// If you are on Windows (Visual Studio / MSVC):
#ifdef _MSC_VER
    #define EFIAPI __cdecl 
#else
// If you are on Linux/Mac/WSL (GCC or Clang):
    #define EFIAPI __attribute__((ms_abi))
#endif


#ifndef NULL
#define NULL ((void*)0)
#endif