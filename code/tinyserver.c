/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

internal string
GetFileExt(string FilePath)
{
    string Ext = {0};
    for (isz ReadCur = (isz)FilePath.Size - 1; ReadCur >= 0; ReadCur--)
    {
        if (FilePath.Ptr[ReadCur] == '\\') break;
        if (FilePath.Ptr[ReadCur] == '.')
        {
            Ext = String(FilePath.Ptr + ReadCur + 1, FilePath.Size - ReadCur - 1);
            break;
        }
    }
    return Ext;
}

internal bool
LoadApp(app_info* App)
{
    bool Result = false;
    
    char LoadLibraryPathBuffer[MAX_PATH_SIZE] = {0};
    string LoadLibraryPath = StringBuffer(LoadLibraryPathBuffer, 0, MAX_PATH_SIZE);
    Eval(AppendString(&LoadLibraryPath, App->BasePath));
    Eval(AppendString(&LoadLibraryPath, StrLit("load_")));
    Eval(AppendString(&LoadLibraryPath, App->LibraryName));
    
    DuplicateFile(App->BasePath.Ptr, LoadLibraryPath.Ptr);
    
    App->LastLibraryWriteTime = GetFileLastWriteTime(App->LibraryName.Ptr);
    App->Library = LoadExternalLibrary(LoadLibraryPath.Ptr);
    if (App->Library)
    {
        App->Module = (module_main)LoadExternalSymbol(App->Library, "ModuleMain");
        Result = true;
    }
    
    eval_fail:
    return Result;
}

internal void
FreeApp(app_info* App)
{
    if (App->Library)
    {
        UnloadExternalLibrary(App->Library);
    }
}

internal bool
PrepareFileForOutput(socket_io* SocketIO)
{
    bool Result = false;
    
    char FilePathBuffer[MAX_PATH_SIZE] = {0};
    string FilePath = StringBuffer(FilePathBuffer, 0, sizeof(FilePathBuffer)-1);
    AppendToPath(&FilePath, GlobalServerInfo->FilesPath);
    AppendToPath(&FilePath, SocketIO->Http.Request.Path);
    
    file_handle File = OpenFileHandle(FilePath.Ptr);
    if (File.Handle)
    {
        string Ext = GetFileExt(SocketIO->Http.Request.Path);
        SocketIO->Http.Response.ContentType = ExtToContentType(Ext);
        SocketIO->Http.Response.FileHandle = File.Handle;
        SocketIO->Http.Response.ContentSize = File.Size;
        SocketIO->Http.Response.StatusCode = StatusCode_OK;
        
        Result = true;
    }
    else
    {
        if (File.Error == SysError_FileNotFound)
        {
            SocketIO->Http.Response.StatusCode = StatusCode_NotFound;
            PrintToConsole("[%d] CreateFile(%s) falhou - %d\n", SocketIO->Socket, FilePath.Ptr, File.Error);
        }
        else
        {
            SocketIO->Http.Response.StatusCode = StatusCode_ServiceUnavailable;
            PrintToConsole("[%d] CreateFile(%s) falhou - %d\n", SocketIO->Socket, FilePath.Ptr, File.Error);
        }
    }
    
    return Result;
}

internal socket_io*
InitializeNewSocketIO(usz NewSocket)
{
    socket_io* Result = NULL;
    
    socket_io SocketIO = {0};
    SocketIO.InBuffer = ReserveMemory(GlobalServerInfo->InBufferMaxSize);
    if (SocketIO.InBuffer
        && ExpandMemory(SocketIO.InBuffer, 0, GlobalServerInfo->InBufferMaxSize,
                        GlobalServerInfo->InBufferInitSize))
    {
        SocketIO.InBufferReservedSize = GlobalServerInfo->InBufferMaxSize;
        SocketIO.InBufferSize = GlobalServerInfo->InBufferInitSize;
        
        response* Response = &SocketIO.Http.Response;
        Response->Header = GetMemoryFromSystem(GlobalServerInfo->OutHeaderMaxSize);
        if (Response->Header)
        {
            Response->HeaderSize = GlobalServerInfo->OutHeaderMaxSize;
            SocketIO.Stage = IoStage_Reading;
            SocketIO.Socket = NewSocket;
            
            Result = (socket_io*)GetMemoryFromHeap(sizeof(socket_io));
            if (Result)
            {
                *Result = SocketIO;
            }
        }
        else
        {
            PrintToConsole("VirtualAlloc (Response header) falhou - %d\n", GetLastError());
        }
    }
    else
    {
        PrintToConsole("VirtualAlloc (InBuffer header) falhou - %d\n", GetLastError());
    }
    
    return Result;
}

internal void
ProcessRequest(socket_io* Client)
{
    char* IoDataBuffer = NULL;
    usz IoDataSize = 0;
    
    request* Request = &Client->Http.Request;
    response* Response = &Client->Http.Response;
    
    string InBuffer = String(Client->InBuffer, Client->InBytes);
    err_type Status = ParseHttpHeader(InBuffer, Request);
    
    if (Status == ErrType_OK)
    {
        string FileExt = GetFileExt(Request->Path);
        if (Request->Verb == HttpVerb_Get
            && FileExt.Ptr)
        {
            if (PrepareFileForOutput(Client))
            {
                Response->StatusCode = StatusCode_OK;
            }
            else
            {
                Response->StatusCode = StatusCode_NotFound;
            }
        }
        
        else if (Request->Verb == HttpVerb_Get
                 || Request->Verb == HttpVerb_Post
                 || Request->Verb == HttpVerb_Put)
        {
            app_info* App = &GlobalServerInfo->App;
            
            usz CurrentLibraryWriteTime = GetFileLastWriteTime(App->BasePath.Ptr);
            if (CurrentLibraryWriteTime != App->LastLibraryWriteTime)
            {
                FreeApp(App);
                LoadApp(App);
            }
            
            if (App->Library)
            {
                // AFAZER(roberto): Entrar na aplicacao por thread propria, ao invez de aqui.
                
                ts_http TsHttp = {0};
                TsHttp.Object = (void*)&Client->Http;
                
                TsHttp.Verb = Request->Verb;
                TsHttp.Path = Request->Path;
                TsHttp.Query = Request->Query;
                TsHttp.HeaderCount = Request->InfoCount;
                TsHttp._TSGetHeaderByKey = &ExternalGetHeaderByKey;
                TsHttp._TSGetHeaderByIdx = &ExternalGetHeaderByIdx;
                
                TsHttp.Entity = InBuffer.Ptr + Request->HeaderSize;
                TsHttp.EntitySize = Request->BodySize;
                TsHttp.EntityType = Request->BodyType;
                TsHttp._TSParseFormData = &ExternalParseFormData;
                TsHttp._TSGetFormFieldByName = &ExternalGetFormFieldByName;
                TsHttp._TSGetFormFieldByIdx = &ExternalGetFormFieldByIdx;
                
                TsHttp._TSAllocContentBuffer = &ExternalAllocContentBuffer;
                TsHttp._TSAllocCookiesBuffer = &ExternalAllocCookiesBuffer;
                TsHttp._TSRecordCookie = &ExternalRecordCookie;
                TsHttp._TSSetReturnCode = &ExternalSetReturnCode;
                TsHttp._TSSetContentType = &ExternalSetContentType;
                TsHttp._TSSetContentSize = &ExternalSetContentSize;
                TsHttp._TSSetCookiesSize = &ExternalSetCookiesSize;
                
                App->Module(&TsHttp);
            }
            else
            {
                Response->StatusCode = StatusCode_ServiceUnavailable;
            }
        }
        
        else
        {
            Response->StatusCode = StatusCode_NotImplemented;
        }
        
        Client->Stage = IoStage_Sending;
        CreateHttpResponseHeader(Request, Response);
        
        if (Response->Cookies
            && !(Equals(String(&Response->Cookies[Response->CookiesSize-4], 4), StrLit("\r\n\r\n"))
                 || Equals(String(&Response->Cookies[Response->CookiesSize-2], 2), StrLit("\n\n"))))
        {
            // OBS(roberto): Precisa por a linha em branco final que termina o header.
            
            if (!(Equals(String(&Response->Cookies[Response->CookiesSize-2], 2), StrLit("\r\n"))
                  || Equals(String(&Response->Cookies[Response->CookiesSize-1], 1), StrLit("\n"))))
            {
                // OBS(roberto): Precisa dar um newline ao final do cookie.
                
                Response->Cookies[Response->CookiesSize++] = '\r';
                Response->Cookies[Response->CookiesSize++] = '\n';
            }
            Response->Cookies[Response->CookiesSize++] = '\r';
            Response->Cookies[Response->CookiesSize++] = '\n';
        }
    }
    
    else if (Status == ErrType_RequestIncomplete)
    {
        if (Request->LastInfo != NULL)
        {
            // Recebendo o header.
            
            usz SpaceRemainingInBuffer = Client->InBufferSize - Client->InBytes;
            if (SpaceRemainingInBuffer == 0)
            {
                Client->Stage = IoStage_Sending;
                Response->StatusCode = StatusCode_ReqHeaderTooLarge;
                CreateHttpResponseHeader(Request, Response);
            }
        }
        
        else
        {
            // Recebendo o body.
            
            usz TotalRequestSize = Request->HeaderSize + Request->BodySize;
            usz AmountLeftToReceive = TotalRequestSize - Client->InBytes;
            
            usz SpaceRemainingInBuffer = Client->InBufferSize - Client->InBytes;
            if (SpaceRemainingInBuffer < AmountLeftToReceive)
            {
                usz AmountToExpand = AmountLeftToReceive - SpaceRemainingInBuffer;
                if (Client->InBufferSize + AmountToExpand <= Client->InBufferReservedSize)
                {
                    if (ExpandMemory(Client->InBuffer, Client->InBufferSize,
                                     Client->InBufferReservedSize, AmountToExpand))
                    {
                        Client->InBufferSize += AmountToExpand;
                    }
                    else
                    {
                        Client->Stage = IoStage_Sending;
                        Response->StatusCode = StatusCode_InternalServerError;
                        CreateHttpResponseHeader(Request, Response);
                    }
                }
                else
                {
                    Client->Stage = IoStage_Sending;
                    Response->StatusCode = StatusCode_PayloadTooLarge;
                    CreateHttpResponseHeader(Request, Response);
                }
            }
        }
    }
    
    else
    {
        Client->Stage = IoStage_Sending;
        Response->StatusCode = StatusCode_BadRequest;
        CreateHttpResponseHeader(Request, Response);
    }
}