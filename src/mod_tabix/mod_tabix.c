#include <zlib.h>  
#include <stdio.h> 
#include <stdint.h>  
#include "htslib/kseq.h"
#include "htslib/kstring.h"
#include "htslib/tbx.h"
#include "r_utils.h"

// STEP 1: declare the type of file handler and the read() function  


struct tabix_callback_t
	{
	request_rec *r;
	int64_t count;
	int64_t limit;
	const char* jsonp_callback;
	void (*startdocument)( struct tabix_callback_t*);
	void (*enddocument)( struct tabix_callback_t*);
	void (*startheader)( struct tabix_callback_t*);
	int (*header)( struct tabix_callback_t*,const kstring_t *line);
	void (*enddheader)( struct tabix_callback_t*);
	void (*startbody)( struct tabix_callback_t*);
	void (*endbody)( struct tabix_callback_t*);
	void (*error)( struct tabix_callback_t*,const char* msg);
	int (*show)( struct tabix_callback_t*,const kstring_t *line);
	};



/** PLAIN handlers ***************************************************/

static void plainStart( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/plain");
	}

static void plainDoNothing( struct tabix_callback_t* handler)
	{
	}

static void plainError( struct tabix_callback_t* handler,const char* msg)
	{
	}
		
static int plainPrint( struct tabix_callback_t* handler,const kstring_t *line)
	{
	if(line!=NULL && line->s!=NULL) ap_rputs(line->s,handler->r);
	return ap_rputc('\n',handler->r);
	}
#ifdef TODOXXXXXXXXXXXXXXXXXXXx
/** XML handlers ***************************************************/

static void xmlStart( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/xml");
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!-- version " MOD_BIO_VERSION "-->\n<tabixs>\n",handler->r);
	}


static void xmlEnd( struct tabix_callback_t* handler)
	{
	ap_rputs("</tabixs>\n",handler->r);
	}

static void xmlError( struct tabix_callback_t* handler,const char* msg)
	{
	}

		
static int xmlShow( struct tabix_callback_t* handler,const kstring_t *seq)
	{
	ap_rputs("<tabix>",handler->r);
	XML_STR("name",seq->name.s);
	XML_STR("comment",seq->comment.s);
	XML_STR("seq",seq->seq.s);
	XML_STR("qual",seq->qual.s);
	return ap_rputs("</tabix>\n",handler->r);
	}
#undef XML_STR

/** json handlers ***************************************************/
static void jsonStart( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_JSON);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(handler->jsonp_callback,handler->r);
		ap_rputc('(',handler->r);
		}
	ap_rputs("[\n",handler->r);
	}


static void jsonEnd( struct tabix_callback_t* handler)
	{
	if(handler->count>0) ap_rputs("\n",handler->r);
	ap_rputs("]\n",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}

static void jsonError( struct tabix_callback_t* handler,const char* msg)
	{
	}
		
static int jsonShow( struct tabix_callback_t* handler,const kstring_t *seq)
	{

	return 0;
	}
/** HTML handlers ***************************************************/
static void htmlStart( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_HTML);
	ap_rputs("<html>",handler->r);
	ap_rputs("<head>",handler->r);
	ap_rputs("<style>",handler->r);
	ap_rputs(
		".tabixs{white-space:pre;font-family:monospace;}\n"
		".ba{color:red;}\n"
		".bt{color:green;}\n"
		".bg{color:yellow;}\n"
		".bc{color:blue;}\n"
		".bn{color:black;}\n"
		".seqname{color:black;}\n"
		".seqqual{color:gray;}\n"
		,handler->r);
	ap_rputs("</style>",handler->r);
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
		
	ap_rputs("<div class='tabixs'>",handler->r);
	}


static void htmlEnd( struct tabix_callback_t* handler)
	{
	ap_rputs("</div><div>version:" MOD_BIO_VERSION "</div></body></html>\n",handler->r);
	}

static void htmlError( struct tabix_callback_t* handler,const char* msg)
	{
	}
		
static int htmlShow(
	    struct tabix_callback_t* handler,
	    const kstring_t *seq
	    )
	{
	if(seq->qual.s!=NULL) ap_xmlPuts(seq->qual.s,handler->r); 
	return ap_rputs("\n",handler->r);
	}
#endif

/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int tabix_handler(request_rec *r);

/* Define our module as an entity and assign a function for registering hooks  */

module AP_MODULE_DECLARE_DATA   tabix_module =
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
    	ap_hook_handler(tabix_handler, NULL, NULL, APR_HOOK_LAST);
	}



static int tabix_handler(request_rec *r)
    {
    htsFile *fp=NULL;
    tbx_t *tbx = NULL;
    hts_itr_t *itr=NULL;
    HttpParamPtr httParams=NULL;
    kstring_t line = {0,0,0};
    int print_header=1;
    int print_body=1;

    int http_status=OK;
    if (!r->handler || strcmp(r->handler, "tabix-handler")) return (DECLINED);
    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
    if( !(
    	str_ends_with(r->canonical_filename,".gz")
       	))  return DECLINED;
   
    httParams = HttpParamParseGET(r); 
    if(httParams==NULL) return DECLINED;
   
   
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{
    	struct tabix_callback_t handler;
    	const char* format=HttpParamGet(httParams,"format");
    	const char* limit=HttpParamGet(httParams,"limit");
    	const char* region=HttpParamGet(httParams,"region");
    	memset((void*)&handler,0,sizeof(struct tabix_callback_t));
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
    	/*else if(strcmp(format,"xml")==0)
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
    	 	}*/
    	 else
    	 	{
    	 	handler.startdocument= plainStart;
    	 	handler.enddocument= plainDoNothing;
    	 	handler.error= plainError;
    	 	handler.startbody=plainDoNothing;
    	 	handler.endbody=plainDoNothing;
    	 	handler.show= plainPrint;
    	 	handler.startheader=plainDoNothing;
    	 	handler.enddheader=plainDoNothing;
    	 	handler.header= plainPrint;
    	 	}
    	
    	fp=hts_open(r->canonical_filename,"r");
    	if(fp==NULL)
    		{
    		http_status=HTTP_INTERNAL_SERVER_ERROR;
    		break;
    		}
    	//read index
    	tbx = tbx_index_load(r->canonical_filename);
    	if(tbx==NULL)
		{
		http_status=HTTP_INTERNAL_SERVER_ERROR;
		break;
		}
    	if(region!=NULL)
    		{
    		itr = tbx_itr_querys(tbx,region);
    		if(itr==NULL)
    		    {
    		    http_status=HTTP_INTERNAL_SERVER_ERROR;
    		    break;
    		    }
    		}


    	handler.startdocument(&handler);
    	if(print_header)
    	    {
    	    handler.startheader(&handler);
    	    while ( hts_getline(fp, KS_SEP_LINE, &line) >= 0 )
    	            {
		    if ( !line.l || line.s[0]!=tbx->conf.meta_char ) break;
		    handler.header(&handler,&line);
    	            }
    	    handler.enddheader(&handler);
    	    }
    	if(print_body)
    	    {
    	    handler.startbody(&handler);
	    if(itr!=NULL)
		{
		while ((handler.limit==-1 || handler.count< handler.limit) && tbx_itr_next(fp, tbx, itr, &line) >= 0)
			{
			if(handler.show(&handler,&line)<0) break;
			handler.count++;
			}

		}
	    else
		{
		while ((handler.limit==-1 || handler.count< handler.limit) && \
			hts_getline(fp, KS_SEP_LINE, &line) >= 0)
			{
			if(handler.show(&handler,&line)<0) break;
			handler.count++;
			}
		}
	    handler.endbody(&handler);
    	    }
	handler.enddocument(&handler);
    	} while(0);/* always abort */
    
    
    //cleanup
    if(itr!=NULL) tbx_itr_destroy(itr);
    HttpParamFree(httParams);
    free(line.s);
    if(fp!=NULL) hts_close(fp);
    if(tbx!=NULL) tbx_destroy(tbx);
    return http_status;
    }
    
   
