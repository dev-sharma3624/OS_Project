#include "Data_Types.h"

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_STRING) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16 *String
);



typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {

    // EFI_TEXT_RESET Reset;
    void* Reset;

    EFI_TEXT_STRING OutputString;

    // EFI_TEXT_TEST_STRING TestString;
    void* TestString;

    // EFI_TEXT_QUERY_MODE QueryMode;
    void* QueryMode;

    // EFI_TEXT_SET_MODE SetMode;
    void* SetMode;

    // EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    void* SetAttribute;

    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    
    /* EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE *Mode; */

} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;