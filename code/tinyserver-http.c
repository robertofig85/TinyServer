/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

internal string
GetHeaderByKey(request* Header, char* TargetKey)
{
    string Value = {0};
    
    char* CurrentKeyPtr = Header->FirstInfo;
    for (usz InfoIdx = 0; InfoIdx < Header->InfoCount; InfoIdx++)
    {
        u8 KeySize = *(u8*)CurrentKeyPtr;
        string CurrentKey = String(CurrentKeyPtr + 1, KeySize);
        char* CurrentValuePtr = CurrentKeyPtr + 1 + KeySize;
        u16 ValueSize = *(u16*)CurrentValuePtr;
        
        if (EqualsCI(CurrentKey, String(TargetKey, StrLen(TargetKey))))
        {
            Value.Ptr = CurrentValuePtr + 2;
            Value.Size = ValueSize;
            break;
        }
        
        CurrentKeyPtr += (1 + KeySize + 2 + ValueSize);
        if (*CurrentKeyPtr == '\r')
        {
            CurrentKeyPtr++;
        }
    }
    
    return Value;
}

internal string
GetHeaderByIdx(request* Header, usz TargetIdx)
{
    string Value = {0};
    
    if (TargetIdx < Header->InfoCount)
    {
        char* CurrentKeyPtr = Header->FirstInfo;
        for (usz InfoIdx = 0; InfoIdx < TargetIdx; InfoIdx++)
        {
            u8 KeySize = *(u8*)CurrentKeyPtr;
            u16 ValueSize = *(u16*)(CurrentKeyPtr + 1 + KeySize);
            CurrentKeyPtr += (1 + KeySize + 2 + ValueSize);
            if (*CurrentKeyPtr == '\r')
            {
                CurrentKeyPtr++;
            }
        }
        
        u8 KeySize = *(u8*)CurrentKeyPtr;
        char* CurrentValuePtr = CurrentKeyPtr + 1 + KeySize;
        Value.Ptr = CurrentValuePtr + 2;
        Value.Size = *(u16*)CurrentValuePtr;
    }
    
    return Value;
}

internal string
ExtToContentType(string Ext)
{
    string Result = {0};
    
    if (Equals(Ext, StrLit("html"))
        || Equals(Ext, StrLit("htm"))) Result = StrLit("text/html");
    else if (Equals(Ext, StrLit("css"))) Result = StrLit("text/css");
    else if (Equals(Ext, StrLit("js"))) Result = StrLit("text/javascript");
    else if (Equals(Ext, StrLit("jpg"))
             || Equals(Ext, StrLit("jpeg"))) Result = StrLit("image/jpeg");
    else if (Equals(Ext, StrLit("png"))) Result = StrLit("image/png");
    else if (Equals(Ext, StrLit("gif"))) Result = StrLit("image/gif");
    
    // OBS(roberto): Implementar outros tipos de content types aqui.
    
    return Result;
}

internal mime_type
ContentTypeToMimeType(string ContentType)
{
    mime_type Result = MimeType_Unknown;
    
    usz Idx = 0;
    string Registry = GetToken(ContentType, &Idx, '/');
    
    if (Equals(Registry, StrLit("application"))) Result = MimeType_Application;
    else if (Equals(Registry, StrLit("audio"))) Result = MimeType_Audio;
    else if (Equals(Registry, StrLit("font"))) Result = MimeType_Font;
    else if (Equals(Registry, StrLit("image"))) Result = MimeType_Image;
    else if (Equals(Registry, StrLit("message"))) Result = MimeType_Message;
    else if (Equals(Registry, StrLit("model"))) Result = MimeType_Model;
    else if (Equals(Registry, StrLit("multipart"))) Result = MimeType_Multipart;
    else if (Equals(Registry, StrLit("text"))) Result = MimeType_Text;
    else if (Equals(Registry, StrLit("video"))) Result = MimeType_Video;
    
    return Result;
}

internal form_field
OrganizeFieldInfo(field_parser* Parser)
{
    form_field Result = {0};
    
    char* StringPtr = (char*)Parser;
    Result.FieldName = String(StringPtr + Parser->FieldNameOffset, Parser->FieldNameLen);
    StringPtr += (Parser->FieldNameOffset + Parser->FieldNameLen);
    
    if (Parser->FilenameLen)
    {
        Result.Filename = String(StringPtr + Parser->FilenameOffset, Parser->FilenameLen);
        StringPtr += (Parser->FilenameOffset + Parser->FilenameLen);
    }
    if (Parser->CharsetLen)
    {
        Result.Charset = String(StringPtr + Parser->CharsetOffset, Parser->CharsetLen);
        StringPtr += (Parser->CharsetOffset + Parser->CharsetLen);
    }
    
    Result.Data = StringPtr + Parser->DataOffset;
    Result.DataLen = Parser->DataLen;
    
    return Result;
}

internal form_field
GetFormFieldByName(form* Body, string TargetName)
{
    form_field Result = {0};
    
    field_parser* TargetField = Body->FirstField;
    for (usz CountIdx = 0; CountIdx < Body->FieldCount; CountIdx++)
    {
        string FieldName = String((char*)TargetField + TargetField->FieldNameOffset,
                                  TargetField->FieldNameLen);
        
        if (Equals(FieldName, TargetName))
        {
            Result = OrganizeFieldInfo(TargetField);
            break;
        }
        
        usz NextFieldOffset = (TargetField->FieldNameOffset + TargetField->FieldNameLen +
                               TargetField->FilenameOffset + TargetField->FilenameLen +
                               TargetField->CharsetOffset + TargetField->CharsetLen +
                               TargetField->DataOffset + TargetField->DataLen +
                               TargetField->NextFieldOffset);
        TargetField = (field_parser*)((u8*)TargetField + NextFieldOffset);
    }
    
    return Result;
}

internal form_field
GetFormFieldByIdx(form* Body, usz TargetIdx)
{
    form_field Result = {0};
    
    if (TargetIdx < Body->FieldCount)
    {
        field_parser* TargetField = Body->FirstField;
        for (usz CountIdx = 0; CountIdx < TargetIdx; CountIdx++)
        {
            usz NextFieldOffset = (TargetField->FieldNameOffset + TargetField->FieldNameLen +
                                   TargetField->FilenameOffset + TargetField->FilenameLen +
                                   TargetField->CharsetOffset + TargetField->CharsetLen +
                                   TargetField->DataOffset + TargetField->DataLen +
                                   TargetField->NextFieldOffset);
            TargetField = (field_parser*)((u8*)TargetField + NextFieldOffset);
        }
        Result = OrganizeFieldInfo(TargetField);
    }
    
    return Result;
}

internal bool
IsMaliciousRequest(string Path, string Query)
{
    bool Result = true;
    
    // OBS(roberto): Protege contra path-traversal (fora da base).
    isz DirIdx = 0;
    usz PathCur = Path.Ptr[0] == '/' ? 1 : 0;
    while (DirIdx >= 0)
    {
        string Dir = GetToken(Path, &PathCur, '/');
        if (Dir.Ptr)
        {
            if (Equals(Dir, StrLit("."))) continue;
            else if (Equals(Dir, StrLit(".."))) DirIdx--;
            else DirIdx++;
        }
        else
        {
            break;
        }
    }
    
    if (DirIdx >= 0)
    {
        // OBS(roberto): Protege contra XSS.
        
        usz QueryCur = 0;
        if (Query.Size == 0
            || (!CharInString('<', Query, &QueryCur)
                && ((QueryCur = 0) == 0)
                && !CharInString('>', Query, &QueryCur)
                && ((QueryCur = 0) == 0)
                && !CharInString('\"', Query, &QueryCur)))
        {
            Result = false;
        }
    }
    
    return Result;
}

internal bool
StripCR(string* String)
{
    Assert(String->Ptr);
    
    bool HasCR = 0;
    if (String->Size > 0)
    {
        if (String->Ptr[String->Size - 1] == '\r')
        {
            String->Size--;
            HasCR = 1;
        }
    }
    
    return HasCR;
}

internal form
ParseFormData(string Entity, request* Request)
{
    form Form = {0}, EmptyForm = {0};
    
    // OBS(roberto): A boundary sempre eh avaliada com dois -- antes do valor que eh trazido no header;
    // inclui-se abaixo esse prefixo para facilitar a busca.
    
    string EntityType = GetHeaderByKey(Request, "Content-Type");
    usz TypeSplitIdx = 0;
    GetToken(EntityType, &TypeSplitIdx, '=');
    EntityType.Ptr[--TypeSplitIdx] = '-';
    EntityType.Ptr[--TypeSplitIdx] = '-';
    string Boundary = String(EntityType.Ptr + TypeSplitIdx, EntityType.Size - TypeSplitIdx);
    
    // OBS(roberto): A boundary final tem -- como sufixo (ex: se a boundary eh "--s20bHc",
    // a final eh "--s20bHc--").
    
    usz FinalBoundarySize = Boundary.Size + 2;
    
    usz ReadCur = 0;
    char* Offset = NULL;
    field_parser Parser = {0};
    field_parser* CurrentField = NULL;
    
    // OBS(roberto): Pula a boundary inicial, porque ela eh inutil.
    string Line = GetToken(Entity, &ReadCur, '\n');
    form_parse ParseStage = FormParse_FirstLine;
    
    bool FormIsValid = true;
    
    while (FormIsValid
           && ReadCur < Entity.Size)
    {
        switch (ParseStage)
        {
            case FormParse_FirstLine:
            {
                Line = GetToken(Entity, &ReadCur, '\n');
                if (Line.Ptr)
                {
                    usz LineCur = 0;
                    if (Contains(Line, String("Content-Disposition: form-data", 30), &LineCur))
                    {
                        CurrentField = (field_parser*)Line.Ptr;
                        Offset = Line.Ptr;
                        
                        // OBS(roberto): name="Field".
                        EvalStr(GetToken(Line, &LineCur, '\"'));
                        Parser.FieldNameOffset = (u8)((Line.Ptr + LineCur) - Offset);
                        Offset += Parser.FieldNameOffset;
                        EvalStr(GetToken(Line, &LineCur, '\"'));
                        Parser.FieldNameLen = (u16)((Line.Ptr + LineCur - 1) - Offset);
                        Offset += Parser.FieldNameLen;
                        
                        // OBS(roberto): Se campo for arquivo, formato eh name="Field";filename="Name".
                        if (Line.Ptr[LineCur] == ';')
                        {
                            Parser.IsFile = 1;
                            LineCur++;
                            
                            // OBS(roberto): filename="Name".
                            EvalStr(GetToken(Line, &LineCur, '\"'));
                            Parser.FilenameOffset = (u8)((Line.Ptr + LineCur) - Offset);
                            Offset += Parser.FilenameOffset;
                            EvalStr(GetToken(Line, &LineCur, '\"'));
                            Parser.FilenameLen = (u16)((Line.Ptr + LineCur - 1) - Offset);
                            Offset += Parser.FilenameLen;
                        }
                        
                        ParseStage = FormParse_SecondLine;
                    }
                    else
                    {
                        FormIsValid = false;
                    }
                }
                else
                {
                    FormIsValid = false;
                }
            } break;
            
            case FormParse_SecondLine:
            {
                Line = GetToken(Entity, &ReadCur, '\n');
                if (Line.Ptr)
                {
                    usz LineCur = 0;
                    
                    // OBS(roberto): Se nao for linha em branco, SecondLine so pode ser Content-Type.
                    if (Contains(Line, String("Content-Type", 12), &LineCur))
                    {
                        //OBS(roberto): Ignora o content-type (estimado depois a partir da extensao do arquivo) e pula pro token charset=Encoding.
                        string IgnoredToken = GetToken(Line, &LineCur, '=');
                        if (IgnoredToken.Ptr)
                        {
                            Parser.CharsetOffset = (u8)((Line.Ptr + LineCur) - Offset);
                            Offset += Parser.CharsetOffset;
                            LineCur = Entity.Ptr[ReadCur-2] == '\r' ? ReadCur-3 : ReadCur-2;
                            Parser.CharsetLen = (u16)((Line.Ptr + LineCur) - Offset);
                            Offset += Parser.CharsetLen;
                        }
                        
                        ParseStage = FormParse_ThirdLine;
                    }
                    
                    // OBS(roberto): Linha em branco indica inicio do dado do campo.
                    else if (Line.Ptr[LineCur] == '\r'
                             || Line.Ptr[LineCur] == '\n')
                    {
                        Parser.DataOffset = (u8)((Entity.Ptr + ReadCur) - Offset);
                        
                        *CurrentField = Parser;
                        Form.LastField = CurrentField;
                        if (!Form.FirstField)
                        {
                            Form.FirstField = CurrentField;
                        }
                        ParseStage = FormParse_Data;
                    }
                    
                    else
                    {
                        FormIsValid = false;
                    }
                }
            } break;
            
            case FormParse_ThirdLine:
            {
                // OBS(roberto): ThirdLine so pode ser linha em branco.
                usz CurrentReadCur = ReadCur;
                JumpCLRF(Entity, ReadCur);
                if (ReadCur > CurrentReadCur)
                {
                    Parser.DataOffset = (u8)((Entity.Ptr + ReadCur) - Offset);
                    
                    *CurrentField = Parser;
                    Form.LastField = CurrentField;
                    if (!Form.FirstField)
                    {
                        Form.FirstField = CurrentField;
                    }
                    ParseStage = FormParse_Data;
                }
                else
                {
                    FormIsValid = false;
                }
            } break;
            
            case FormParse_Data:
            {
                usz DataEnd = 0;
                string RawData = String(Entity.Ptr + ReadCur, Entity.Size - ReadCur);
                if (Contains(RawData, Boundary, &DataEnd))
                {
                    ReadCur += DataEnd;
                    
                    // OBS(roberto): Tira 2 bytes para compensar pelo \r\n ao final do dado.
                    JumpBackCLRF(RawData, DataEnd);
                    Parser.DataLen = (u32)DataEnd;
                    
                    Form.FieldCount++;
                    ReadCur += Boundary.Size;
                    if (Entity.Ptr[ReadCur] == '-')
                    {
                        ParseStage = FormParse_Complete;
                    }
                    else if (Entity.Ptr[ReadCur] == '\r'
                             || Entity.Ptr[ReadCur] == '\n')
                    {
                        JumpCLRF(Entity, ReadCur);
                        Parser.NextFieldOffset = (u16)((intptr_t)(Entity.Ptr + ReadCur) - (intptr_t)(RawData.Ptr + DataEnd));
                        ParseStage = FormParse_FirstLine;
                    }
                    else
                    {
                        FormIsValid = false;
                    }
                    
                    CurrentField->DataLen = Parser.DataLen;
                    CurrentField->NextFieldOffset = Parser.NextFieldOffset;
                    
                    ClearBuffer(&Parser, sizeof(Parser));
                    CurrentField = (field_parser*)(Entity.Ptr + ReadCur);
                }
                else
                {
                    FormIsValid = false;
                }
            } break;
            
            case FormParse_Complete:
            {
                ReadCur = Entity.Size;
            } break;
            
            default:
            {
                FormIsValid = false;
            } break;
        }
    }
    
    if (false)
    {
        evalstr_fail:
        FormIsValid = true;
    }
    
    return FormIsValid ? Form : EmptyForm;
}

internal err_type
ParseHttpHeader(string InBuffer, request* Request)
{
    err_type Result = ErrType_OK;
    
    /*====================================
    // Parse da linha inicial do header.
    ====================================*/
    
    if (!Request->FirstInfo)
    {
        usz ReadCur = 0;
        
        string Line = GetToken(InBuffer, &ReadCur, '\n');
        if (Line.Size > 0)
        {
            usz LineReadCur = 0;
            string Verb = GetToken(Line, &LineReadCur, ' ');
            
            // OBS(roberto): Se novos metodos forem implementados, adiciar a lista aqui.
            if (Equals(Verb, StrLit("GET"))) Request->Verb = HttpVerb_Get;
            else if (Equals(Verb, StrLit("HEAD"))) Request->Verb = HttpVerb_Head;
            else if (Equals(Verb, StrLit("POST"))) Request->Verb = HttpVerb_Post;
            else if (Equals(Verb, StrLit("PUT"))) Request->Verb = HttpVerb_Put;
            else if (Equals(Verb, StrLit("DELETE"))) Request->Verb = HttpVerb_Delete;
            else if (Equals(Verb, StrLit("CONNECT"))) Request->Verb = HttpVerb_Connect;
            else if (Equals(Verb, StrLit("OPTIONS"))) Request->Verb = HttpVerb_Options;
            else if (Equals(Verb, StrLit("TRACE"))) Request->Verb = HttpVerb_Trace;
            else if (Equals(Verb, StrLit("PATCH"))) Request->Verb = HttpVerb_Patch;
            
            if (Request->Verb != HttpVerb_Unknown)
            {
                string Uri = GetToken(Line, &LineReadCur, ' ');
                
                usz UriCur = 0;
                Request->Path = GetToken(Uri, &UriCur, '?');
                if (UriCur > 0)
                {
                    Request->Query = String(Uri.Ptr + UriCur, Uri.Size - UriCur);
                }
                else
                {
                    Request->Path = Uri;
                }
                
                if (!IsMaliciousRequest(Request->Path, Request->Query))
                {
                    string Version = String(Line.Ptr + LineReadCur, Line.Size - LineReadCur);
                    StripCR(&Version);
                    
                    // OBS(roberto): Se novas versoes forem implementadas, adicionar a lista aqui.
                    char* VersionList[] = { "HTTP/1.0", "HTTP/1.1", "" };
                    if (StringInList(Version, VersionList))
                    {
                        Request->Version = Version;
                        Request->HeaderSize = --ReadCur;
                        Request->FirstInfo = InBuffer.Ptr + ReadCur;
                        Request->LastInfo = Request->FirstInfo;
                    }
                    else
                    {
                        Result = ErrType_Forbid;
                    }
                }
                else
                {
                    Result = ErrType_Forbid;
                }
            }
            else
            {
                Result = ErrType_RequestInvalid;
            }
        }
        else
        {
            Result = ErrType_RequestIncomplete;
        }
    }
    
    /*===============================
    // Parse do restante do header.
    ===============================*/
    
    if (Result == ErrType_OK
        && Request->LastInfo)
    {
        Result = ErrType_RequestIncomplete;
        
        while (Request->HeaderSize < InBuffer.Size)
        {
            string CurrentInfo = String(Request->LastInfo, InBuffer.Size - Request->HeaderSize);
            usz ReadCur = 0;
            if (CurrentInfo.Ptr[ReadCur+1] == '\r'
                || CurrentInfo.Ptr[ReadCur+1] == '\n')
            {
                Result = ErrType_OK;
                
                // OBS(roberto): Poe o cursor no fim do header.
                JumpCLRF(CurrentInfo, ReadCur);
                JumpCLRF(CurrentInfo, ReadCur);
                Request->HeaderSize += ReadCur;
                
                // OBS(roberto): Desabilita para indicar que terminou parse.
                Request->LastInfo = NULL;
                
                if (Request->Verb == HttpVerb_Post
                    || Request->Verb == HttpVerb_Put)
                {
                    string EntitySize = GetHeaderByKey(Request, "Content-Length");
                    mime_type MimeType = ContentTypeToMimeType(GetHeaderByKey(Request, "Content-Type"));
                    if (EntitySize.Size > 0
                        && MimeType != MimeType_Unknown)
                    {
                        Request->BodyType = MimeType;
                        Request->BodySize = StringToNumber(EntitySize);
                    }
                    else
                    {
                        Result = ErrType_RequestInvalid;
                    }
                }
                
                break;
            }
            
            u8* KeySizeInfo = (u8*)(CurrentInfo.Ptr + ReadCur);
            ReadCur += sizeof(u8);
            string Key = GetToken(CurrentInfo, &ReadCur, ':');
            EvalStr(Key);
            
            u16* ValueSizeInfo = (u16*)(CurrentInfo.Ptr + --ReadCur);
            ReadCur += sizeof(u16);
            string Value = GetToken(CurrentInfo, &ReadCur, '\n');
            EvalStr(Value);
            
            StripCR(&Value);
            *KeySizeInfo = (u8)Key.Size;
            *ValueSizeInfo = (u16)Value.Size;
            
            Request->HeaderSize += --ReadCur;
            Request->LastInfo = CurrentInfo.Ptr + ReadCur;
            Request->InfoCount++;
        }
    }
    
    /*=============================
    // Parse do body (se houver).
    =============================*/
    
    if (Result == ErrType_OK
        && Request->BodySize > 0)
    {
        usz TotalRequestSize = Request->HeaderSize + Request->BodySize;
        if (TotalRequestSize > InBuffer.Size)
        {
            Result = ErrType_RequestIncomplete;
        }
    }
    
    if (false)
    {
        evalstr_fail:
        Result = ErrType_RequestInvalid;
    }
    
    return Result;
}

internal bool
FormatTimeIMF(string* Time, time_format TimeFormat)
{
    bool Result = false;
    
    if (Time->MaxSize >= (Time->Size + IMF_DATE_LENGTH))
    {
        char* WeekDay = NULL;
        switch (TimeFormat.WeekDay)
        {
            case WeekDay_Sunday: WeekDay = "Sun"; break;
            case WeekDay_Monday: WeekDay = "Mon"; break;
            case WeekDay_Tuesday: WeekDay = "Tue"; break;
            case WeekDay_Wednesday: WeekDay = "Wed"; break;
            case WeekDay_Thursday: WeekDay = "Thu"; break;
            case WeekDay_Friday: WeekDay = "Fri"; break;
            case WeekDay_Saturday: WeekDay = "Sat"; break;
        }
        AppendString(Time, String(WeekDay, 3));
        
        AppendData(Time, ", ");
        AppendData(Time, (TimeFormat.Day < 10 ? "0" : ""));
        AppendNumber(Time, TimeFormat.Day);
        AppendData(Time, " ");
        
        char* Month = NULL;
        switch (TimeFormat.Month)
        {
            case 1: Month = "Jan"; break;
            case 2: Month = "Feb"; break;
            case 3: Month = "Mar"; break;
            case 4: Month = "Apr"; break;
            case 5: Month = "May"; break;
            case 6: Month = "Jun"; break;
            case 7: Month = "Jul"; break;
            case 8: Month = "Aug"; break;
            case 9: Month = "Sep"; break;
            case 10: Month = "Oct"; break;
            case 11: Month = "Nov"; break;
            case 12: Month = "Dec"; break;
        }
        AppendString(Time, String(Month, 3));
        
        AppendData(Time, " ");
        AppendNumber(Time, TimeFormat.Year);
        AppendData(Time, " ");
        AppendData(Time, (TimeFormat.Hour < 10 ? "0" : ""));
        AppendNumber(Time, TimeFormat.Hour);
        AppendData(Time, " ");
        AppendData(Time, (TimeFormat.Minute < 10 ? "0" : ""));
        AppendNumber(Time, TimeFormat.Minute);
        AppendData(Time, " ");
        AppendData(Time, (TimeFormat.Second < 10 ? "0" : ""));
        AppendNumber(Time, TimeFormat.Second);
        AppendString(Time, StrLit(" GMT"));
        
        Result = true;
    }
    
    return Result;
}

internal void
CreateHttpResponseHeader(request* Request, response* Response)
{
    string Header = StringBuffer(Response->Header, 0, Response->HeaderSize);
    string LineBreak = StrLit("\r\n");
    
    /*======================
    // Campos obrigatorios
    ======================*/
    
    Eval(AppendString(&Header, Request->Version));
    Eval(AppendString(&Header, StrLit(" ")));
    
    string FirstLine = {0};
    switch (Response->StatusCode)
    {
        case StatusCode_Continue: FirstLine = StrLit("100 Continue"); break;
        case StatusCode_SwitchingProtocol: FirstLine = StrLit("101 Switching Protocol"); break;
        case StatusCode_OK: FirstLine = StrLit("200 OK"); break;
        case StatusCode_Created: FirstLine = StrLit("201 Created"); break;
        case StatusCode_Accepted: FirstLine = StrLit("202 Accepted"); break;
        case StatusCode_NonAuthInfo: FirstLine = StrLit("203 Non-Authoritative Information"); break;
        case StatusCode_NoContent: FirstLine = StrLit("204 No Content"); break;
        case StatusCode_ResetContent: FirstLine = StrLit("205 Reset Content"); break;
        case StatusCode_MultipleChoices: FirstLine = StrLit("300 Multiple Choices"); break;
        case StatusCode_MovedPermanently: FirstLine = StrLit("301 Moved Permanently"); break;
        case StatusCode_Found: FirstLine = StrLit("302 Found"); break;
        case StatusCode_SeeOther: FirstLine = StrLit("303 See Other"); break;
        case StatusCode_NoModified: FirstLine = StrLit("304 No Modified"); break;
        case StatusCode_UseProxy: FirstLine = StrLit("305 Use Proxy"); break;
        case StatusCode_TempRedirect: FirstLine = StrLit("307 Temporary Redirect"); break;
        case StatusCode_PermRedirect: FirstLine = StrLit("308 Permanent Redirect"); break;
        case StatusCode_BadRequest: FirstLine = StrLit("400 Bad Request"); break;
        case StatusCode_Unauthorized: FirstLine = StrLit("401 Unauthorized"); break;
        case StatusCode_PaymentRequired: FirstLine = StrLit("402 Payment Required"); break;
        case StatusCode_Forbidden: FirstLine = StrLit("403 Forbidden"); break;
        case StatusCode_NotFound: FirstLine = StrLit("404 Not Found"); break;
        case StatusCode_MethodNotAllowed: FirstLine = StrLit("405 Method Not Allowed"); break;
        case StatusCode_NotAcceptable: FirstLine = StrLit("406 Not Acceptable"); break;
        case StatusCode_ProxyAuthRequired: FirstLine = StrLit("407 Proxy Authorization Required"); break;
        case StatusCode_RequestTimeout: FirstLine = StrLit("408 Request Timeout"); break;
        case StatusCode_Conflict: FirstLine = StrLit("409 Conflict"); break;
        case StatusCode_Gone: FirstLine = StrLit("410 Gone"); break;
        case StatusCode_LengthRequired: FirstLine = StrLit("411 Length Required"); break;
        case StatusCode_PreconditionFailed: FirstLine = StrLit("412 Precondition Failed"); break;
        case StatusCode_PayloadTooLarge: FirstLine = StrLit("413 Payload Too Large"); break;
        case StatusCode_URITooLong: FirstLine = StrLit("414 URI Too Long"); break;
        case StatusCode_UnsupportedMediaType: FirstLine = StrLit("415 Unsupported Media Type"); break;
        case StatusCode_RangeNotSatisfiable: FirstLine = StrLit("416 Range Not Satisfiable"); break;
        case StatusCode_ExpectationFailed: FirstLine = StrLit("417 Expectation Failed"); break;
        case StatusCode_ImATeapot: FirstLine = StrLit("418 I'm a Teapot"); break;
        case StatusCode_MisdirectedRequest: FirstLine = StrLit("421 Misdirected Request"); break;
        case StatusCode_TooEarly: FirstLine = StrLit("425 Too Early"); break;
        case StatusCode_UpgradeRequired: FirstLine = StrLit("426 Update Required"); break;
        case StatusCode_PreconditionRequired: FirstLine = StrLit("428 Precondition Required"); break;
        case StatusCode_TooManyRequests: FirstLine = StrLit("429 Too Many Requests"); break;
        case StatusCode_ReqHeaderTooLarge: FirstLine = StrLit("431 Request Header Fields Too Large"); break;
        case StatusCode_LoginTimeout: FirstLine = StrLit("440 Login Timeout"); break;
        case StatusCode_UnavailableForLegalReasons: FirstLine = StrLit("451 Unavailable for Legal Reasons"); break;
        default:
        case StatusCode_InternalServerError: FirstLine = StrLit("500 Internal Server Error"); break;
        case StatusCode_NotImplemented: FirstLine = StrLit("501 Not Implemented"); break;
        case StatusCode_BadGateway: FirstLine = StrLit("502 Bad Gateway"); break;
        case StatusCode_ServiceUnavailable: FirstLine = StrLit("503 Service Unavailable"); break;
        case StatusCode_GatewayTimeout: FirstLine = StrLit("504 Gateway Timeout"); break;
        case StatusCode_HttpVersionNotSupported: FirstLine = StrLit("505 HTTP Version Not Supported"); break;
        case StatusCode_InsufficientStorage: FirstLine = StrLit("507 Insufficient Storage"); break;
        case StatusCode_NotExtended: FirstLine = StrLit("510 Not Extended"); break;
        case StatusCode_NetworkAuthRequired: FirstLine = StrLit("511 Network Authentication Required"); break;
    }
    Eval(AppendString(&Header, FirstLine));
    Eval(AppendString(&Header, LineBreak));
    
    Eval(AppendString(&Header, StrLit("Date: ")));
    Eval(FormatTimeIMF(&Header, GetCurrentSystemTime()));
    Eval(AppendString(&Header, LineBreak));
    
    Eval(AppendString(&Header, StrLit("Server: Tinyserver 0.5\r\n")));
    
    string Connection = GetHeaderByKey(Request, "Connection");
    Eval(AppendString(&Header, StrLit("Connection: ")));
    Eval(AppendString(&Header, Connection.Ptr ? Connection : StrLit("close")));
    Eval(AppendString(&Header, LineBreak));
    
    Eval(AppendString(&Header, StrLit("Content-Length: ")));
    Eval(AppendNumber(&Header, Response->ContentSize));
    Eval(AppendString(&Header, LineBreak));
    
    /*===================
    // Campos opcionais
    ===================*/
    
    if (Response->ContentSize)
    {
        Eval(AppendString(&Header, StrLit("Content-Type: ")));
        Eval(AppendString(&Header, Response->ContentType));
        Eval(AppendString(&Header, LineBreak));
    }
    
    /*==============
    // Linha final
    ==============*/
    
    if (!Response->CookiesSize)
    {
        // OBS(roberto): Se for enviar cookies, newline final do header estara no buffer dos cookies;
        // caso contrario, estara aqui.
        
        Eval(AppendString(&Header, LineBreak));
    }
    
    Response->HeaderSize = Header.Size;
    
    eval_fail:
    return;
}