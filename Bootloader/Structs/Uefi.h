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

#include "Data_Types.h"
#include "Simple_Text_Output_Protocol.h"
#include "Simple_Text_Input_Protocol.h"


typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;


typedef struct {
UINT32 Type;
EFI_PHYSICAL_ADDRESS PhysicalStart;
EFI_VIRTUAL_ADDRESS VirtualStart;
UINT64 NumberOfPages;
UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;



typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP) (
IN OUT UINTN *MemoryMapSize,
OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
OUT UINTN *MapKey,
OUT UINTN *DescriptorSize,
OUT UINT32 *DescriptorVersion
);



typedef
EFI_STATUS
(EFIAPI *EFI_EXIT_BOOT_SERVICES) (
IN EFI_HANDLE ImageHandle,
IN UINTN MapKey
);


typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiUnacceptedMemoryType,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;





typedef struct {
    EFI_TABLE_HEADER Hdr;

    //
    // Task Priority Services
    //

    // EFI_RAISE_TPL RaiseTPL; // EFI 1.0+
    void* RaiseTPL; // EFI 1.0+

    // EFI_RESTORE_TPL RestoreTPL; // EFI 1.0+
    void* RestoreTPL; // EFI 1.0+



    //
    // Memory Services
    //

    // EFI_ALLOCATE_PAGES AllocatePages; // EFI 1.0+
    void* AllocatePages; // EFI 1.0+

    // EFI_FREE_PAGES FreePages; // EFI 1.0+
    void* FreePages; // EFI 1.0+

    EFI_GET_MEMORY_MAP GetMemoryMap; // EFI 1.0+

    // EFI_ALLOCATE_POOL AllocatePool; // EFI 1.0+
    void* AllocatePool; // EFI 1.0+

    // EFI_FREE_POOL FreePool; // EFI 1.0+
    void* FreePool; // EFI 1.0+



    //
    // Event & Timer Services
    //


    // EFI_CREATE_EVENT CreateEvent; // EFI 1.0+
    void* CreateEvent; // EFI 1.0+

    // EFI_SET_TIMER SetTimer; // EFI 1.0+
    void* SetTimer; // EFI 1.0+
    
    // EFI_WAIT_FOR_EVENT WaitForEvent; // EFI 1.0+
    void* WaitForEvent; // EFI 1.0+

    // EFI_SIGNAL_EVENT SignalEvent; // EFI 1.0+
    void* SignalEvent; // EFI 1.0+

    // EFI_CLOSE_EVENT CloseEvent; // EFI 1.0+
    void* CloseEvent; // EFI 1.0+

    // EFI_CHECK_EVENT CheckEvent; // EFI 1.0+
    void* CheckEvent; // EFI 1.0+



    //
    // Protocol Handler Services
    //

    // EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface; // EFI 1.˓→0+
    void* InstallProtocolInterface; // EFI 1.˓→0+

    // EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface; // EFI 1.˓→0+
    void* ReinstallProtocolInterface; // EFI 1.˓→0+

    // EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface; // EFI 1.˓→0+
    void* UninstallProtocolInterface; // EFI 1.˓→0+

    // EFI_HANDLE_PROTOCOL HandleProtocol; // EFI 1.˓→0+
    void* HandleProtocol; // EFI 1.˓→0+

    // VOID* Reserved; // EFI 1.0+
    VOID* Reserved; // EFI 1.0+

    // EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify; // EFI 1.˓→0+
    void* RegisterProtocolNotify; // EFI 1.˓→0+

    // EFI_LOCATE_HANDLE LocateHandle; // EFI 1.˓→0+
    void* LocateHandle; // EFI 1.˓→0+

    // EFI_LOCATE_DEVICE_PATH LocateDevicePath; // EFI 1.˓→0+
    void* LocateDevicePath; // EFI 1.˓→0+

    // EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable; // EFI 1.˓→0+
    void* InstallConfigurationTable; // EFI 1.˓→0+





    //
    // Image Services
    //

    // EFI_IMAGE_UNLOAD LoadImage; // EFI 1.0+
    void* LoadImage; // EFI 1.0+

    // EFI_IMAGE_START StartImage; // EFI 1.0+
    void* StartImage; // EFI 1.0+

    // EFI_EXIT Exit; // EFI 1.0+
    void* Exit; // EFI 1.0+

    // EFI_IMAGE_UNLOAD UnloadImage; // EFI 1.0+
    void* UnloadImage; // EFI 1.0+

    EFI_EXIT_BOOT_SERVICES ExitBootServices; // EFI 1.0+

/* 

    //
    // Miscellaneous Services
    //


    // EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount; // EFI 1.0+
    void* GetNextMonotonicCount; // EFI 1.0+

    // EFI_STALL Stall; // EFI 1.0+
    void* Stall; // EFI 1.0+

    // EFI_SET_WATCHDOG_TIMER SetWatchdogTimer; // EFI 1.0+
    void* SetWatchdogTimer; // EFI 1.0+





    //
    // DriverSupport Services
    //



    EFI_CONNECT_CONTROLLER ConnectController; // EFI 1.1
    EFI_DISCONNECT_CONTROLLER DisconnectController; // EFI 1.1+
    //
    // Open and Close Protocol Services
    //
    EFI_OPEN_PROTOCOL OpenProtocol; // EFI 1.1+
    EFI_CLOSE_PROTOCOL CloseProtocol; // EFI 1.1+
    EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation;// EFI 1.1+
    //
    // Library Services
    //
    EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle; // EFI 1.1+
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer; // EFI 1.1+
    EFI_LOCATE_PROTOCOL LocateProtocol; // EFI 1.1+
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces; //
    ˓→ EFI 1.1+
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces; //
    ˓→ EFI 1.1+*
    //
    // 32-bit CRC Services
    //
    EFI_CALCULATE_CRC32 CalculateCrc32; // EFI 1.1+
    //
    // Miscellaneous Services
    //
    EFI_COPY_MEM CopyMem; // EFI 1.1+
    EFI_SET_MEM SetMem; // EFI 1.1+
    EFI_CREATE_EVENT_EX CreateEventEx; // UEFI 2.0+ */


} EFI_BOOT_SERVICES;



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

