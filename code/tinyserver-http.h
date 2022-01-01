#if !defined(HTTP_PARSE_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define HTTP_PARSE_H


#define JumpCLRF(Buf, Cur) Cur += ((Buf).Size-Cur >= 2) ? \
((Buf).Ptr[Cur] == '\r' ? 2 : 1) : 0;
#define JumpBackCLRF(Buf, Cur) Cur -= (Cur >= 2) ? (Buf.Ptr[Cur-2] == '\r' ? 2 : 1) : 0;
#define EvalStr(Str) if (Str.Size == 0) goto evalstr_fail;
#define Eval(Expression) if (!(Expression)) goto eval_fail;

#define IMF_DATE_LENGTH 30
#define ERROR_500_MESSAGE \
"HTTP/1.1 500 Internal Server Error\r\n"\
"Connection: Closed\r\n"\
"\r\n"


typedef enum
{
    WeekDay_Sunday,
    WeekDay_Monday,
    WeekDay_Tuesday,
    WeekDay_Wednesday,
    WeekDay_Thursday,
    WeekDay_Friday,
    WeekDay_Saturday
} week_day;

typedef struct
{
    u32 Year;
    u32 Month;
    u32 Day;
    u32 Hour;
    u32 Minute;
    u32 Second;
    week_day WeekDay;
} time_format;

typedef enum
{
    ErrType_OK,
    ErrType_AppInvalid,
    ErrType_Auth,
    ErrType_Conn,
    ErrType_Entity,
    ErrType_FileNotFound,
    ErrType_FileSize,
    ErrType_Forbid,
    ErrType_Memory,
    ErrType_Method,
    ErrType_Query,
    ErrType_RequestIncomplete,
    ErrType_RequestInvalid,
    ErrType_Server,
    ErrType_Timeout,
} err_type;

typedef enum
{
    StatusCode_Continue = 100,
    StatusCode_SwitchingProtocol = 101,
    
    StatusCode_OK = 200,
    StatusCode_Created = 201,
    StatusCode_Accepted = 202,
    StatusCode_NonAuthInfo = 203,
    StatusCode_NoContent = 204,
    StatusCode_ResetContent = 205,
    
    StatusCode_MultipleChoices = 300,
    StatusCode_MovedPermanently = 301,
    StatusCode_Found = 302,
    StatusCode_SeeOther = 303,
    StatusCode_NoModified = 304,
    StatusCode_UseProxy = 305,
    StatusCode_TempRedirect = 307,
    StatusCode_PermRedirect = 308,
    
    StatusCode_BadRequest = 400,
    StatusCode_Unauthorized = 401,
    StatusCode_PaymentRequired = 402,
    StatusCode_Forbidden = 403,
    StatusCode_NotFound = 404,
    StatusCode_MethodNotAllowed = 405,
    StatusCode_NotAcceptable = 406,
    StatusCode_ProxyAuthRequired = 407,
    StatusCode_RequestTimeout = 408,
    StatusCode_Conflict = 409,
    StatusCode_Gone = 410,
    StatusCode_LengthRequired = 411,
    StatusCode_PreconditionFailed = 412,
    StatusCode_PayloadTooLarge = 413,
    StatusCode_URITooLong = 414,
    StatusCode_UnsupportedMediaType = 415,
    StatusCode_RangeNotSatisfiable = 416,
    StatusCode_ExpectationFailed = 417,
    StatusCode_ImATeapot = 418,
    StatusCode_MisdirectedRequest = 421,
    StatusCode_TooEarly = 425,
    StatusCode_UpgradeRequired = 426,
    StatusCode_PreconditionRequired = 428,
    StatusCode_TooManyRequests = 429,
    StatusCode_ReqHeaderTooLarge = 431,
    StatusCode_LoginTimeout = 440,
    StatusCode_UnavailableForLegalReasons = 451,
    
    StatusCode_InternalServerError = 500,
    StatusCode_NotImplemented = 501,
    StatusCode_BadGateway = 502,
    StatusCode_ServiceUnavailable = 503,
    StatusCode_GatewayTimeout = 504,
    StatusCode_HttpVersionNotSupported = 505,
    StatusCode_InsufficientStorage = 507,
    StatusCode_NotExtended = 510,
    StatusCode_NetworkAuthRequired = 511
} status_code;

typedef enum
{
    HttpVerb_Unknown,
    HttpVerb_Get,
    HttpVerb_Head,
    HttpVerb_Post,
    HttpVerb_Put,
    HttpVerb_Delete,
    HttpVerb_Connect,
    HttpVerb_Options,
    HttpVerb_Trace,
    HttpVerb_Patch,
} http_verb;

typedef enum
{
    MimeType_Unknown,
    MimeType_Application,
    MimeType_Audio,
    MimeType_Font,
    MimeType_Image,
    MimeType_Message,
    MimeType_Model,
    MimeType_Multipart,
    MimeType_Text,
    MimeType_Video
} mime_type;

typedef struct
{
    http_verb Verb;
    string Uri;
    string Path;
    string Query;
    string Version;
    
    usz HeaderSize;
    usz InfoCount;
    char* FirstInfo;
    char* LastInfo;
    
    usz BodySize;
    mime_type BodyType;
} request;

typedef struct
{
    string FieldName;
    string Filename;
    //string FileType;
    string Charset;
    
    void* Data;
    u32 DataLen;
} form_field;

#pragma pack(push, 1)
typedef struct
{
    u8 IsFile;
    u8 FieldNameOffset;
    u16 FieldNameLen;
    u8 FilenameOffset;
    u16 FilenameLen;
    u8 CharsetOffset;
    u16 CharsetLen;
    u8 DataOffset;
    u32 DataLen;
    u16 NextFieldOffset;
} field_parser;
#pragma pack(pop)

typedef enum
{
    FormParse_FirstLine,
    FormParse_SecondLine,
    FormParse_ThirdLine,
    FormParse_Data,
    FormParse_Complete,
    FormParse_Error
} form_parse;

typedef struct
{
    usz FieldCount;
    field_parser* FirstField;
    field_parser* LastField;
} form;

typedef enum
{
    CookieAttr_ExpDate = 0x1,
    CookieAttr_MaxAge = 0x2,
    CookieAttr_Domain = 0x4,
    CookieAttr_Path = 0x8,
    CookieAttr_Secure = 0x10,
    CookieAttr_HttpOnly = 0x20,
    CookieAttr_SameSiteStrict = 0x40,
    CookieAttr_SameSiteLax = 0x80,
    CookieAttr_SameSiteNone = 0x100
} cookie_attr;

typedef struct
{
    string  Name;
    string Value;
    
    time_format ExpDate;
    u32 MaxAge;
    string Domain;
    string Path;
    
    u32 AttrFlags;
} cookie;

typedef struct
{
    char* Header;
    usz HeaderSize;
    
    status_code StatusCode;
    
    usz FileHandle;
    char* Content;
    usz ContentSize;
    string ContentType;
    
    char* Cookies;
    usz CookiesSize;
} response;

#endif
