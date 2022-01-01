#ifndef WIN32_TINYSERVER_H
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define WIN32_TINYSERVER_H

#define MIN_SOCKETS 10
#define MAX_SOCKETS SOMAXCONN

#define WSAFailure(Exp) (Exp) != 0
#define WSASuccess(Exp) (Exp) == 0

#define WIN32_LEAN_AND_MEAN

#ifndef __FLTUSED__
#define __FLTUSED__
extern __declspec(selectany) int _fltused=1;
#endif

#define QUEUE_SIZE 256
#define MAX_PATH_SIZE MAX_PATH

typedef BOOL (*fn_disconnect_ex)(SOCKET, OVERLAPPED*, DWORD, DWORD);
global fn_disconnect_ex DisconnectEx = NULL;

typedef struct
{
    WSAOVERLAPPED Overlapped;
    WSABUF Buffer1;
    WSABUF Buffer2;
    WSABUF Buffer3;
} win_io;

inline WSABUF
WSABuf(char* Ptr, usz Size)
{
    WSABUF Result = { (ULONG)Size, Ptr };
    return Result;
}

typedef struct
{
    CRITICAL_SECTION Lock;
    
    volatile long UsedSockets;
    volatile long FreeSockets;
    struct _socket_io* UsedListEnd;
    struct _socket_io* FreeListEnd;
} server_sockets;

typedef struct
{
    CRITICAL_SECTION Lock;
    
    volatile long Count;
    volatile long ReadIdx;
    volatile long WriteIdx;
    struct _socket_io* Sockets[QUEUE_SIZE];
} work_queue;

typedef struct
{
    work_queue RecvQueue;
    work_queue SendQueue;
    HANDLE RecvSemaphore;
    HANDLE SendSemaphore;
} server_work_queues;

typedef struct
{
    HANDLE CompletionPort;
    server_work_queues* WorkQueues;
} io_params;

typedef struct
{
    u32 ThreadId;
    
    timing_info ClockActive;
    timing_info ClockWaiting;
    
    volatile long TimeActive;
    volatile long TimeWaiting;
} thread_trace;

typedef struct
{
    usz IOCount;
    usz RecvCount;
    usz SendCount;
    
    thread_trace IO[16];
    thread_trace Recv[96];
    thread_trace Send[96];
} diagnostics;


internal void AcceptPackage(usz ListeningSocket, socket_io* Client);
internal void AddSocket(server_sockets* Server, socket_io* Socket);
internal void AddSocketAndUse(server_sockets* Server, socket_io* Socket);
inline void ClearBuffer(void* Address, usz SizeToClear);
internal void CloseFileHandle(usz FileHandle);
internal void CloseSocket(socket_io* Conn);
internal void DeleteUsedSocket(server_sockets* Server, socket_io* Socket);
internal void DisconnectSocket(socket_io* SocketIO);
inline void DuplicateFile(char* SrcPath, char* DstPath);
internal bool ExpandMemory(void* Buffer, usz CurrentSize, usz MaxSize, usz AmountToExpand);
internal void FreeMemoryFromHeap(void* Address);
internal void FreeMemoryFromSystem(void* Address);
inline time_format GetCurrentSystemTime(void);
internal usz GetFileLastWriteTime(char* Filename);
internal void* GetMemoryFromHeap(usz SizeToAllocate);
internal void* GetMemoryFromSystem(usz MemSize);
inline u32 GetSystemErrorCode(void);
internal bool IsExistingPath(string Path);
inline usz LoadExternalLibrary(char* LibPath);
inline void* LoadExternalSymbol(usz Library, char* Symbol);
internal file_handle OpenFileHandle(char* Path);
internal void PrintToConsole(char* Format, ...);
internal bool ReadFileIntoBuffer(file_handle File, char* Buffer);
inline u64 Record(timing_info*);
internal void RecvPackage(socket_io* Client);
internal void* ReserveMemory(usz SizeToReserve);
internal void SendPackage(socket_io* Client, u32 BufferCount);
internal void ShrinkMemory(void* Address, usz AmountToShrinkBy);
inline void StartTiming(timing_info* Info);
inline void StopTiming(timing_info* Info);
inline void UnloadExternalLibrary(usz Library);
internal socket_io* UseSocket(server_sockets* Server);

#endif
