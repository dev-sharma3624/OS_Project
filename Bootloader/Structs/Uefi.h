#ifndef UEFI_H
#define UEFI_H

#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42
#define EFI_BOOT_SERVICES_REVISION EFI_SPECIFICATION_VERSION
#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define EFI_RUNTIME_SERVICES_REVISION EFI_SPECIFICATION_VERSION
#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249
#define EFI_2_100_SYSTEM_TABLE_REVISION ((2<<16) | (100))
#define EFI_2_90_SYSTEM_TABLE_REVISION ((2<<16) | (90))
#define EFI_2_80_SYSTEM_TABLE_REVISION ((2<<16) | (80))
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2<<16) | (70))
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2<<16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2<<16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2<<16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2<<16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2<<16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2<<16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2<<16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2<<16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1<<16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1<<16) | (02))
#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION
#define EFI_SYSTEM_TABLE_REVISION EFI_2_100_SYSTEM_TABLE_REVISION


#include "Simple_Text_Output_Protocol.h"
#include "Simple_Text_Input_Protocol.h"
#include "Boot_Services.h"
#include "Gop.h"

typedef EFI_STATUS (EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP) (
    IN UINTN                  MemoryMapSize,
    IN UINTN                  DescriptorSize,
    IN UINT32                 DescriptorVersion,
    IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
);

typedef EFI_STATUS (EFIAPI *EFI_CONVERT_POINTER) (
    IN UINTN                  DebugDisposition,
    IN void                   **Address
);



typedef struct {

    EFI_TABLE_HEADER Hdr;

    //
    // Time Services
    //

    // EFI_GET_TIME GetTime;
    void* GetTime;

    // EFI_SET_TIME SetTime;
    void* SetTime;

    // EFI_GET_WAKEUP_TIME GetWakeupTime;
    void* GetWakeupTime;

    // EFI_SET_WAKEUP_TIME SetWakeupTime;
    void* SetWakeupTime;


    //
    // Virtual Memory Services
    //

    EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
    EFI_CONVERT_POINTER ConvertPointer;


    //
    // Variable Services
    //

    // EFI_GET_VARIABLE GetVariable;
    void* GetVariable;

    // EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
    void* GetNextVariableName;

    // EFI_SET_VARIABLE SetVariable;
    void* SetVariable;



    //
    // Miscellaneous Services
    //

    // EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
    void* GetNextHighMonotonicCount;

    // EFI_RESET_SYSTEM ResetSystem;
    void* ResetSystem;


    //
    // UEFI 2.0 Capsule Services
    //

    // EFI_UPDATE_CAPSULE UpdateCapsule;
    void* UpdateCapsule;

    // EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;
    void* QueryCapsuleCapabilities;

    //
    // Miscellaneous UEFI 2.0 Service
    //

    // EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
    void* QueryVariableInfo;

} EFI_RUNTIME_SERVICES;




typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    // EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;


/* typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_ENTRY_POINT) (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
); */

#endif