#ifndef APACHE_UTILS_H
#define APACHE_UTILS_H

/* Include the required headers from httpd */
#include <httpd.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_request.h>
#include <http_config.h>
#include <http_log.h>

/* max number of records to be fetched */
#define DEFAULT_LIMIT_RECORDS 100 

int str_ends_with(const char*,const char*);


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
int ap_xmlPutc(char c,request_rec *r);

/* debug */
#define TRACEINFO(r,...) \
	ap_log_error(__FILE__, __LINE__, 0,0,r->server,##__VA_ARGS__)


#endif

