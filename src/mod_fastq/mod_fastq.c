/**
 * Author: Pierre Lindenbaum PhD
 * April 2014
 * Motivation: apache2 module for Bioinformatics
 * WWW: http://github.com/lindenb/mod_bio
 */
 #include <zlib.h>  
#include <stdio.h> 
#include <stdint.h>  
#include "htslib/kseq.h"
#include "htslib/kstring.h"
#include "r_utils.h"

// STEP 1: declare the type of file handler and the read() function
KSEQ_INIT(gzFile, gzread)

struct fastq_callback_t
	{
	request_rec *r;
	int64_t count;
	int64_t limit;
	const char* jsonp_callback;
	void (*startdocument)( struct fastq_callback_t*);
	void (*enddocument)( struct fastq_callback_t*);
	void (*error)( struct fastq_callback_t*,const char* msg);
	int (*show)( struct fastq_callback_t*,const kseq_t *seq);
	};
/** PLAIN handlers ***************************************************/

static void plainStart( struct fastq_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/plain");
	}

static void plainEnd( struct fastq_callback_t* handler)
	{
	}

static void plainError( struct fastq_callback_t* handler,const char* msg)
	{
	}

		
static int plainShow( struct fastq_callback_t* handler,const kseq_t *seq)
	{
	ap_rputc('@',handler->r);
	if(seq->name.s!=NULL)
		{
		ap_rputs(seq->name.s,handler->r);
		if(seq->comment.s!=NULL)
			{
			ap_rputc(' ',handler->r);
			ap_rputs(seq->comment.s,handler->r);
			}	
		}
	ap_rputc('\n',handler->r);
	if(seq->seq.s!=NULL) ap_rputs(seq->seq.s,handler->r); 
	ap_rputs("\n+\n",handler->r);
	if(seq->qual.s!=NULL) ap_rputs(seq->qual.s,handler->r); 
	return ap_rputc('\n',handler->r);
	}

/** XML handlers ***************************************************/

static void xmlStart( struct fastq_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/xml");
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!-- version " MOD_BIO_VERSION "-->\n<fastqs>\n",handler->r);
	}


static void xmlEnd( struct fastq_callback_t* handler)
	{
	ap_rputs("</fastqs>\n",handler->r);
	}

static void xmlError( struct fastq_callback_t* handler,const char* msg)
	{
	ap_rputs("<error>",handler->r);
	ap_xmlPuts(msg,handler->r);
	ap_rputs("</error>\n",handler->r);
	}
#define XML_STR(tag,s) \
	if( (s)!=NULL &&  (s)[0]!=0) \
		{\
		ap_rputs("<"  tag ">",handler->r);\
		ap_xmlPuts((s),handler->r);\
		ap_rputs("</"  tag ">",handler->r);\
		}
		
static int xmlShow( struct fastq_callback_t* handler,const kseq_t *seq)
	{
	ap_rputs("<fastq>",handler->r);
	XML_STR("name",seq->name.s);
	XML_STR("comment",seq->comment.s);
	XML_STR("seq",seq->seq.s);
	XML_STR("qual",seq->qual.s);
	return ap_rputs("</fastq>\n",handler->r);
	}
#undef XML_STR

/** json handlers ***************************************************/
static void jsonStart( struct fastq_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_JSON);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(handler->jsonp_callback,handler->r);
		ap_rputc('(',handler->r);
		}
	ap_rputs("[\n",handler->r);
	}


static void jsonEnd( struct fastq_callback_t* handler)
	{
	if(handler->count>0) ap_rputs("\n",handler->r);
	ap_rputs("]\n",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}

static void jsonError( struct fastq_callback_t* handler,const char* msg)
	{
	}
		
static int jsonShow( struct fastq_callback_t* handler,const kseq_t *seq)
	{
	if(handler->count>0) ap_rputs(",\n",handler->r);
	ap_rputs("{\"name\":",handler->r);
	ap_jsonQuote(seq->name.s,handler->r);
	if(seq->comment.s!=NULL)
		{
		ap_rputs(",\"comment\":",handler->r);
		ap_jsonQuote(seq->comment.s,handler->r);
		}
	ap_rprintf(handler->r,",\"len\":%d,\"seq\":", (int)(seq->seq.l));
	ap_jsonQuote(seq->seq.s,handler->r);
	ap_rputs(",\"qual\":",handler->r);
	ap_jsonQuote(seq->qual.s,handler->r);
	return ap_rputc('}',handler->r);
	}
/** HTML handlers ***************************************************/
static void htmlStart( struct fastq_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_HTML);
	ap_rputs("<!doctype html>\n<html lang=\"en\">",handler->r);
	ap_rputs("<head>",handler->r);
	printDefaulthtmlHead(handler->r);
	ap_rputs("</head>",handler->r);
	ap_rputs("<body>",handler->r);
	ap_rputs("<form>"
		"<label for='format'>Format:</label> <select  id='format' name='format'>"
		"<option value='html'>html</option>"
		"<option value='json'>json</option>"
		"<option value='xml'>xml</option>"
		"<option value='text'>text</option>"
		"</select> "
		"<label for='limit'>Limit:</label> <input id='limit' name='limit' type='number' value='10'/> <input type='submit'/></form>"
		,handler->r);
		
	ap_rputs("<div class='filebody'>",handler->r);
	}


static void htmlEnd( struct fastq_callback_t* handler)
	{
	ap_rputs("</div>",handler->r);
	ap_rputs(html_address,handler->r);
	ap_rputs("</body></html>\n",handler->r);
	}

static void htmlError( struct fastq_callback_t* handler,const char* msg)
	{
	}
		
static int htmlShow( struct fastq_callback_t* handler,const kseq_t *seq)
	{
	ap_rputs("<span class='seqname'>@",handler->r);
	if(seq->name.s!=NULL)
		{
		ap_xmlPuts(seq->name.s,handler->r);
		if( seq->comment.s !=NULL)
			{
			ap_rputc(' ',handler->r);
			ap_xmlPuts(seq->comment.s,handler->r);
			}	
		}
	ap_rputs("</span>\n",handler->r);
	if(seq->seq.s!=NULL)
		{
		size_t i=0;
		for(i=0;i< seq->seq.l;++i)
			{
			if(i==0 || toupper(seq->seq.s[i])!=toupper(seq->seq.s[i-1]))
				{
				if(i>0) ap_rputs("</span>",handler->r);
				ap_rprintf(handler->r,"<span class='b%c'>",tolower(seq->seq.s[i]));
				}
			ap_rputc(seq->seq.s[i],handler->r);
			}
		}
	ap_rputs("</span>\n+\n<span class='seqqual'>",handler->r);
	if(seq->qual.s!=NULL) ap_xmlPuts(seq->qual.s,handler->r); 
	return ap_rputs("</span>\n",handler->r);
	}


/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int fastq_handler(request_rec *r);

/* Define our module as an entity and assign a function for registering hooks  */

module AP_MODULE_DECLARE_DATA   fastq_module =
	{
	    STANDARD20_MODULE_STUFF,
	    NULL,            // Per-directory configuration handler
	    NULL,            // Merge handler for per-directory configurations
	    NULL,            // Per-server configuration handler
	    NULL,            // Merge handler for per-server configurations
	    NULL,            // Any directives we may have for httpd
	    register_hooks   // Our hook registering function
	};


static void register_hooks(apr_pool_t *pool) 
	{
    	ap_hook_handler(fastq_handler, NULL, NULL, APR_HOOK_LAST);
	}


static int fastq_handler(request_rec *r)
    {
    gzFile fp=NULL;  
    kseq_t *seq=NULL;  
    HttpParamPtr httParams=NULL;
    int http_status=OK;
    if (!r->handler || strcmp(r->handler, "fastq-handler")) return (DECLINED);
    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
    if( !(
    	str_ends_with(r->canonical_filename,".fastq") ||
        str_ends_with(r->canonical_filename,".fastq.gz") ||
        str_ends_with(r->canonical_filename,".fq") ||
        str_ends_with(r->canonical_filename,".fq.gz")
       	))  return DECLINED;
   
    httParams = HttpParamParseGET(r); 
    if(httParams==NULL) return DECLINED;
   
   
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{
	int l;
    	struct fastq_callback_t handler;
    	const char* format=HttpParamGet(httParams,"format");
    	const char* limit=HttpParamGet(httParams,"limit");
    	memset((void*)&handler,0,sizeof(struct fastq_callback_t));
    	handler.r=r;
    	handler.limit=DEFAULT_LIMIT_RECORDS;
    	
    	
    	if(limit!=NULL)
    		{
    		handler.limit=atol(limit);
    		}

    	if(format==NULL)
    		{
    		http_status=DECLINED;
    		break;
    		}
    	 else if(strcmp(format,"xml")==0)
    	 	{
    	 	handler.startdocument= xmlStart;
    	 	handler.enddocument= xmlEnd;
    	 	handler.error= xmlError;
    	 	handler.show= xmlShow;
    	 	}
    	 else if(strcmp(format,"json")==0 || strcmp(format,"jsonp")==0)
    	 	{
    	 	handler.jsonp_callback=HttpParamGet(httParams,"callback");
    	 	handler.startdocument= jsonStart;
    	 	handler.enddocument= jsonEnd;
    	 	handler.error= jsonError;
    	 	handler.show= jsonShow;
    	 	}
    	 else if(strcmp(format,"html")==0)
    	 	{
    	 	handler.startdocument= htmlStart;
    	 	handler.enddocument= htmlEnd;
    	 	handler.error= htmlError;
    	 	handler.show= htmlShow;
    	 	}
    	 else
    	 	{
    	 	handler.startdocument= plainStart;
    	 	handler.enddocument= plainEnd;
    	 	handler.error= plainError;
    	 	handler.show= plainShow;
    	 	}
    	
    
    	fp = gzopen(r->canonical_filename, "r");
    	if(fp==NULL)
    		{
    		http_status=HTTP_INTERNAL_SERVER_ERROR;
    		break;
    		}
    	seq = kseq_init(fp); //  initialize seq  
    	if(seq==NULL)
    		{
    		http_status=HTTP_INTERNAL_SERVER_ERROR;
    		break;
    		}

    	handler.startdocument(&handler);
    	while ((handler.limit==-1 || handler.count< handler.limit) && (l = kseq_read(seq)) >= 0)
    		{ 
    		if(handler.show(&handler,seq)<0) break;
		handler.count++;
		} 
	handler.enddocument(&handler);
    	} while(0);/* always abort */
    
    
    //cleanup
    HttpParamFree(httParams);
    if(seq!=NULL) kseq_destroy(seq); //destroy seq  
    if(fp!=NULL) gzclose(fp); // close the file handler  
    return http_status;
    }
    
   
