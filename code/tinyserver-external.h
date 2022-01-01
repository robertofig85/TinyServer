#if !defined(TINYSERVER_EXTERNAL_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define TINYSERVER_EXTERNAL_H

typedef string ts_string;
typedef form_field ts_form_field;
typedef week_day ts_week_day;
typedef time_format ts_time_format;
typedef mime_type ts_mime_type;
typedef http_verb ts_http_verb;
typedef cookie_attr ts_cookie_attr;
typedef cookie ts_cookie;
typedef form ts_form;

typedef struct _ts_http ts_http;
struct _ts_http
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
};

#endif
