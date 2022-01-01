#if !defined(TINYSERVER_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define TINYSERVER_H

#define Round(Size, Boundary) (((Size) - 1) & (Boundary)) + (Boundary)

typedef enum
{
    IoStage_None,
    IoStage_Accepting,
    IoStage_Reading,
    IoStage_Sending,
    IoStage_Closing
} io_stage;

typedef struct
{
    request Request;
    response Response;
} http_info;

typedef struct _socket_io
{
    u8 OSData[80];
    usz Socket;
    struct _socket_io* Prev;
    struct _socket_io* Next;
    
    char* InBuffer;
    usz InBufferSize;
    usz InBufferReservedSize;
    
    usz InBytes;
    usz OutBytes;
    
    io_stage Stage;
    http_info Http;
} socket_io;

typedef void (*module_main)(void*);
typedef struct
{
    usz Library;
    string BasePath;
    string LibraryName;
    usz LastLibraryWriteTime;
    module_main Module;
} app_info;

typedef struct
{
    u16 ServerPort;
    string FilesPath;
    
    usz PageSize;
    usz MemBoundary;
    usz SystemThreads;
    
    u32 SocketsMin;
    u32 SocketsMax;
    u32 RecvThreadsMin;
    u32 RecvThreadsMax;
    u32 SendThreadsMin;
    u32 SendThreadsMax;
    
    usz InHeaderMaxSize;
    usz InBodyMaxSize;
    usz InBufferMaxSize;
    usz InBufferInitSize;
    
    usz OutHeaderMaxSize;
    usz OutCookiesMaxSize;
    usz OutBodyMaxSize;
    
    app_info App;
} server_info;

typedef enum
{
    SysError_Ok,
    SysError_FileNotFound,
    SysError_Unknown
} sys_error;

typedef struct
{
    usz Handle;
    usz Size;
    sys_error Error;
} file_handle;

typedef struct
{
    u64 Start;
    u64 End;
    u64 Diff;
} timing_info;

#endif
