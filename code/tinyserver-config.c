/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */


internal string
LexerGetNextToken(string Buffer, usz* restrict ReadCur)
{
    string Token = {0};
    
    usz Cur = *ReadCur;
    while (Cur < Buffer.Size)
    {
        if (Buffer.Ptr[Cur] == ' ')
        {
            do
            {
                Cur++;
            } while (Cur < Buffer.Size
                     && Buffer.Ptr[Cur] == ' ');
        }
        
        else if (Buffer.Ptr[Cur] == '\t')
        {
            do
            {
                Cur++;
            } while (Cur < Buffer.Size
                     && Buffer.Ptr[Cur] == '\t');
        }
        
        else if (Buffer.Ptr[Cur] == '#')
        {
            GetToken(Buffer, &Cur, '\n');
        }
        
        else if (Buffer.Ptr[Cur] == '\r')
        {
            Cur += 2;
        }
        
        else if (Buffer.Ptr[Cur] == '\n'
                 || Buffer.Ptr[Cur] == ':')
        {
            Cur++;
        }
        
        else if (IsDigit(Buffer.Ptr[Cur]))
        {
            Token.Ptr = Buffer.Ptr+Cur;
            do
            {
                Cur++;
            } while (Cur < Buffer.Size
                     && IsDigit(Buffer.Ptr[Cur]));
            Token.Size = (usz)Buffer.Ptr+Cur - (usz)Token.Ptr;
            break;
        }
        
        else if (Buffer.Ptr[Cur] == '"')
        {
            Cur++;
            Token = GetToken(Buffer, &Cur, '"');
            break;
        }
        
        else if (IsLetter(Buffer.Ptr[Cur])
                 || Buffer.Ptr[Cur] == '.'
                 || Buffer.Ptr[Cur] == '/'
                 || Buffer.Ptr[Cur] == '\\')
        {
            Token.Ptr = Buffer.Ptr+Cur;
            do
            {
                Cur++;
            } while (Cur < Buffer.Size
                     && Buffer.Ptr[Cur] != ' '
                     && Buffer.Ptr[Cur] != '\t'
                     && Buffer.Ptr[Cur] != '#'
                     && Buffer.Ptr[Cur] != '\r'
                     && Buffer.Ptr[Cur] != '\n'
                     && Buffer.Ptr[Cur] != ':');
            Token.Size = (usz)Buffer.Ptr+Cur - (usz)Token.Ptr;
            break;
        }
        
        else
        {
            break;
        }
    }
    *ReadCur = Cur;
    
    return Token;
}

internal usz
ParseSize(string Buffer, usz* restrict ReadCur)
{
    usz Result = 0;
    
    string SizeToken = LexerGetNextToken(Buffer, ReadCur);
    if (IsNumber(SizeToken))
    {
        string BytesToken = LexerGetNextToken(Buffer, ReadCur);
        if (BytesToken.Ptr)
        {
            if (EqualsCI(BytesToken, String("b", 1))
                || EqualsCI(BytesToken, String("bytes", 5)))
            {
                Result = StringToNumber(SizeToken);
            }
            
            else if (EqualsCI(BytesToken, String("kb", 2))
                     || EqualsCI(BytesToken, String("kilobytes", 9)))
            {
                Result = Kilobyte(StringToNumber(SizeToken));
            }
            
            else if (EqualsCI(BytesToken, String("mb", 2))
                     || EqualsCI(BytesToken, String("megabytes", 9)))
            {
                Result = Megabyte(StringToNumber(SizeToken));
            }
            
            else if (EqualsCI(BytesToken, String("gb", 2))
                     || EqualsCI(BytesToken, String("gigabytes", 9)))
            {
                Result = Gigabyte(StringToNumber(SizeToken));
            }
        }
    }
    
    return Result;
}

internal server_info*
ParseServerConfigFile(void)
{
    server_info* Info = NULL;
    
    // Carrega valores default, para caso nao consiga ler arquivo .conf.
    server_info TempInfo = {0};
    TempInfo.ServerPort = 80;
    TempInfo.SocketsMin = 10;
    TempInfo.SocketsMax = 50;
    TempInfo.RecvThreadsMin = 10;
    TempInfo.RecvThreadsMax = 20;
    TempInfo.SendThreadsMin = 10;
    TempInfo.SendThreadsMax = 20;
    TempInfo.InHeaderMaxSize = Kilobyte(4);
    TempInfo.InBodyMaxSize = Kilobyte(8);
    TempInfo.OutHeaderMaxSize = Kilobyte(4);
    TempInfo.OutCookiesMaxSize = Kilobyte(8);
    TempInfo.OutBodyMaxSize = Megabyte(2);
    
    string Buffer = {0};
    file_handle File = OpenFileHandle("./tinyserver.conf");
    if (File.Handle
        && File.Size < Kilobyte(100)
        && (Buffer.Ptr = GetMemoryFromSystem(File.Size)) != NULL)
    {
        Buffer.Size = File.Size;
        
        if (ReadFileIntoBuffer(File, Buffer.Ptr))
        {
            usz ReadCur = 0;
            string Token = LexerGetNextToken(Buffer, &ReadCur);
            
            while (Token.Ptr)
            {
                if (Equals(Token, StrLit("server-port")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.ServerPort = (u16)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("files-folder-path")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && !TempInfo.FilesPath.Ptr
                        && IsExistingPath(Token))
                    {
                        TempInfo.FilesPath.Ptr = Token.Ptr;
                        TempInfo.FilesPath.Size = Token.Size;
                    }
                }
                
                else if (Equals(Token, StrLit("sockets-min")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.SocketsMin = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("sockets-max")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.SocketsMax = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("recv-threads-min")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.RecvThreadsMin = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("recv-threads-max")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.RecvThreadsMax = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("send-threads-min")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.SendThreadsMin = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("send-threads-max")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && IsNumber(Token))
                    {
                        TempInfo.SendThreadsMax = (u32)StringToNumber(Token);
                    }
                }
                
                else if (Equals(Token, StrLit("request-header-max-size")))
                {
                    usz Size = ParseSize(Buffer, &ReadCur);
                    if (Size > 0)
                    {
                        TempInfo.InHeaderMaxSize = Size;
                    }
                }
                
                else if (Equals(Token, StrLit("request-body-max-size")))
                {
                    usz Size = ParseSize(Buffer, &ReadCur);
                    if (Size > 0)
                    {
                        TempInfo.InBodyMaxSize = Size;
                    }
                }
                
                else if (Equals(Token, StrLit("response-header-max-size")))
                {
                    usz Size = ParseSize(Buffer, &ReadCur);
                    if (Size > 0)
                    {
                        TempInfo.OutHeaderMaxSize = Size;
                    }
                }
                
                else if (Equals(Token, StrLit("response-cookies-max-size")))
                {
                    usz Size = ParseSize(Buffer, &ReadCur);
                    if (Size > 0)
                    {
                        TempInfo.OutCookiesMaxSize = Size;
                    }
                }
                
                else if (Equals(Token, StrLit("response-body-max-size")))
                {
                    usz Size = ParseSize(Buffer, &ReadCur);
                    if (Size > 0)
                    {
                        TempInfo.OutBodyMaxSize = Size;
                    }
                }
                
                else if (Equals(Token, StrLit("app-base-path")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && !TempInfo.App.BasePath.Ptr
                        && IsExistingPath(Token))
                    {
                        TempInfo.App.BasePath.Ptr = Token.Ptr;
                        TempInfo.App.BasePath.Size = Token.Size;
                    }
                }
                
                else if (Equals(Token, StrLit("app-library-name")))
                {
                    Token = LexerGetNextToken(Buffer, &ReadCur);
                    if (Token.Ptr
                        && !TempInfo.App.LibraryName.Ptr)
                    {
                        TempInfo.App.LibraryName.Ptr = Token.Ptr;
                        TempInfo.App.LibraryName.Size = Token.Size;
                    }
                }
                
                Token = LexerGetNextToken(Buffer, &ReadCur);
            }
        }
    }
    
    // OBS(roberto): Adiciona +1 ao tamanho de cada string para adicionar um terminador \0 a copia.
    usz InfoSize = (sizeof(server_info) + TempInfo.FilesPath.Size+1
                    + TempInfo.App.BasePath.Size+1 + TempInfo.App.LibraryName.Size+1);
    
    Info = GetMemoryFromSystem(InfoSize);
    if (Info)
    {
        CopyData(Info, InfoSize, &TempInfo, sizeof(TempInfo));
        usz BufferOffset = sizeof(server_info);
        
        if (Info->FilesPath.Ptr)
        {
            char* FilesPath = (char*)Info + BufferOffset;
            CopyData(FilesPath, Info->FilesPath.Size, Info->FilesPath.Ptr, Info->FilesPath.Size);
            Info->FilesPath.Ptr = FilesPath;
            
            // OBS(roberto): Offset recebe 1 byte a mais para o terminador \0.
            BufferOffset += Info->FilesPath.Size+1;
        }
        else
        {
            Info->FilesPath.Ptr = "./files";
            Info->FilesPath.Size = sizeof(".\files");
        }
        
        app_info* App = &Info->App;
        if (App->BasePath.Ptr
            && App->LibraryName.Ptr)
        {
            char* BasePath = (char*)Info + BufferOffset;
            CopyData(BasePath, App->BasePath.Size, App->BasePath.Ptr, App->BasePath.Size);
            App->BasePath.Ptr = BasePath;
            
            // OBS(roberto): Verifica se path termina com separador de diretorio, e inclui um se
            // nao terminar. Isso eh para que uma leitura de App->BasePath.Ptr ja de o path completo
            // da biblioteca, ja que o nome da biblioteca eh apendado logo apos na memoria.
            
            if (BasePath[App->BasePath.Size-1] != '/'
                && BasePath[App->BasePath.Size-1] != '\\')
            {
                BasePath[App->BasePath.Size++] = '/';
            }
            BufferOffset += Info->App.BasePath.Size;
            
            char* LibraryName = (char*)Info + BufferOffset;
            CopyData(LibraryName, App->LibraryName.Size, App->LibraryName.Ptr, App->LibraryName.Size);
            App->LibraryName.Ptr = LibraryName;
            
            // OBS(roberto): Offset recebe 1 byte a mais para o terminador \0.
            BufferOffset += App->LibraryName.Size+1;
        }
    }
    
    if (Buffer.Ptr) FreeMemoryFromSystem(Buffer.Ptr);
    if (File.Handle) CloseFileHandle(File.Handle);
    
    return Info;
}
