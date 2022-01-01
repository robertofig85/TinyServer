/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

/*==============================================================================================
  ..:: Lista de tarefas ::..

Features:
  [ ] Criar thread especifica da aplicacao.
  [X] Melhorar header de saida.
  [ ] HTTP 2.0.
  [ ] SSH.
  [ ] Ler configs do servidor de um arquivo externo.
  [ ] Re-ler configs de quanto em quanto tempo e ajustar dados do servidor de acordo.
  [ ] Sistema externo de config do servidor remotamente (GUI).

Desempenho:
  [ ] Cache LRU e cache-control do header.
  [ ] Lista circular de InBuffers (max de conns possiveis, na casa dos TBs de memoria virtual).
  [ ] Pegar buffer inicial do AcceptEx() da lista acima, usar soquete so com a conexao aceita.
  [ ] GZip da resposta.
  [ ] Ajustar numero de soquetes e threads por previsao (no loop principal).
===============================================================================================*/


#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <strsafe.h>

#include "tinyserver-types.h"
#include "tinyserver-strings.h"
#include "tinyserver-http.h"
#include "tinyserver.h"
#include "tinyserver-external.h"
#include "win32-tinyserver.h"

global HANDLE GlobalHeap;
global HANDLE GlobalConsole;
global HANDLE GlobalLogFile;

global diagnostics GlobalTracer;
global server_info* GlobalServerInfo;
global server_sockets GlobalSockets;

global volatile long GlobalRecvThreadId = 0;
global volatile long GlobalSendThreadId = 0;
global volatile long GlobalIOThreadId = 0;

global volatile long GlobalAcceptCount = 0;
global volatile long GlobalRecvCount = 0;
global volatile long GlobalSendCount = 0;
global volatile long GlobalDisconnectCount = 0;

global char* GlobalErrorMessage = "";

#include "tinyserver-strings.c"
#include "tinyserver-http.c"
#include "tinyserver-external.c"
#include "tinyserver-config.c"
#include "tinyserver.c"

//===============================
// DEBUG
//===============================

internal void
Log(char* Format, ...)
{
    char Buffer[256] = {0};
    va_list Args;
    va_start(Args, Format);
    u32 OutputSize = wvsprintf(Buffer, Format, Args);
    va_end(Args);
    
    WriteFile(GlobalLogFile, Buffer, OutputSize, NULL, NULL);
}

internal void
PrintToConsole(char* Format, ...)
{
    char Buffer[1024] = {0};
    va_list Args;
    va_start(Args, Format);
    u32 OutputSize = wvsprintf(Buffer, Format, Args);
    va_end(Args);
    
    LPDWORD BytesWritten = NULL;
    WriteConsole(GlobalConsole, Buffer, OutputSize, BytesWritten, NULL);
}

internal void
DEBUGPrintSockAddr(usz Sock)
{
    struct sockaddr_in HostInfo = {0};
    int HostInfoSize = sizeof(HostInfo);
    struct sockaddr_in PeerInfo = {0};
    int PeerInfoSize = sizeof(PeerInfo);
    
    if (WSASuccess(getsockname(Sock, (struct sockaddr*)&HostInfo, &HostInfoSize))
        && WSASuccess(getpeername(Sock, (struct sockaddr*)&PeerInfo, &PeerInfoSize)))
    {
        PrintToConsole("%u\tHost: %d.%d.%d.%d:%d\tPeer: %d.%d.%d.%d:%d\n",
                       Sock,
                       HostInfo.sin_addr.S_un.S_un_b.s_b1,
                       HostInfo.sin_addr.S_un.S_un_b.s_b2,
                       HostInfo.sin_addr.S_un.S_un_b.s_b3,
                       HostInfo.sin_addr.S_un.S_un_b.s_b4,
                       HostInfo.sin_port,
                       PeerInfo.sin_addr.S_un.S_un_b.s_b1,
                       PeerInfo.sin_addr.S_un.S_un_b.s_b2,
                       PeerInfo.sin_addr.S_un.S_un_b.s_b3,
                       PeerInfo.sin_addr.S_un.S_un_b.s_b4,
                       PeerInfo.sin_port);
    }
    else
    {
        PrintToConsole("Getpeername failed (%d).\n", WSAGetLastError());
    }
}

//===============================
// Timing_Info
//===============================

inline void
StartTiming(timing_info* Info)
{
#ifndef _NOTIMING_
    Info->End = 0;
    Info->Start = __rdtsc();
#endif
}

inline void
StopTiming(timing_info* Info)
{
#ifndef _NOTIMING_
    Info->End = __rdtsc();
    Info->Diff += (Info->End - Info->Start);
    Info->Start = 0;
#endif
}

inline u64
Record(timing_info* Info)
{
    u64 Result = 0;
    if (Info->Start > 0)
    {
        StopTiming(Info);
        Result = Info->Diff;
        Info->Diff = 0;
        StartTiming(Info);
    }
    else
    {
        Result = Info->Diff;
        Info->Diff = 0;
    }
    return Result;
}

inline time_format
GetCurrentSystemTime(void)
{
    SYSTEMTIME SystemTime;
    GetSystemTime(&SystemTime);
    
    time_format TimeFormat = { SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
        SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, (week_day)SystemTime.wDayOfWeek };
    
    return TimeFormat;
}

//=============================
// Memoria
//=============================

internal void*
GetMemoryFromSystem(usz MemSize)
{
    void* Result = VirtualAlloc(NULL, MemSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    return Result;
}

internal void
FreeMemoryFromSystem(void* Address)
{
    if (Address)
    {
        VirtualFree(Address, 0, MEM_RELEASE);
    }
}

internal void*
ReserveMemory(usz SizeToReserve)
{
    void* Result = VirtualAlloc(NULL, SizeToReserve, MEM_RESERVE, PAGE_NOACCESS);
    return Result;
}

internal bool
ExpandMemory(void* Buffer, usz CurrentSize, usz MaxSize, usz AmountToExpand)
{
    bool Result = false;
    if (CurrentSize + AmountToExpand <= MaxSize)
    {
        Buffer = VirtualAlloc((u8*)Buffer + CurrentSize, AmountToExpand, MEM_COMMIT, PAGE_READWRITE);
        if (Buffer)
        {
            Result = true;
        }
    }
    return Result;
}

internal void
ShrinkMemory(void* Address, usz AmountToShrinkBy)
{
    if (Address)
    {
        VirtualFree(Address, AmountToShrinkBy, MEM_DECOMMIT);
    }
}

internal void*
GetMemoryFromHeap(usz SizeToAllocate)
{
    void* Result = HeapAlloc(GlobalHeap, 0, SizeToAllocate);
    return Result;
}

internal void
FreeMemoryFromHeap(void* Address)
{
    if (Address)
    {
        HeapFree(GlobalHeap, 0, Address);
    }
}

inline void
ClearBuffer(void* Address, usz SizeToClear)
{
    memset(Address, 0, SizeToClear);
}

//========================================
// Arquivos
//========================================

internal file_handle
OpenFileHandle(char* Path)
{
    file_handle Result = {0};
    
    HANDLE File = CreateFileA(Path,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        DWORD FileSizeHigh;
        DWORD FileSizeLow = GetFileSize(File, &FileSizeHigh);
        if (FileSizeHigh || FileSizeLow)
        {
            Result.Handle = (usz)File;
            Result.Size = (usz)FileSizeLow | ((usz)FileSizeHigh << 32);
            Result.Error = SysError_Ok;
        }
    }
    else
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            Result.Error = SysError_FileNotFound;
        }
        else
        {
            Result.Error = SysError_Unknown;
        }
    }
    
    return Result;
}

internal void
CloseFileHandle(usz FileHandle)
{
    CloseHandle((HANDLE)FileHandle);
}

internal usz
GetFileLastWriteTime(char* Filename)
{
    usz Result = {0};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        FILETIME Filetime = Data.ftLastWriteTime;
        Result = (usz)Filetime.dwLowDateTime | ((usz)Filetime.dwHighDateTime << sizeof(u32));
    }
    return Result;
}

inline void
DuplicateFile(char* SrcPath, char* DstPath)
{
    CopyFile(SrcPath, DstPath, FALSE);
}

internal bool
ReadFileIntoBuffer(file_handle File, char* Buffer)
{
    bool ReadSuccess = false;
    
    DWORD BytesRead = 0;
    if (Buffer
        && ReadFile((HANDLE)File.Handle, Buffer, (DWORD)File.Size, &BytesRead, NULL)
        && BytesRead == File.Size)
    {
        ReadSuccess = true;
    }
    
    return ReadSuccess;
}

internal bool
MakeDirectory(char* Path)
{
    bool Result = CreateDirectoryA(Path, NULL);
    return Result;
}

internal bool
IsExistingPath(string Path)
{
    bool Result = false;
    
    char FullPath[MAX_PATH] = {0};
    CopyData(FullPath, ArrayCount(FullPath), Path.Ptr, Path.Size);
    
    DWORD PathAttributes = GetFileAttributesA(FullPath);
    if (PathAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        Result = true;
    }
    
    return Result;
}

//============================================
// Misc
//============================================

inline usz
LoadExternalLibrary(char* LibPath)
{
    usz Library = (usz)LoadLibraryA(LibPath);
    return Library;
}

inline void*
LoadExternalSymbol(usz Library, char* SymbolName)
{
    void* Symbol = (void*)GetProcAddress((HMODULE)Library, SymbolName);
    return Symbol;
}

inline void
UnloadExternalLibrary(usz Library)
{
    FreeLibrary((HMODULE)Library);
}

inline u32
GetSystemErrorCode(void)
{
    u32 Result = GetLastError();
    return Result;
}

//================
// Server_Sockets
//================

internal void
AddSocket(server_sockets* Server, socket_io* Socket)
{
    EnterCriticalSection(&Server->Lock);
    {
        if (Server->FreeSockets > 0)
        {
            Server->FreeListEnd->Next = Socket;
            Socket->Prev = Server->FreeListEnd;
        }
        _WriteBarrier_;
        Server->FreeListEnd = Socket;
        InterlockedIncrement(&Server->FreeSockets);
    }
    LeaveCriticalSection(&Server->Lock);
}

internal void
AddSocketAndUse(server_sockets* Server, socket_io* Socket)
{
    EnterCriticalSection(&Server->Lock);
    {
        if (Server->UsedSockets > 0)
        {
            Server->UsedListEnd->Next = Socket;
            Socket->Prev = Server->UsedListEnd;
        }
        _WriteBarrier_;
        Server->UsedListEnd = Socket;
        InterlockedIncrement(&Server->UsedSockets);
    }
    LeaveCriticalSection(&Server->Lock);
}

internal socket_io*
UseSocket(server_sockets* Server)
{
    socket_io* Result = NULL;
    
    EnterCriticalSection(&Server->Lock);
    if (Server->FreeSockets > 0)
    {
        Result = Server->FreeListEnd;
        if (Server->FreeListEnd->Prev)
        {
            Server->FreeListEnd->Prev->Next = NULL;
        }
        _WriteBarrier_;
        Server->FreeListEnd = Server->FreeListEnd->Prev;
        InterlockedDecrement(&Server->FreeSockets);
        
        if (Server->UsedSockets > 0)
        {
            Server->UsedListEnd->Next = Result;
        }
        Result->Prev = Server->UsedListEnd;
        _WriteBarrier_;
        Server->UsedListEnd = Result;
        InterlockedIncrement(&Server->UsedSockets);
    }
    else
    {
        Log("-- UseSocket with no free socket --");
    }
    LeaveCriticalSection(&Server->Lock);
    
    return Result;
}

internal void
FreeSocket(server_sockets* Server, socket_io* Socket)
{
    EnterCriticalSection(&Server->Lock);
    if (Server->UsedSockets > 0)
    {
        if (Socket->Prev)
        {
            Socket->Prev->Next = Socket->Next;
        }
        if (Socket->Next)
        {
            Socket->Next->Prev = Socket->Prev;
        }
        if (Server->UsedListEnd == Socket)
        {
            Server->UsedListEnd = Socket->Prev;
        }
        InterlockedDecrement(&Server->UsedSockets);
        
        if (Server->FreeSockets > 0)
        {
            Server->FreeListEnd->Next = Socket;
        }
        _WriteBarrier_;
        Socket->Prev = Server->FreeListEnd;
        Socket->Next = NULL;
        Server->FreeListEnd = Socket;
        InterlockedIncrement(&Server->FreeSockets);
    }
    else
    {
        _UnreachableCode_
    }
    LeaveCriticalSection(&Server->Lock);
}

internal void
DeleteUsedSocket(server_sockets* Server, socket_io* Socket)
{
    EnterCriticalSection(&Server->Lock);
    {
        if (Socket->Prev)
        {
            Socket->Prev->Next = Socket->Next;
        }
        if (Socket->Next)
        {
            Socket->Next->Prev = Socket->Prev;
        }
        InterlockedDecrement(&Server->UsedSockets);
    }
    LeaveCriticalSection(&Server->Lock);
}

internal void
DeleteFreeSocket(server_sockets* Server, socket_io* Socket)
{
    EnterCriticalSection(&Server->Lock);
    {
        if (Socket->Prev)
        {
            Socket->Prev->Next = Socket->Next;
        }
        if (Socket->Next)
        {
            Socket->Next->Prev = Socket->Prev;
        }
        InterlockedDecrement(&Server->FreeSockets);
    }
    LeaveCriticalSection(&Server->Lock);
}

//============
// Work_Queue
//============

internal bool
PushQueue(work_queue* Queue, socket_io* NewEntry)
{
    bool Result = false;
    
    EnterCriticalSection(&Queue->Lock);
    if (Queue->Count < QUEUE_SIZE)
    {
        Queue->Sockets[Queue->WriteIdx] = NewEntry;
        Queue->WriteIdx = (Queue->WriteIdx + 1) & (QUEUE_SIZE - 1);
        Result = true;
        InterlockedIncrement(&Queue->Count);
    }
    LeaveCriticalSection(&Queue->Lock);
    
    return Result;
}

internal socket_io*
PopQueue(work_queue* Queue)
{
    socket_io* Result = NULL;
    
    EnterCriticalSection(&Queue->Lock);
    if (Queue->Count < QUEUE_SIZE)
    {
        Result = Queue->Sockets[Queue->ReadIdx];
        Queue->Sockets[Queue->ReadIdx] = NULL;
        Queue->ReadIdx = (Queue->ReadIdx + 1) & (QUEUE_SIZE - 1);
        InterlockedDecrement(&Queue->Count);
    }
    LeaveCriticalSection(&Queue->Lock);
    
    return Result;
}

//=============================================
// Socket IO
//=============================================

internal usz
CreateNewSocket(HANDLE CompletionPort)
{
    usz NewSocket = INVALID_SOCKET;
    
    SOCKET Socket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (Socket != INVALID_SOCKET
        && CreateIoCompletionPort((HANDLE)Socket, CompletionPort, (ULONG_PTR)NULL, 0))
    {
        NewSocket = (usz)Socket;
    }
    
    return NewSocket;
}
internal void
ResetSocket(socket_io* SocketIO)
{
    socket_io CleanSocket = {0};
    
    CleanSocket.Socket = SocketIO->Socket;
    CleanSocket.Prev = SocketIO->Prev;
    CleanSocket.Next = SocketIO->Next;
    
    ShrinkMemory(SocketIO->InBuffer + GlobalServerInfo->InBufferInitSize,
                 SocketIO->InBufferReservedSize - GlobalServerInfo->InBufferInitSize);
    ClearBuffer(SocketIO->InBuffer, GlobalServerInfo->InBufferInitSize);
    CleanSocket.InBuffer = SocketIO->InBuffer;
    CleanSocket.InBufferReservedSize = SocketIO->InBufferReservedSize;
    CleanSocket.InBufferSize = GlobalServerInfo->InBufferInitSize;
    
    response* Response = &SocketIO->Http.Response;
    ClearBuffer(Response->Header, Response->HeaderSize);
    CleanSocket.Http.Response.Header = Response->Header;
    CleanSocket.Http.Response.HeaderSize = GlobalServerInfo->OutHeaderMaxSize;
    
    if (Response->ContentSize > 0)
    {
        FreeMemoryFromSystem(Response->Content);
        if (Response->FileHandle)
        {
            CloseFileHandle(Response->FileHandle);
        }
    }
    
    if (Response->CookiesSize > 0)
    {
        FreeMemoryFromSystem(Response->Cookies);
    }
    
    *SocketIO = CleanSocket;
}

internal void
TerminateSocket(socket_io* SocketIO)
{
    FreeMemoryFromSystem(SocketIO->InBuffer);
    FreeMemoryFromSystem(SocketIO->Http.Response.Header);
    
    if (SocketIO->Http.Response.ContentSize > 0)
    {
        FreeMemoryFromSystem(SocketIO->Http.Response.Content);
        if (SocketIO->Http.Response.FileHandle)
        {
            CloseFileHandle(SocketIO->Http.Response.FileHandle);
        }
    }
    
    if (SocketIO->Http.Response.CookiesSize > 0)
    {
        FreeMemoryFromSystem(SocketIO->Http.Response.Cookies);
    }
    
    closesocket((SOCKET)SocketIO->Socket);
    DeleteUsedSocket(&GlobalSockets, SocketIO);
    FreeMemoryFromHeap(SocketIO);
}

internal void
AcceptPackage(usz ListeningSocket, socket_io* Client)
{
    local DWORD LocalAddrSize = 0x10 + sizeof(struct sockaddr_in);
    local DWORD RemoteAddrSize = 0x10 + sizeof(struct sockaddr_in);
    
    InterlockedIncrement(&GlobalAcceptCount);
    
    win_io* WinIO = (win_io*)&Client->OSData[0];
    WSAOVERLAPPED* Overlapped = &WinIO->Overlapped;
    Client->Stage = IoStage_Accepting;
    
    _WriteBarrier_;
    
    DWORD BytesRecv = 0;
    DWORD InDataSize = (DWORD)(GlobalServerInfo->InBufferInitSize - LocalAddrSize - RemoteAddrSize);
    if (WSAFailure(AcceptEx((SOCKET)ListeningSocket, (SOCKET)Client->Socket, Client->InBuffer,
                            InDataSize, LocalAddrSize, RemoteAddrSize, &BytesRecv, Overlapped)))
    {
        int Error = WSAGetLastError();
        if (Error == ERROR_IO_PENDING)
        {
            // OBS(roberto): Operacao correta, nao precisa fazer nada.
        }
        else
        {
            DisconnectSocket(Client);
        }
    }
}

internal void
RecvPackage(socket_io* Client)
{
    win_io* WinIO = (win_io*)&Client->OSData[0];
    WSAOVERLAPPED* Overlapped = &WinIO->Overlapped; 
    ClearBuffer(Overlapped, sizeof(WSAOVERLAPPED));
    WSABUF* Data = &WinIO->Buffer1;
    
    DWORD Flags = 0;
    if (WSAFailure(WSARecv((SOCKET)Client->Socket, Data, 1, NULL, &Flags, Overlapped, NULL)))
    {
        int Error = WSAGetLastError();
        switch (Error)
        {
            case WSAEMSGSIZE:
            case WSA_IO_PENDING:
            {
                // OBS(roberto): Operacao correta, nao precisa fazer nada.
            } break;
            
            case WSAEWOULDBLOCK:
            {
                // AFAZER(roberto): Apendar buffers em estrutura propria?!
            } break;
            
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAENETDOWN:
            case WSAENETRESET:
            {
                DisconnectSocket(Client);
            } break;
            
            default:
            {
                TerminateSocket(Client);
            } break;
        }
    }
}

internal void
SendPackage(socket_io* Client, u32 BufferIdx)
{
    win_io* WinIO = (win_io*)&Client->OSData[0];
    WSAOVERLAPPED* Overlapped = &WinIO->Overlapped; 
    ClearBuffer(Overlapped, sizeof(WSAOVERLAPPED));
    WSABUF* Buffers = (&WinIO->Buffer1) + BufferIdx;
    ULONG BufferCount = 3 - BufferIdx;
    
    if (WSAFailure(WSASend((SOCKET)Client->Socket, Buffers, BufferCount, NULL, 0, Overlapped, NULL)))
    {
        int Error = WSAGetLastError();
        switch (Error)
        {
            case WSA_IO_PENDING:
            {
                // OBS(roberto): Operacao correta, nao precisa fazer nada.
            } break;
            
            case WSAEWOULDBLOCK:
            case WSAENOBUFS:
            {
                // AFAZER(roberto): Apendar buffers em estrutura propria?!
            } break;
            
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAENETDOWN:
            case WSAENETRESET:
            {
                DisconnectSocket(Client);
            } break;
            
            default:
            {
                TerminateSocket(Client);
            } break;
        }
    }
}

internal void
DisconnectSocket(socket_io* Client)
{
    InterlockedIncrement(&GlobalDisconnectCount);
    
    ClearBuffer(&Client->OSData, sizeof(Client->OSData));
    Client->Stage = IoStage_Closing;
    if (DisconnectEx((SOCKET)Client->Socket, (WSAOVERLAPPED*)&Client->OSData, TF_REUSE_SOCKET, 0))
    {
        int Error = WSAGetLastError();
        switch (Error)
        {
            case WSA_IO_PENDING:
            {
                // OBS(roberto): Operacao correta, nao precisa fazer nada.
            } break;
            
            case WSAENOTCONN:
            {
                GlobalErrorMessage = "WSAENotConn";
            } break;
            
            default:
            {
                TerminateSocket(Client);
            } break;
        }
    }
}
//==========================================
// Threads
//==========================================


DWORD WINAPI
TimingThread(void* Param)
{
    for (;;)
    {
        Sleep(1000);
        
        PrintToConsole("Sockets: Used (%d), Free (%d)\n", GlobalSockets.UsedSockets, GlobalSockets.FreeSockets);
        PrintToConsole("Actions: Accept (%d), Recv (%d), Send (%d), Disconnect (%d)\n",
                       GlobalAcceptCount, GlobalRecvCount, GlobalSendCount, GlobalDisconnectCount);
        PrintToConsole("Error: %s\n\n", GlobalErrorMessage);
        
        for (usz Idx = 0; Idx < GlobalTracer.IOCount; Idx++)
        {
            u64 TimeWaiting = Record(&GlobalTracer.IO[Idx].ClockWaiting);
            u64 TimeActive = Record(&GlobalTracer.IO[Idx].ClockActive);
            u64 TimeTotal = TimeActive + TimeWaiting;
            PrintToConsole("IoCtl Thread %d:\t%d.%.3d / %d.%.3d\n", GlobalTracer.IO[Idx].ThreadId,
                           (u32)(TimeActive * 100 / TimeTotal), (u32)((TimeActive * 100) % TimeTotal),
                           (u32)(TimeWaiting * 100 / TimeTotal), (u32)((TimeWaiting * 100) % TimeTotal));
        }
        
        PrintToConsole("\n");
        
        for (usz Idx = 0; Idx < GlobalTracer.RecvCount; Idx++)
        {
            u64 TimeWaiting = Record(&GlobalTracer.Recv[Idx].ClockWaiting);
            u64 TimeActive = Record(&GlobalTracer.Recv[Idx].ClockActive);
            u64 TimeTotal = TimeActive + TimeWaiting;
            PrintToConsole("Recv Thread %d:\t%d.%.3d / %d.%.3d\n", GlobalTracer.Recv[Idx].ThreadId,
                           (u32)(TimeActive * 100 / TimeTotal), (u32)((TimeActive * 100) % TimeTotal),
                           (u32)(TimeWaiting * 100 / TimeTotal), (u32)((TimeWaiting * 100) % TimeTotal));
        }
        
        PrintToConsole("\n");
        
        for (usz Idx = 0; Idx < GlobalTracer.SendCount; Idx++)
        {
            u64 TimeWaiting = Record(&GlobalTracer.Send[Idx].ClockWaiting);
            u64 TimeActive = Record(&GlobalTracer.Send[Idx].ClockActive);
            u64 TimeTotal = TimeActive + TimeWaiting;
            PrintToConsole("Send Thread %d:\t%d.%.3d / %d.%.3d\n", GlobalTracer.Send[Idx].ThreadId,
                           (u32)(TimeActive * 100 / TimeTotal), (u32)((TimeActive * 100) % TimeTotal),
                           (u32)(TimeWaiting * 100 / TimeTotal), (u32)((TimeWaiting * 100) % TimeTotal));
        }
        
        PrintToConsole("\n======================================\n\n");
    }
}

DWORD WINAPI
RecvThread(LPVOID Param)
{
    server_work_queues* WorkQueues = (server_work_queues*)Param;
    work_queue* RecvQueue = &WorkQueues->RecvQueue;
    work_queue* SendQueue = &WorkQueues->SendQueue;
    HANDLE RecvSemaphore = WorkQueues->RecvSemaphore;
    
    long ThreadIdx = InterlockedExchangeAdd(&GlobalRecvThreadId, 1);
    thread_trace* Trace = &GlobalTracer.Recv[ThreadIdx];
    Trace->ThreadId = GetCurrentThreadId();
    
    for (;;)
    {
        StartTiming(&Trace->ClockWaiting);
        WaitForSingleObject(RecvSemaphore, INFINITE);
        StopTiming(&Trace->ClockWaiting);
        
        InterlockedIncrement(&GlobalRecvCount);
        
        StartTiming(&Trace->ClockActive);
        socket_io* Client = PopQueue(RecvQueue);
        if (Client)
        {
            ProcessRequest(Client);
            
            win_io* WinIO = (win_io*)&Client->OSData[0];
            if (Client->Stage == IoStage_Reading)
            {
                WinIO->Buffer1 = WSABuf(Client->InBuffer + Client->InBytes,
                                        Client->InBufferSize - Client->InBytes);
                RecvPackage(Client);
            }
            else
            {
                response* Response = &Client->Http.Response;
                WinIO->Buffer1 = WSABuf(Response->Header, Response->HeaderSize);
                WinIO->Buffer2 = WSABuf(Response->Cookies, Response->CookiesSize);
                
                usz OutBufferSize = Response->ContentSize;
                if (Response->FileHandle)
                {
                    OutBufferSize = Min(Response->ContentSize, GlobalServerInfo->OutBodyMaxSize);
                    Response->Content = GetMemoryFromSystem(OutBufferSize);
                    
                    file_handle File = {
                        .Handle = Response->FileHandle,
                        .Size = OutBufferSize
                    };
                    ReadFileIntoBuffer(File, Response->Content);
                }
                WinIO->Buffer3 = WSABuf(Response->Content, (u32)OutBufferSize);
                
                SendPackage(Client, 0);
            }
        }
        
        else
        {
            // AFAZER(roberto): Como tratar?
            PrintToConsole("ERRO - RecvQueue->Pop() falhou. Count: %d, ReadIdx: %d, WriteIdx: %d\n",
                           RecvQueue->Count, RecvQueue->ReadIdx, RecvQueue->WriteIdx);
        }
        
        StopTiming(&Trace->ClockActive);
    }
    
    return 0;
}

DWORD WINAPI
SendThread(LPVOID Param)
{
    server_work_queues* WorkQueues = (server_work_queues*)Param;
    work_queue* RecvQueue = &WorkQueues->RecvQueue;
    work_queue* SendQueue = &WorkQueues->SendQueue;
    HANDLE SendSemaphore = WorkQueues->SendSemaphore;
    
    long ThreadIdx = InterlockedExchangeAdd(&GlobalSendThreadId, 1);
    thread_trace* Trace = &GlobalTracer.Send[ThreadIdx];
    Trace->ThreadId = GetCurrentThreadId();
    
    for (;;)
    {
        StartTiming(&Trace->ClockWaiting);
        WaitForSingleObject(SendSemaphore, INFINITE);
        StopTiming(&Trace->ClockWaiting);
        
        InterlockedIncrement(&GlobalSendCount);
        
        StartTiming(&Trace->ClockActive);
        socket_io* Client = PopQueue(SendQueue);
        if (Client)
        {
            win_io* WinIO = (win_io*)&Client->OSData[0];
            usz TotalOutSize = (Client->Http.Response.HeaderSize 
                                + Client->Http.Response.CookiesSize 
                                + Client->Http.Response.ContentSize);
            
            if (Client->OutBytes == TotalOutSize)
            {
                // Terminou de enviar resposta.
                
                switch (Client->Http.Response.StatusCode)
                {
                    case StatusCode_InternalServerError:
                    case StatusCode_RequestTimeout:
                    {
                        DisconnectSocket(Client);
                    } break;
                    
                    default:
                    {
                        string Connection = GetHeaderByKey(&Client->Http.Request, "Connection");
                        if (EqualsCI(Connection, StrLit("keep-alive")))
                        {
                            ResetSocket(Client);
                            
                            WinIO->Buffer1.buf = Client->InBuffer;
                            WinIO->Buffer1.len = (u32)Client->InBufferSize;
                            Client->Stage = IoStage_Reading;
                            
                            RecvPackage(Client);
                        }
                        else
                        {
                            DisconnectSocket(Client);
                        }
                    } break;
                }
            }
            
            else
            {
                // Ainda tem resposta para enviar.
                
                u32 BufferIdx = 0;
                usz SentOffset = Client->OutBytes;
                
                if (SentOffset < Client->Http.Response.HeaderSize)
                {
                    // Esta enviando o header.
                    
                    WinIO->Buffer1.buf += (u32)SentOffset;
                    WinIO->Buffer1.len -= (u32)SentOffset;
                }
                else
                {
                    BufferIdx++;
                    SentOffset -= Client->Http.Response.HeaderSize;
                    if (SentOffset < Client->Http.Response.CookiesSize)
                    {
                        // Esta enviando os cookies.
                        
                        WinIO->Buffer2.buf += (u32)SentOffset;
                        WinIO->Buffer2.len -= (u32)SentOffset;
                    }
                    else
                    {
                        // Esta enviando o content.
                        
                        BufferIdx++;
                        SentOffset -= Client->Http.Response.CookiesSize;
                        
                        usz OutBufferSize = Min(Client->Http.Response.ContentSize,
                                                GlobalServerInfo->OutBodyMaxSize);
                        if (SentOffset % OutBufferSize == 0)
                        {
                            // Tem que ler mais do arquivo no buffer.
                            
                            usz AmountLeftToRead = Client->Http.Response.ContentSize - SentOffset;
                            usz AmountToReadNext = Min(AmountLeftToRead, 
                                                       GlobalServerInfo->OutBodyMaxSize);
                            
                            file_handle File = {
                                .Handle = Client->Http.Response.FileHandle,
                                .Size = AmountToReadNext
                            };
                            ReadFileIntoBuffer(File, Client->Http.Response.Content);
                            
                            WinIO->Buffer3.buf = Client->Http.Response.Content;
                            WinIO->Buffer3.len = (u32)AmountToReadNext;
                        }
                        else
                        {
                            WinIO->Buffer3.buf += (u32)SentOffset;
                            WinIO->Buffer3.len -= (u32)SentOffset;
                        }
                    }
                }
                
                SendPackage(Client, BufferIdx);
            }
        }
        else
        {
            // AFAZER(roberto): Como tratar?
            PrintToConsole("ERRO - SendQueue->Pop() falhou. Count: %d, ReadIdx: %d, WriteIdx: %d\n",
                           SendQueue->Count, SendQueue->ReadIdx, SendQueue->WriteIdx);
        }
        
        StopTiming(&Trace->ClockActive);
    }
    
    return 0;
}

DWORD WINAPI
IOThread(LPVOID Params)
{
    HANDLE CompletionPort = ((io_params*)Params)->CompletionPort;
    server_work_queues* WorkQueues = ((io_params*)Params)->WorkQueues;    
    
    long ThreadIdx = InterlockedExchangeAdd(&GlobalIOThreadId, 1);
    thread_trace* Trace = &GlobalTracer.IO[ThreadIdx];
    Trace->ThreadId = GetCurrentThreadId();
    
    for (;;)
    {
        DWORD BytesTransferred = 0;
        ULONG_PTR CompletionKey;
        OVERLAPPED* Overlapped;
        
        StartTiming(&Trace->ClockWaiting);
        BOOL Completed = GetQueuedCompletionStatus(CompletionPort, &BytesTransferred, &CompletionKey,
                                                   &Overlapped, INFINITE);
        StopTiming(&Trace->ClockWaiting);
        
        StartTiming(&Trace->ClockActive);
        if (Completed)
        {
            socket_io* Client = (socket_io*)Overlapped;
            
            char* StageName = "@";
            if (Client->Stage == IoStage_Accepting) StageName = "Accepting";
            else if (Client->Stage == IoStage_Reading) StageName = "Reading";
            else if (Client->Stage == IoStage_Sending) StageName = "Sending";
            else if (Client->Stage == IoStage_Closing) StageName = "Closing";
            
            Log("%d: %s - %d bytes\n", Client->Socket, StageName, BytesTransferred);
            
            switch (Client->Stage)
            {
                case IoStage_Accepting:
                case IoStage_Reading:
                {
                    if (BytesTransferred > 0)
                    {
                        Client->InBytes += BytesTransferred;
                        
                        PushQueue(&WorkQueues->RecvQueue, Client);
                        ReleaseSemaphore(WorkQueues->RecvSemaphore, 1, NULL);
                    }
                    else
                    {
                        DisconnectSocket(Client);
                    }
                } break;
                
                case IoStage_Sending:
                {
                    if (BytesTransferred > 0)
                    {
                        Client->OutBytes += BytesTransferred;
                        
                        PushQueue(&WorkQueues->SendQueue, Client);
                        ReleaseSemaphore(WorkQueues->SendSemaphore, 1, NULL);
                    }
                    else
                    {
                        DisconnectSocket(Client);
                    }
                } break;
                
                case IoStage_Closing:
                {
                    ResetSocket(Client);
                    FreeSocket(&GlobalSockets, Client);
                } break;
                
                default:
                {
                    _UnreachableCode_;
                    //TerminateSocket(Client);
                }
            }
        }
        
        else
        {
            PrintToConsole("QueuedCompletionPort failed with: %d\n", GetLastError());
            
            if (Overlapped)
            {
                socket_io* Client = (socket_io*)Overlapped;
                TerminateSocket(Client);
            }
            else
            {
                // AFAZER(roberto): Como livrar o soquete sem a informacao dele?!
                _UnreachableCode_;
            }
        }
        
        StopTiming(&Trace->ClockActive);
    }
    
    return 0;
}

int
Main(void)
{
    GlobalServerInfo = ParseServerConfigFile();
    if (!GlobalServerInfo)
    {
        goto exit;
    }
    
    GlobalLogFile = CreateFileA("./logfile.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,NULL);
    
    //=====================
    // Configs do sistema.
    //=====================
    
    u64 MaxConn = MAX_SOCKETS;
    
    GlobalHeap = GetProcessHeap();
    GlobalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo = {0};
    if (GetConsoleScreenBufferInfo(GlobalConsole, &ConsoleInfo))
    {
        SMALL_RECT WindowRect = ConsoleInfo.srWindow;
        WindowRect.Bottom += 5;
        if (!SetConsoleWindowInfo(GlobalConsole, TRUE, &WindowRect))
        {
            int Error = GetLastError();
        }
    }
    
    ClearBuffer(&GlobalTracer, sizeof(GlobalTracer));
    ClearBuffer(&GlobalSockets, sizeof(GlobalSockets));
    
    SYSTEM_INFO SystemInfo = {0};
    GetSystemInfo(&SystemInfo);
    GlobalServerInfo->SystemThreads = SystemInfo.dwNumberOfProcessors;
    GlobalServerInfo->PageSize = SystemInfo.dwPageSize;
    GlobalServerInfo->MemBoundary = SystemInfo.dwAllocationGranularity;
    
    GlobalServerInfo->InBufferMaxSize = Round(GlobalServerInfo->InHeaderMaxSize + GlobalServerInfo->InBodyMaxSize, GlobalServerInfo->MemBoundary);
    GlobalServerInfo->InBufferInitSize = Round(GlobalServerInfo->InHeaderMaxSize, GlobalServerInfo->PageSize);
    
    
    //================================
    // Configs do soquete de leitura.
    //================================
    
    bool Result = false;
    WSADATA WsaData;
    SOCKET ListeningSocket;
    WSAEVENT ListeningEvent;
    
    if (WSASuccess(WSAStartup(MAKEWORD(2, 2), &WsaData)))
    {
        ListeningSocket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (ListeningSocket != INVALID_SOCKET)
        {
            SOCKADDR_IN ListeningAddr = {0};
            ListeningAddr.sin_family = AF_INET;
            ListeningAddr.sin_port = htons(GlobalServerInfo->ServerPort);
            ListeningAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            
            if (WSASuccess(bind(ListeningSocket, (SOCKADDR*)&ListeningAddr, sizeof(SOCKADDR_IN)))
                && WSASuccess(listen(ListeningSocket, SOMAXCONN)))
            {
                ListeningEvent = WSACreateEvent();
                if (ListeningEvent != WSA_INVALID_EVENT)
                {
                    if (WSASuccess(WSAEventSelect(ListeningSocket, ListeningEvent, FD_ACCEPT)))
                    {
                        Result = true;
                    }
                }
            }
        }
    }
    if (!Result)
    {
        int Error = GetLastError();
        goto exit;
    }
    
    DWORD BytesReturned;
    
    GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
    WSAIoctl(ListeningSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
             &GuidDisconnectEx, sizeof(GuidDisconnectEx), &DisconnectEx, sizeof(DisconnectEx),
             &BytesReturned, NULL, NULL);
    
    if (!(AcceptEx && DisconnectEx))
    {
        goto exit;
    }
    
    
    //===================================
    // Configs dos soquetes de cliente.
    //===================================
    
    HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!CreateIoCompletionPort((HANDLE)ListeningSocket, CompletionPort, (ULONG_PTR)NULL, 0))
    {
        PrintToConsole("CompletionPort falhou - %d.\n", GetLastError());
        goto exit;
    }
    
    InitializeCriticalSectionAndSpinCount(&GlobalSockets.Lock, 1000);
    for (usz SocketNum = 0; SocketNum < MIN_SOCKETS; SocketNum++)
    {
        usz Socket = CreateNewSocket(CompletionPort);
        socket_io* SocketIO = InitializeNewSocketIO(Socket);
        if (SocketIO)
        {
            AddSocket(&GlobalSockets, SocketIO);
        }
        else
        {
            goto exit;
        }
    }
    
    //=======================
    // Configs das threads.
    //=======================
    
    server_work_queues WorkQueues = {0};
    WorkQueues.RecvSemaphore = CreateSemaphoreA(NULL, 0, GlobalServerInfo->RecvThreadsMin, NULL);
    WorkQueues.SendSemaphore = CreateSemaphoreA(NULL, 0, GlobalServerInfo->SendThreadsMin, NULL);
    InitializeCriticalSectionAndSpinCount(&WorkQueues.RecvQueue.Lock, 500);
    InitializeCriticalSectionAndSpinCount(&WorkQueues.SendQueue.Lock, 500);
    
    DWORD ThreadId = 0;
    for (; GlobalTracer.RecvCount < GlobalServerInfo->RecvThreadsMin; GlobalTracer.RecvCount++)
    {
        HANDLE RecvThreadHandle = CreateThread(NULL, 0, RecvThread, &WorkQueues, 0, &ThreadId);
        CloseHandle(RecvThreadHandle);
    }
    for (; GlobalTracer.SendCount < GlobalServerInfo->SendThreadsMin; GlobalTracer.SendCount++)
    {
        HANDLE SendThreadHandle = CreateThread(NULL, 0, SendThread, &WorkQueues, 0, &ThreadId);
        CloseHandle(SendThreadHandle);
    }
    
    io_params IOParams = { CompletionPort, &WorkQueues };
    for (; GlobalTracer.IOCount < GlobalServerInfo->SystemThreads; GlobalTracer.IOCount++)
    {
        HANDLE IOThreadHandle = CreateThread(NULL, 0, IOThread, &IOParams, 0, &ThreadId);
        CloseHandle(IOThreadHandle);
    }
    
#ifndef _NOTIMING_
    HANDLE TimingThreadHandle = CreateThread(NULL, 0, TimingThread, NULL, 0, 0);
    CloseHandle(TimingThreadHandle);
#endif
    
    
    //====================
    // Ciclo de Accepts.
    //====================
    
    timing_info Trace = {0};
    for (;;)
    {
        DWORD EventResult = WSAWaitForMultipleEvents(1, &ListeningEvent, TRUE, WSA_INFINITE, FALSE);
        
        switch (EventResult)
        {
            case WSA_WAIT_TIMEOUT:
            {
                PrintToConsole("Connection timed out.\n");
                // AFAZER(roberto): ???
            } break;
            
            case WSA_WAIT_FAILED:
            {
                PrintToConsole("Connection failed - %d.\n", WSAGetLastError());
                // AFAZER(roberto): ???
            } break;
            
            default:
            {
                WSANETWORKEVENTS NetworkEvents = {0};
                if (WSASuccess(WSAEnumNetworkEvents(ListeningSocket, ListeningEvent, &NetworkEvents)))
                {
                    if (NetworkEvents.lNetworkEvents == FD_ACCEPT)
                    {
                        //DEBUGPrint("Connection attempted. Error code: %d\n",NetworkEvents.iErrorCode[FD_ACCEPT]);
                    }
                    else
                    {
                        PrintToConsole("Different event attempted (%d)\n", NetworkEvents.lNetworkEvents);
                    }
                }
                else
                {
                    PrintToConsole("EnumNetworkEvents() falhou - %d.\n", WSAGetLastError());
                }
                
                socket_io* Client = UseSocket(&GlobalSockets);
                if (!Client)
                {
                    usz NewSocket = CreateNewSocket(CompletionPort);
                    if (NewSocket != INVALID_SOCKET)
                    {
                        Client = InitializeNewSocketIO(NewSocket);
                        if (Client)
                        {
                            AddSocketAndUse(&GlobalSockets, Client);
                        }
                        else
                        {
                            _UnreachableCode_;
                        }
                    }
                    else
                    {
                        _UnreachableCode_;
                    }
                }
                
                AcceptPackage(ListeningSocket, Client);
            }
        }
    }
    
    WSACleanup();
    
    exit:
    ExitProcess(0);
}
