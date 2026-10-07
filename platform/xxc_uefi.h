// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef XXC_UEFI_H
#define XXC_UEFI_H

typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;
typedef signed char        INT8;
typedef short              INT16;
typedef int                INT32;
typedef long long          INT64;
typedef UINT64             UINTN;
typedef INT64              INTN;
typedef UINT8              BOOLEAN;
typedef UINT16             CHAR16;
typedef UINT64             EFI_STATUS;
typedef void              *EFI_HANDLE;
typedef void              *EFI_EVENT;
typedef UINT64             EFI_PHYSICAL_ADDRESS;
typedef UINT64             EFI_VIRTUAL_ADDRESS;
typedef UINTN              EFI_TPL;

#define EFIAPI __attribute__((ms_abi))
#define IN
#define OUT
#define OPTIONAL

typedef struct { UINT32 a; UINT16 b, c; UINT8 d[8]; } EFI_GUID;

typedef struct {
    UINT64 Signature; UINT32 Revision, HeaderSize, CRC32, Reserved;
} EFI_TABLE_HEADER;

#define EFI_ERR(n)              (0x8000000000000000ULL | (n))
#define EFI_ERROR(s)            ((INT64)(s) < 0)

#define EFI_SUCCESS             0
#define EFI_WARN_UNKNOWN_GLYPH  1
#define EFI_WARN_DELETE_FAILURE 2
#define EFI_WARN_WRITE_FAILURE  3
#define EFI_WARN_BUFFER_TOO_SMALL 4

#define EFI_LOAD_ERROR          EFI_ERR(1)
#define EFI_INVALID_PARAMETER   EFI_ERR(2)
#define EFI_UNSUPPORTED         EFI_ERR(3)
#define EFI_BAD_BUFFER_SIZE     EFI_ERR(4)
#define EFI_BUFFER_TOO_SMALL    EFI_ERR(5)
#define EFI_NOT_READY           EFI_ERR(6)
#define EFI_DEVICE_ERROR        EFI_ERR(7)
#define EFI_WRITE_PROTECTED     EFI_ERR(8)
#define EFI_OUT_OF_RESOURCES    EFI_ERR(9)
#define EFI_VOLUME_CORRUPTED    EFI_ERR(10)
#define EFI_VOLUME_FULL         EFI_ERR(11)
#define EFI_NO_MEDIA            EFI_ERR(12)
#define EFI_MEDIA_CHANGED       EFI_ERR(13)
#define EFI_NOT_FOUND           EFI_ERR(14)
#define EFI_ACCESS_DENIED       EFI_ERR(15)
#define EFI_NO_RESPONSE         EFI_ERR(16)
#define EFI_NO_MAPPING          EFI_ERR(17)
#define EFI_TIMEOUT             EFI_ERR(18)
#define EFI_NOT_STARTED         EFI_ERR(19)
#define EFI_ALREADY_STARTED     EFI_ERR(20)
#define EFI_ABORTED             EFI_ERR(21)
#define EFI_ICMP_ERROR          EFI_ERR(22)
#define EFI_TFTP_ERROR          EFI_ERR(23)
#define EFI_PROTOCOL_ERROR      EFI_ERR(24)
#define EFI_INCOMPATIBLE_VERSION EFI_ERR(25)
#define EFI_SECURITY_VIOLATION  EFI_ERR(26)
#define EFI_CRC_ERROR           EFI_ERR(27)
#define EFI_END_OF_MEDIA        EFI_ERR(28)
#define EFI_END_OF_FILE         EFI_ERR(31)
#define EFI_INVALID_LANGUAGE    EFI_ERR(32)
#define EFI_COMPROMISED_DATA    EFI_ERR(33)

typedef enum {
    AllocateAnyPages, AllocateMaxAddress, AllocateAddress, MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType, EfiLoaderCode, EfiLoaderData,
    EfiBootServicesCode, EfiBootServicesData,
    EfiRuntimeServicesCode, EfiRuntimeServicesData,
    EfiConventionalMemory, EfiUnusableMemory,
    EfiACPIReclaimMemory, EfiACPIMemoryNVS,
    EfiMemoryMappedIO, EfiMemoryMappedIOPortSpace,
    EfiPalCode, EfiPersistentMemory, EfiUnacceptedMemoryType, EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    UINT32 Type; UINT32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS  VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

#define EFI_PAGE_SIZE 4096
#define EFI_MEMORY_UC 0x1ULL
#define EFI_MEMORY_WC 0x2ULL
#define EFI_MEMORY_WT 0x4ULL
#define EFI_MEMORY_WB 0x8ULL
#define EFI_MEMORY_RUNTIME 0x8000000000000000ULL

#define EVT_TIMER                         0x80000000
#define EVT_RUNTIME                       0x40000000
#define EVT_NOTIFY_WAIT                   0x00000100
#define EVT_NOTIFY_SIGNAL                 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

#define TPL_APPLICATION 4
#define TPL_CALLBACK    8
#define TPL_NOTIFY      16
#define TPL_HIGH_LEVEL  31

typedef void (EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT Event, void *Context);
typedef enum { TimerCancel, TimerPeriodic, TimerRelative } EFI_TIMER_DELAY;

typedef enum { EFI_NATIVE_INTERFACE } EFI_INTERFACE_TYPE;
typedef enum { AllHandles, ByRegisterNotify, ByProtocol } EFI_LOCATE_SEARCH_TYPE;

typedef struct { UINT8 Type; UINT8 SubType; UINT8 Length[2]; } EFI_DEVICE_PATH_PROTOCOL;

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x01
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x02
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x04
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x08
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x10
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x20

typedef struct {
    EFI_HANDLE AgentHandle; EFI_HANDLE ControllerHandle;
    UINT32 Attributes; UINT32 OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef struct {
    UINT16 Year;  UINT8 Month; UINT8 Day;
    UINT8 Hour;   UINT8 Minute; UINT8 Second; UINT8 Pad1;
    UINT32 Nanosecond;
    INT16  TimeZone;
    UINT8  Daylight; UINT8 Pad2;
} EFI_TIME;

#define EFI_UNSPECIFIED_TIMEZONE 0x07FF

typedef struct { UINT32 Resolution; UINT32 Accuracy; BOOLEAN SetsToZero; } EFI_TIME_CAPABILITIES;

typedef struct { UINT16 ScanCode; CHAR16 UnicodeChar; } EFI_INPUT_KEY;

#define SCAN_NULL 0x00
#define SCAN_UP 0x01
#define SCAN_DOWN 0x02
#define SCAN_RIGHT 0x03
#define SCAN_LEFT 0x04
#define SCAN_HOME 0x05
#define SCAN_END 0x06
#define SCAN_INSERT 0x07
#define SCAN_DELETE 0x08
#define SCAN_PAGE_UP 0x09
#define SCAN_PAGE_DOWN 0x0A
#define SCAN_F1 0x0B
#define SCAN_F2 0x0C
#define SCAN_F3 0x0D
#define SCAN_F4 0x0E
#define SCAN_F5 0x0F
#define SCAN_F6 0x10
#define SCAN_F7 0x11
#define SCAN_F8 0x12
#define SCAN_F9 0x13
#define SCAN_F10 0x14
#define SCAN_ESC 0x17

typedef struct EFI_SIMPLE_TEXT_INPUT EFI_SIMPLE_TEXT_INPUT;
struct EFI_SIMPLE_TEXT_INPUT {
    EFI_STATUS (EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT *, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT *, EFI_INPUT_KEY *Key);
    EFI_EVENT  WaitForKey;
};

#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN 0x03
#define EFI_RED 0x04
#define EFI_MAGENTA 0x05
#define EFI_BROWN 0x06
#define EFI_LIGHTGRAY 0x07
#define EFI_DARKGRAY 0x08
#define EFI_LIGHTBLUE 0x09
#define EFI_LIGHTGREEN 0x0A
#define EFI_LIGHTCYAN 0x0B
#define EFI_LIGHTRED 0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW 0x0E
#define EFI_WHITE 0x0F
#define EFI_TEXT_ATTR(fg, bg) ((fg) | ((bg) << 4))

typedef struct {
    INT32 MaxMode; INT32 Mode; INT32 Attribute;
    INT32 CursorColumn; INT32 CursorRow; BOOLEAN CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct EFI_SIMPLE_TEXT_OUTPUT EFI_SIMPLE_TEXT_OUTPUT;
struct EFI_SIMPLE_TEXT_OUTPUT {
    EFI_STATUS (EFIAPI *Reset)(EFI_SIMPLE_TEXT_OUTPUT *, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT *, CHAR16 *);
    EFI_STATUS (EFIAPI *TestString)(EFI_SIMPLE_TEXT_OUTPUT *, CHAR16 *);
    EFI_STATUS (EFIAPI *QueryMode)(EFI_SIMPLE_TEXT_OUTPUT *, UINTN Mode, UINTN *Columns, UINTN *Rows);
    EFI_STATUS (EFIAPI *SetMode)(EFI_SIMPLE_TEXT_OUTPUT *, UINTN Mode);
    EFI_STATUS (EFIAPI *SetAttribute)(EFI_SIMPLE_TEXT_OUTPUT *, UINTN Attribute);
    EFI_STATUS (EFIAPI *ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT *);
    EFI_STATUS (EFIAPI *SetCursorPosition)(EFI_SIMPLE_TEXT_OUTPUT *, UINTN Col, UINTN Row);
    EFI_STATUS (EFIAPI *EnableCursor)(EFI_SIMPLE_TEXT_OUTPUT *, BOOLEAN Visible);
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

#define EFI_GOP_GUID { 0x9042a9de, 0x23dc, 0x4a38, { 0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a } }

typedef enum { PixelRGBReserved8BitPerColor, PixelBGRReserved8BitPerColor,
               PixelBitMask, PixelBltOnly, PixelFormatMax } EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct { UINT32 RedMask, GreenMask, BlueMask, ReservedMask; } EFI_PIXEL_BITMASK;

typedef struct {
    UINT32 Version, HorizontalResolution, VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GOP_MODE_INFO;

typedef struct {
    UINT32 MaxMode, Mode;
    EFI_GOP_MODE_INFO *Info; UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase; UINTN FrameBufferSize;
} EFI_GOP_MODE;

typedef struct { UINT8 Blue, Green, Red, Reserved; } EFI_GOP_BLT_PIXEL;
typedef enum { EfiBltVideoFill, EfiBltVideoToBltBuffer, EfiBltBufferToVideo,
               EfiBltVideoToVideo, EfiGraphicsOutputBltOperationMax } EFI_GOP_BLT_OPERATION;

typedef struct EFI_GOP EFI_GOP;
struct EFI_GOP {
    EFI_STATUS (EFIAPI *QueryMode)(EFI_GOP *, UINT32 ModeNumber, UINTN *SizeOfInfo, EFI_GOP_MODE_INFO **Info);
    EFI_STATUS (EFIAPI *SetMode)(EFI_GOP *, UINT32 ModeNumber);
    EFI_STATUS (EFIAPI *Blt)(EFI_GOP *, EFI_GOP_BLT_PIXEL *BltBuffer, UINT32 BltOperation,
                             UINTN SourceX, UINTN SourceY, UINTN DestX, UINTN DestY,
                             UINTN Width, UINTN Height, UINTN Delta);
    EFI_GOP_MODE *Mode;
};

#define EFI_LOADED_IMAGE_GUID { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B } }

typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL *FilePath;
    void *Reserved;
    UINT32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    UINT64 ImageSize;
    UINT32 ImageCodeType;
    UINT32 ImageDataType;
    EFI_STATUS (EFIAPI *Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

#define EFI_SIMPLE_FILE_SYSTEM_GUID { 0x964E5B22, 0x6459, 0x11D2, { 0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B } }
#define EFI_FILE_INFO_GUID          { 0x09576E92, 0x6D3F, 0x11D2, { 0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B } }
#define EFI_FILE_SYSTEM_INFO_GUID   { 0x09576E93, 0x6D3F, 0x11D2, { 0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B } }

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

#define EFI_FILE_READ_ONLY  0x0000000000000001ULL
#define EFI_FILE_HIDDEN     0x0000000000000002ULL
#define EFI_FILE_SYSTEM     0x0000000000000004ULL
#define EFI_FILE_DIRECTORY  0x0000000000000010ULL
#define EFI_FILE_ARCHIVE    0x0000000000000020ULL

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    EFI_TIME CreateTime, LastAccessTime, ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

typedef struct {
    UINT64 Size; BOOLEAN ReadOnly;
    UINT64 VolumeSize, FreeSpace; UINT32 BlockSize;
    CHAR16 VolumeLabel[1];
} EFI_FILE_SYSTEM_INFO;

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
struct EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *Open)(EFI_FILE_PROTOCOL *, EFI_FILE_PROTOCOL **NewHandle,
                              CHAR16 *FileName, UINT64 OpenMode, UINT64 Attributes);
    EFI_STATUS (EFIAPI *Close)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (EFIAPI *Delete)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (EFIAPI *Read)(EFI_FILE_PROTOCOL *, UINTN *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *Write)(EFI_FILE_PROTOCOL *, UINTN *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *GetPosition)(EFI_FILE_PROTOCOL *, UINT64 *Position);
    EFI_STATUS (EFIAPI *SetPosition)(EFI_FILE_PROTOCOL *, UINT64 Position);
    EFI_STATUS (EFIAPI *GetInfo)(EFI_FILE_PROTOCOL *, EFI_GUID *InformationType, UINTN *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *SetInfo)(EFI_FILE_PROTOCOL *, EFI_GUID *InformationType, UINTN BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *Flush)(EFI_FILE_PROTOCOL *);
};

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *, EFI_FILE_PROTOCOL **Root);
};

typedef struct {
    EFI_TABLE_HEADER Hdr;

    UINTN      (EFIAPI *RaiseTPL)(EFI_TPL NewTpl);
    void       (EFIAPI *RestoreTPL)(EFI_TPL OldTpl);

    EFI_STATUS (EFIAPI *AllocatePages)(UINT32 AllocateType, UINT32 MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
    EFI_STATUS (EFIAPI *GetMemoryMap)(UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN *MapKey,
                                      UINTN *DescriptorSize, UINT32 *DescriptorVersion);
    EFI_STATUS (EFIAPI *AllocatePool)(UINT32 PoolType, UINTN Size, void **Buffer);
    EFI_STATUS (EFIAPI *FreePool)(void *Buffer);

    EFI_STATUS (EFIAPI *CreateEvent)(UINT32 Type, EFI_TPL NotifyTpl, EFI_EVENT_NOTIFY NotifyFunction,
                                     void *NotifyContext, EFI_EVENT *Event);
    EFI_STATUS (EFIAPI *SetTimer)(EFI_EVENT Event, UINT32 TimerDelay, UINT64 TriggerTime /* 100 ns units */);
    EFI_STATUS (EFIAPI *WaitForEvent)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
    EFI_STATUS (EFIAPI *SignalEvent)(EFI_EVENT Event);
    EFI_STATUS (EFIAPI *CloseEvent)(EFI_EVENT Event);
    EFI_STATUS (EFIAPI *CheckEvent)(EFI_EVENT Event);

    EFI_STATUS (EFIAPI *InstallProtocolInterface)(EFI_HANDLE *Handle, EFI_GUID *Protocol, UINT32 InterfaceType, void *Interface);
    EFI_STATUS (EFIAPI *ReinstallProtocolInterface)(EFI_HANDLE Handle, EFI_GUID *Protocol, void *OldInterface, void *NewInterface);
    EFI_STATUS (EFIAPI *UninstallProtocolInterface)(EFI_HANDLE Handle, EFI_GUID *Protocol, void *Interface);
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
    void       *Reserved;
    EFI_STATUS (EFIAPI *RegisterProtocolNotify)(EFI_GUID *Protocol, EFI_EVENT Event, void **Registration);
    EFI_STATUS (EFIAPI *LocateHandle)(UINT32 SearchType, EFI_GUID *Protocol, void *SearchKey,
                                      UINTN *BufferSize, EFI_HANDLE *Buffer);
    EFI_STATUS (EFIAPI *LocateDevicePath)(EFI_GUID *Protocol, EFI_DEVICE_PATH_PROTOCOL **DevicePath, EFI_HANDLE *Device);
    EFI_STATUS (EFIAPI *InstallConfigurationTable)(EFI_GUID *Guid, void *Table);

    EFI_STATUS (EFIAPI *LoadImage)(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle, EFI_DEVICE_PATH_PROTOCOL *DevicePath,
                                   void *SourceBuffer, UINTN SourceSize, EFI_HANDLE *ImageHandle);
    EFI_STATUS (EFIAPI *StartImage)(EFI_HANDLE ImageHandle, UINTN *ExitDataSize, CHAR16 **ExitData);
    EFI_STATUS (EFIAPI *Exit)(EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus, UINTN ExitDataSize, CHAR16 *ExitData);
    EFI_STATUS (EFIAPI *UnloadImage)(EFI_HANDLE ImageHandle);
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);

    EFI_STATUS (EFIAPI *GetNextMonotonicCount)(UINT64 *Count);
    EFI_STATUS (EFIAPI *Stall)(UINTN Microseconds);
    EFI_STATUS (EFIAPI *SetWatchdogTimer)(UINTN Timeout, UINT64 WatchdogCode, UINTN DataSize, CHAR16 *WatchdogData);

    EFI_STATUS (EFIAPI *ConnectController)(EFI_HANDLE ControllerHandle, EFI_HANDLE *DriverImageHandle,
                                           EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath, BOOLEAN Recursive);
    EFI_STATUS (EFIAPI *DisconnectController)(EFI_HANDLE ControllerHandle, EFI_HANDLE DriverImageHandle, EFI_HANDLE ChildHandle);

    EFI_STATUS (EFIAPI *OpenProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface,
                                      EFI_HANDLE AgentHandle, EFI_HANDLE ControllerHandle, UINT32 Attributes);
    EFI_STATUS (EFIAPI *CloseProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, EFI_HANDLE AgentHandle, EFI_HANDLE ControllerHandle);
    EFI_STATUS (EFIAPI *OpenProtocolInformation)(EFI_HANDLE Handle, EFI_GUID *Protocol,
                                                 EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer, UINTN *EntryCount);

    EFI_STATUS (EFIAPI *ProtocolsPerHandle)(EFI_HANDLE Handle, EFI_GUID ***ProtocolBuffer, UINTN *ProtocolBufferCount);
    EFI_STATUS (EFIAPI *LocateHandleBuffer)(UINT32 SearchType, EFI_GUID *Protocol, void *SearchKey,
                                            UINTN *NoHandles, EFI_HANDLE **Buffer);
    EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
    EFI_STATUS (EFIAPI *InstallMultipleProtocolInterfaces)(EFI_HANDLE *Handle, ...);
    EFI_STATUS (EFIAPI *UninstallMultipleProtocolInterfaces)(EFI_HANDLE Handle, ...);
    EFI_STATUS (EFIAPI *CalculateCrc32)(void *Data, UINTN DataSize, UINT32 *Crc32);
    void       (EFIAPI *CopyMem)(void *Destination, void *Source, UINTN Length);
    void       (EFIAPI *SetMem)(void *Buffer, UINTN Size, UINT8 Value);
    EFI_STATUS (EFIAPI *CreateEventEx)(UINT32 Type, EFI_TPL NotifyTpl, EFI_EVENT_NOTIFY NotifyFunction,
                                       const void *NotifyContext, const EFI_GUID *EventGroup, EFI_EVENT *Event);
} EFI_BOOT_SERVICES;

typedef enum { EfiResetCold, EfiResetWarm, EfiResetShutdown, EfiResetPlatformSpecific } EFI_RESET_TYPE;

typedef struct { EFI_GUID CapsuleGuid; UINT32 HeaderSize; UINT32 Flags; UINT32 CapsuleImageSize; } EFI_CAPSULE_HEADER;

#define EFI_VARIABLE_NON_VOLATILE       0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x00000004

#define EFI_GLOBAL_VARIABLE_GUID { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA,0x0D,0x00,0xE0,0x98,0x03,0x2B,0x8C } }

typedef struct {
    EFI_TABLE_HEADER Hdr;

    EFI_STATUS (EFIAPI *GetTime)(EFI_TIME *Time, EFI_TIME_CAPABILITIES *Capabilities);
    EFI_STATUS (EFIAPI *SetTime)(EFI_TIME *Time);
    EFI_STATUS (EFIAPI *GetWakeupTime)(BOOLEAN *Enabled, BOOLEAN *Pending, EFI_TIME *Time);
    EFI_STATUS (EFIAPI *SetWakeupTime)(BOOLEAN Enable, EFI_TIME *Time);

    EFI_STATUS (EFIAPI *SetVirtualAddressMap)(UINTN MemoryMapSize, UINTN DescriptorSize, UINT32 DescriptorVersion,
                                              EFI_MEMORY_DESCRIPTOR *VirtualMap);
    EFI_STATUS (EFIAPI *ConvertPointer)(UINTN DebugDisposition, void **Address);

    EFI_STATUS (EFIAPI *GetVariable)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 *Attributes,
                                     UINTN *DataSize, void *Data);
    EFI_STATUS (EFIAPI *GetNextVariableName)(UINTN *VariableNameSize, CHAR16 *VariableName, EFI_GUID *VendorGuid);
    EFI_STATUS (EFIAPI *SetVariable)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 Attributes,
                                     UINTN DataSize, void *Data);

    EFI_STATUS (EFIAPI *GetNextHighMonotonicCount)(UINT32 *HighCount);
    void       (EFIAPI *ResetSystem)(UINT32 ResetType, EFI_STATUS ResetStatus, UINTN DataSize, void *ResetData);

    EFI_STATUS (EFIAPI *UpdateCapsule)(EFI_CAPSULE_HEADER **CapsuleHeaderArray, UINTN CapsuleCount,
                                       EFI_PHYSICAL_ADDRESS ScatterGatherList);
    EFI_STATUS (EFIAPI *QueryCapsuleCapabilities)(EFI_CAPSULE_HEADER **CapsuleHeaderArray, UINTN CapsuleCount,
                                                  UINT64 *MaximumCapsuleSize, UINT32 *ResetType);
    EFI_STATUS (EFIAPI *QueryVariableInfo)(UINT32 Attributes, UINT64 *MaximumVariableStorageSize,
                                           UINT64 *RemainingVariableStorageSize, UINT64 *MaximumVariableSize);
} EFI_RUNTIME_SERVICES;

#define EFI_ACPI_20_TABLE_GUID { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81 } }
#define EFI_ACPI_TABLE_GUID    { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d } }
#define EFI_SMBIOS_TABLE_GUID  { 0xeb9d2d31, 0x2d88, 0x11d3, { 0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d } }
#define EFI_SMBIOS3_TABLE_GUID { 0xf2fd1544, 0x9794, 0x4a2c, { 0x99,0x2e,0xe5,0xbb,0xcf,0x20,0xe3,0x94 } }

typedef struct { EFI_GUID VendorGuid; void *VendorTable; } EFI_CONFIGURATION_TABLE;

struct EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32  FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;        EFI_SIMPLE_TEXT_INPUT  *ConIn;
    EFI_HANDLE ConsoleOutHandle;       EFI_SIMPLE_TEXT_OUTPUT *ConOut;
    EFI_HANDLE StandardErrorHandle;    EFI_SIMPLE_TEXT_OUTPUT *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES    *BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
};

#endif
