#ifndef APACHE_UTILS_H
#define APACHE_UTILS_H

/* Include the required headers from httpd */
#include <httpd.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_request.h>
#include <http_config.h>
#include <http_log.h>
#include "mod_bio_version.h"

#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#endif
#ifndef MAX
#define MAX(a,b) (a>b?a:b)
#endif


#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef true
#define true TRUE
#endif
#ifndef false
#define false FALSE
#endif
#ifndef null
#define null NULL
#endif


/* max number of records to be fetched */
#define DEFAULT_LIMIT_RECORDS 100 

int str_ends_with(const char*,const char*);
int str_is_empty(const char* );

typedef struct http_param
	{
	char* key;
	char* value;
	struct http_param* next;
	}HttpParam,*HttpParamPtr;

HttpParamPtr HttpParamFree(HttpParamPtr);
HttpParamPtr HttpParamParseQuery(const char* query,size_t query_len);
HttpParamPtr HttpParamParseGET(request_rec *r);
const char* HttpParamGet(const HttpParamPtr root,const char* key);

/* xml */
int ap_xmlPuts(const char* s,request_rec *r);
int ap_xmlNPuts(const char* s,size_t n,request_rec *r);
int ap_xmlPutc(char c,request_rec *r);
/* json */
int ap_jsonQuote(const char* s,request_rec *r);
int ap_jsonNQuote(const char* s,size_t n,request_rec *r);
int ap_jsonEscapeC(char c,request_rec *r);

/* debug */
#define TRACEINFO(r,...) \
	ap_log_error(__FILE__, __LINE__, 0,0,r->server,##__VA_ARGS__)

/* apache2 stuff*/


/* mime types */

#define MIME_TYPE_JSON "application/json"
#define MIME_TYPE_XML "text/xml"
#define MIME_TYPE_TEXT "text/plain"
#define MIME_TYPE_HTML "text/html"
#define MIME_TYPE_JAVASCRIPT "application/javascript"

/* HTML fragments */
extern const char* html_address;
extern int printDefaulthtmlHead(request_rec *r);

/* file exist ? */
extern int fileExists(const char* filename);
/* file with extension exists */
extern int fileExtExists(const char* filename,const char* suffix);
/* file without extention exists */
extern int baseNameExists(const char* filename);

/* parse region 'chrom:start-end' */
typedef struct chrom_start_end_t
    {
	/* must be released with free() */
	char* chromosome;
	int p_beg_i0;
	int p_end_i0;
    } ChromStartEnd;

extern int parseRegion(const char* s,ChromStartEnd* pos);

#endif

