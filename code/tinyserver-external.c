/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

//====================
// Request Header
//====================

extern ts_string
ExternalGetHeaderByKey(ts_http* Http, char* Key)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    string Value = GetHeaderByKey(&HttpInfo->Request, Key);
    return Value;
}

extern ts_string
ExternalGetHeaderByIdx(ts_http* Http, size_t Idx)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    string Value = GetHeaderByIdx(&HttpInfo->Request, Idx);
    return Value;
}

//===================
// Request Body
//===================

extern ts_form
ExternalParseFormData(ts_http* Http)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    string Entity = String(Http->Entity, Http->EntitySize);
    form Form = ParseFormData(Entity, &HttpInfo->Request);
    return Form;
}

extern ts_form_field
ExternalGetFormFieldByName(form* Form, char* Name)
{
    form_field Field = GetFormFieldByName(Form, String(Name, StrLen(Name)));
    return Field;
}

extern ts_form_field
ExternalGetFormFieldByIdx(form* Form, size_t Idx)
{
    form_field Field = GetFormFieldByIdx(Form, Idx);
    return Field;
}

//====================
// Response
//====================

extern ts_string
ExternalAllocContentBuffer(ts_http* Http, size_t MemSize)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    
    ts_string Result = {0};
    if (MemSize <= GlobalServerInfo->OutBodyMaxSize)
    {
        usz SizeToAlloc = Round(MemSize, GlobalServerInfo->OutBodyMaxSize);
        Result.Ptr = HttpInfo->Response.Content = GetMemoryFromSystem(SizeToAlloc);
        Result.MaxSize = SizeToAlloc;
    }
    return Result;
}

extern ts_string
ExternalAllocCookiesBuffer(ts_http* Http, size_t MemSize)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    
    // OBS(roberto): Aloca +4 bytes para garantir espaco para as newlines do fim do header.
    
    ts_string Result = {0};
    if (MemSize+4 <= GlobalServerInfo->OutCookiesMaxSize)
    {
        usz SizeToAlloc = Round(MemSize+4, GlobalServerInfo->PageSize);
        Result.Ptr = HttpInfo->Response.Cookies = GetMemoryFromSystem(SizeToAlloc);
        Result.MaxSize = SizeToAlloc;
    }
    return Result;
}

extern bool
ExternalRecordCookie(ts_http* Http, ts_cookie* NewCookie)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    
    bool Result = false;
    if (!HttpInfo->Response.Cookies)
    {
        HttpInfo->Response.Cookies = GetMemoryFromSystem(GlobalServerInfo->OutCookiesMaxSize);
    }
    
    // OBS(roberto): Omite 2 bytes que serao usados na ultima newline do fim do header.
    
    string Cookies = StringBuffer(HttpInfo->Response.Cookies, HttpInfo->Response.CookiesSize,
                                  GlobalServerInfo->OutCookiesMaxSize-2);
    
    Eval(AppendString(&Cookies, StrLit("Set-Cookie: ")));
    Eval(AppendString(&Cookies, NewCookie->Name));
    Eval(AppendString(&Cookies, StrLit("=")));
    Eval(AppendString(&Cookies, NewCookie->Value));
    
    if (NewCookie->AttrFlags & CookieAttr_ExpDate)
    {
        Eval(AppendString(&Cookies, StrLit("; Expires=")));
        Eval(FormatTimeIMF(&Cookies, NewCookie->ExpDate));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_MaxAge)
    {
        Eval(AppendString(&Cookies, StrLit("; Max-Age=")));
        Eval(AppendNumber(&Cookies, NewCookie->MaxAge));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_Domain)
    {
        Eval(AppendString(&Cookies, StrLit("; Domain=")));
        Eval(AppendString(&Cookies, NewCookie->Domain));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_Path)
    {
        Eval(AppendString(&Cookies, StrLit("; Path=")));
        Eval(AppendString(&Cookies, NewCookie->Path));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_SameSiteStrict)
    {
        Eval(AppendString(&Cookies, StrLit("; SameSite=Strict")));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_SameSiteLax)
    {
        Eval(AppendString(&Cookies, StrLit("; SameSite=Lax")));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_SameSiteNone)
    {
        Eval(AppendString(&Cookies, StrLit("; SameSite=None")));
        NewCookie->AttrFlags |= CookieAttr_Secure;
    }
    
    if (NewCookie->AttrFlags & CookieAttr_Secure)
    {
        Eval(AppendString(&Cookies, StrLit("; Secure")));
    }
    
    if (NewCookie->AttrFlags & CookieAttr_HttpOnly)
    {
        Eval(AppendString(&Cookies, StrLit("; HttpOnly")));
    }
    
    Eval(AppendData(&Cookies, "\r\n"));
    HttpInfo->Response.CookiesSize = Cookies.Size;
    Result = true;
    
    eval_fail:
    return Result;
}

extern bool
ExternalSetReturnCode(ts_http* Http, size_t Code)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    
    bool Result = false;
    switch ((status_code)Code)
    {
        case StatusCode_Continue:
		case StatusCode_SwitchingProtocol:    
		case StatusCode_OK:
		case StatusCode_Created:
		case StatusCode_Accepted:
		case StatusCode_NonAuthInfo:
		case StatusCode_NoContent:
		case StatusCode_ResetContent:    
		case StatusCode_MultipleChoices:
		case StatusCode_MovedPermanently:
		case StatusCode_Found:
		case StatusCode_SeeOther:
		case StatusCode_NoModified:
		case StatusCode_UseProxy:
		case StatusCode_TempRedirect:
		case StatusCode_PermRedirect:    
		case StatusCode_BadRequest:
		case StatusCode_Unauthorized:
		case StatusCode_PaymentRequired:
		case StatusCode_Forbidden:
		case StatusCode_NotFound:
		case StatusCode_MethodNotAllowed:
		case StatusCode_NotAcceptable:
		case StatusCode_ProxyAuthRequired:
		case StatusCode_RequestTimeout:
		case StatusCode_Conflict:
		case StatusCode_Gone:
		case StatusCode_LengthRequired:
		case StatusCode_PreconditionFailed:
		case StatusCode_PayloadTooLarge:
		case StatusCode_URITooLong:
		case StatusCode_UnsupportedMediaType:
		case StatusCode_RangeNotSatisfiable:
		case StatusCode_ExpectationFailed:
		case StatusCode_ImATeapot:
		case StatusCode_MisdirectedRequest:
		case StatusCode_TooEarly:
		case StatusCode_UpgradeRequired:
		case StatusCode_PreconditionRequired:
		case StatusCode_TooManyRequests:
		case StatusCode_ReqHeaderTooLarge:
		case StatusCode_LoginTimeout:
		case StatusCode_UnavailableForLegalReasons:    
		case StatusCode_InternalServerError:
		case StatusCode_NotImplemented:
		case StatusCode_BadGateway:
		case StatusCode_ServiceUnavailable:
		case StatusCode_GatewayTimeout:
		case StatusCode_HttpVersionNotSupported:
		case StatusCode_InsufficientStorage:
		case StatusCode_NotExtended:
		case StatusCode_NetworkAuthRequired:
        {
            HttpInfo->Response.StatusCode = (status_code)Code;
            Result = true;
        } break;
        
        default:
        {
            HttpInfo->Response.StatusCode = StatusCode_InternalServerError;
        } break;
    }
    
    return Result;
}

extern bool
ExternalSetContentType(ts_http* Http, char* ContentType)
{
	http_info* HttpInfo = (http_info*)Http->Object;
	string Dst = StringBuffer(HttpInfo->Response.ContentTypeBuffer, 0,
	sizeof(HttpInfo->Response.ContentTypeBuffer));
	string Src = String(ContentType, StrLen(ContentType));
	
	bool Result = false;
    if (CopyString(&Dst, Src))
	{
		HttpInfo->Response.ContentTypeSize = Dst.Size;
		Result = true;
	}
	return Result;
}

extern void
ExternalSetContentSize(ts_http* Http, size_t Size)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    HttpInfo->Response.ContentSize = Size;
}

extern void
ExternalSetCookiesSize(ts_http* Http, size_t Size)
{
    http_info* HttpInfo = (http_info*)Http->Object;
    HttpInfo->Response.CookiesSize = Size;
}
