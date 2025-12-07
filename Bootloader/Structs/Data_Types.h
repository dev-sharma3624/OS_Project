#include <stdint.h> // Gives us uint32_t, uint64_t, etc.

// -------------------------------------------------------------------------
// 1. UEFI DATA TYPES (From Spec Chapter 2)
// -------------------------------------------------------------------------

typedef uint8_t     UINT8;
typedef uint16_t    UINT16;
typedef uint32_t    UINT32;
typedef uint64_t    UINT64;
typedef uint64_t    UINTN;      // "Native" Integer (64-bit on x64 Systems)

typedef int8_t      INT8;
typedef int16_t     INT16;
typedef int32_t     INT32;
typedef int64_t     INT64;
typedef int64_t     INTN;

typedef void        VOID;
typedef uint16_t    CHAR16;     // UEFI uses 2-byte characters (Unicode)
typedef uint8_t     BOOLEAN;


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