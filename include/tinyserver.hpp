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

struct ts_http
{
    void* Object;
    
    // Request header.
    ts_http_verb Verb;
    ts_string Path;
    ts_string Query;
    size_t HeaderCount;
    ts_string (*_TSGetHeaderByKey)(ts_http*, char*);
    ts_string (*_TSGetHeaderByIdx)(ts_http*, size_t);
    
    // Request body.
    void* Entity;
    size_t EntitySize;
    ts_mime_type EntityType;
    ts_form (*_TSParseFormData)(ts_http*);
    ts_form_field (*_TSGetFormFieldByName)(ts_form*, char*);
    ts_form_field (*_TSGetFormFieldByIdx)(ts_form*, size_t);
    
    // Response.
    ts_string (*_TSAllocContentBuffer)(ts_http*, size_t);
    ts_string (*_TSAllocCookieBuffer)(ts_http*, size_t);
    bool (*_TSRecordCookie)(ts_http*, ts_cookie*);
    bool (*_TSSetReturnCode)(ts_http*, size_t);
    void (*_TSSetContentType)(ts_http*, char*);
    void (*_TSSetContentSize)(ts_http*, size_t);
    void (*_TSSetCookieSize)(ts_http*, size_t);
    
    // C++ methods.
    ts_string GetHeaderByKey(char* A) { return _TSGetHeaderByKey(this, A); }
    ts_string GetHeaderByIdx(size_t A) { return _TSGetHeaderByIdx(this, A); }
    ts_form ParseFormData(void) { return _TSParseFormData(this); }
    ts_form_field GetFormFieldByName(ts_form* A, char* B) { return _TSGetFormFieldByName(A, B); }
    ts_form_field GetFormFieldByIdx(ts_form* A, size_t B) { return _TSGetFormFieldByIdx(A, B); }
    ts_string AllocContentBuffer(size_t A) { return _TSAllocContentBuffer(this, A); }
    ts_string AllocCookieBuffer(size_t A) { return _TSAllocCookieBuffer(this, A); }
    bool RecordCookie(ts_cookie* A) { return _TSRecordCookie(this, A); }
    bool SetReturnCode(size_t A) { return _TSSetReturnCode(this, A); }
    void SetContentType(char* A) { return _TSSetContentType(this, A); }
    void SetContentSize(size_t A) { return _TSSetContentSize(this, A); }
    void SetCookieSize(size_t A) { return _TSSetCookieSize(this, A); }
};

#endif
