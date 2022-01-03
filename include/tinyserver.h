#ifndef TINYSERVER_H
#define TINYSERVER_H


#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    char* Ptr;
    size_t Size;
    size_t MaxSize;
} ts_string;

typedef struct
{
    ts_string FieldName;
    ts_string Filename;
    ts_string Charset;
    void* Data;
    uint32_t DataLen;
} ts_form_field;

typedef enum
{
    WeekDay_Sunday,
    WeekDay_Monday,
    WeekDay_Tuesday,
    WeekDay_Wednesday,
    WeekDay_Thursday,
    WeekDay_Friday,
    WeekDay_Saturday
} ts_week_day;

typedef struct
{
    uint32_t Year;
    uint32_t Month;
    uint32_t Day;
    uint32_t Hour;
    uint32_t Minute;
    uint32_t Second;
    ts_week_day WeekDay;
} ts_time_format;

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
} ts_mime_type;

typedef enum
{
    HttpVerb_None,
    HttpVerb_Get,
    HttpVerb_Head,
    HttpVerb_Post,
    HttpVerb_Put,
    HttpVerb_Delete,
    HttpVerb_Connect,
    HttpVerb_Options,
    HttpVerb_Trace,
    HttpVerb_Patch,
} ts_http_verb;

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
} ts_cookie_attr;

typedef struct
{
    ts_string  Name;
    ts_string Value;
    
    ts_time_format ExpDate;
    uint32_t MaxAge;
    ts_string Domain;
    ts_string Path;
    
    uint32_t AttrFlags;
} ts_cookie;

typedef struct
{
    size_t FieldCount;
    void* FirstField;
    void* LastField;
} ts_form;


//==================================================================
// Main struct, from which to access the Request data, e by which 
// configure the Response parameters.
//==================================================================

typedef struct
{
    void* Object;
    
    // Request header.
    const ts_http_verb Verb;
    const ts_string Path;
    const ts_string Query;
    const size_t HeaderCount;
    ts_string (*_TSGetHeaderByKey)(ts_http*, char*);
    ts_string (*_TSGetHeaderByIdx)(ts_http*, size_t);
    
    // Request body.
    void* const Entity;
    const size_t EntitySize;
    const ts_mime_type EntityType;
    ts_form (*_TSParseFormData)(ts_http*);
    ts_form_field (*_TSGetFormFieldByName)(ts_form*, char*);
    ts_form_field (*_TSGetFormFieldByIdx)(ts_form*, size_t);
    
    // Response.
    ts_string (*_TSAllocContentBuffer)(ts_http*, size_t);
    ts_string (*_TSAllocCookiesBuffer)(ts_http*, size_t);
    bool (*_TSRecordCookie)(ts_http*, ts_cookie*);
    bool (*_TSSetReturnCode)(ts_http*, size_t);
    void (*_TSSetContentType)(ts_http*, char*);
    void (*_TSSetContentSize)(ts_http*, size_t);
    void (*_TSSetCookiesSize)(ts_http*, size_t);
} ts_http;

// C methods.
ts_string (*TSGetHeaderByKey)(ts_http*, char*);
ts_string (*TSGetHeaderByIdx)(ts_http*, size_t);
ts_form (*TSParseFormData)(ts_http*);
ts_form_field (*TSGetFormFieldByName)(ts_form*, char*);
ts_form_field (*TSGetFormFieldByIdx)(ts_form*, size_t);
ts_string (*TSAllocContentBuffer)(ts_http*, size_t);
ts_string (*TSAllocCookiesBuffer)(ts_http*, size_t);
bool (*TSRecordCookie)(ts_http*, ts_cookie*);
bool (*TSSetReturnCode)(ts_http*, size_t);
void (*TSSetContentType)(ts_http*, char*);
void (*TSSetContentSize)(ts_http*, size_t);
void (*TSSetCookiesSize)(ts_http*, size_t);

void TSInit(ts_http* Http)
{
    TSGetHeaderByKey = Http->_TSGetHeaderByKey;
    TSGetHeaderByIdx = Http->_TSGetHeaderByIdx;
    TSParseFormData = Http->_TSParseFormData;
    TSGetFormFieldByName = Http->_TSGetFormFieldByName;
    TSGetFormFieldByIdx = Http->_TSGetFormFieldByIdx;
    TSAllocContentBuffer = Http->_TSAllocContentBuffer;
    TSAllocCookiesBuffer = Http->_TSAllocCookiesBuffer;
    TSRecordCookie = Http->_TSRecordCookie;
    TSSetReturnCode = Http->_TSSetReturnCode;
    TSSetContentType = Http->_TSSetContentType;
    TSSetContentSize = Http->_TSSetContentSize;
    TSSetCookiesSize = Http->_TSSetCookiesSize;
}


#endif
