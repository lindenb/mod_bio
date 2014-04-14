#include <zlib.h>  
#include <stdio.h> 
#include <stdint.h>  
#include "htslib/faidx.h"
#include "r_utils.h"




struct faidx_callback_t
	{
	request_rec *r;
	int length;
	ChromStartEnd region;
	const char* jsonp_callback;
	void (*startdocument)( struct faidx_callback_t*);
	void (*enddocument)( struct faidx_callback_t*);
	int (*show)( struct faidx_callback_t*,const char *seq,int len);
	};
/** PLAIN handlers ***************************************************/

static void plainStart( struct faidx_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/plain");
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rprintf(handler->r,">%s:%d-%d",
		    handler->region.chromosome,
		    handler->region.p_beg_i0,
		    handler->region.p_end_i0
		);
	    }
	handler->length=0;
	}

static void plainEnd( struct faidx_callback_t* handler)
	{
	ap_rputc('\n',handler->r);
	}

		
static int plainShow(
	    struct faidx_callback_t* handler,
	    const char *seq,
	    int len
	    )
	{
	int i=0;
	if(seq==NULL) return -1;
	for(i=0;i< len;++i)
	    {
	    if(handler->length%50==0) ap_rputc('\n',handler->r);
	    if(ap_rputc(seq[i],handler->r)==-1)
		{
		return -1;
		}
	    handler->length++;
	    }
	return 0;
	}

/** XML handlers ***************************************************/

static void xmlStart( struct faidx_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/xml");
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<faidx>\n<sequence ",handler->r);
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rputs(" chrom=\"",handler->r);
	    ap_xmlPuts(handler->region.chromosome,handler->r);
	    ap_rprintf(handler->r,"\" start=\"%d\" end=\"%d\"",
		    handler->region.p_beg_i0,
		    handler->region.p_end_i0
		);
	    }
	ap_rputc('>',handler->r);
	}


static void xmlEnd( struct faidx_callback_t* handler)
	{
	ap_rputs("</sequence>\n</faidx>\n",handler->r);
	}

		
static int xmlShow(
	    struct faidx_callback_t* handler,
	    const char *seq,
	    int len
	    )
	{
	int i=0;
	if(seq==NULL) return -1;
	for(i=0;i< len;++i)
	    {
	    if(ap_rputc(seq[i],handler->r)==-1)
		{
		return -1;
		}
	    handler->length++;
	    }
	return 0;
	}



/** json handlers ***************************************************/
static void jsonStart( struct faidx_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_JSON);
	if(handler->jsonp_callback!=NULL)
			{
			ap_rputs(handler->jsonp_callback,handler->r);
			ap_rputc('(',handler->r);
			}

	ap_rputs("{",handler->r);
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rputs("\"chrom\":",handler->r);
	    ap_jsonQuote(handler->region.chromosome,handler->r);
	    ap_rprintf(handler->r,",\"start\":%d,\"end\":%d",
		    handler->region.p_beg_i0,
		    handler->region.p_end_i0
		);
	    ap_rputs(",\"sequence\":\"",handler->r);
	    }
	}


static void jsonEnd( struct faidx_callback_t* handler)
	{
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rputc('\"',handler->r);
	    }
	ap_rputs("}",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}


		
static int jsonShow(
	    struct faidx_callback_t* handler,
	    const char *seq,
	    int len
	    )
	{
	int i=0;
	if(seq==NULL) return -1;
	for(i=0;i< len;++i)
	    {
	    if(ap_rputc(seq[i],handler->r)==-1)
		{
		return -1;
		}
	    handler->length++;
	    }
	return 0;
	}

/** HTML handlers ***************************************************/
static void htmlStart( struct faidx_callback_t* handler)
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
		"<label for='region'>Region:</label> <input id='region' name='region' placeholder='chrom:start-end' "
		,handler->r);
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rputs(" value=\"",handler->r);
	    ap_xmlPuts(handler->region.chromosome,handler->r);
	    ap_rprintf(handler->r,":%d-%d",
		    handler->region.p_beg_i0,
		    handler->region.p_end_i0
		);
	    ap_rputs("\"",handler->r);
	    }
	ap_rputs("/>"
		"<input type='submit'/></form><div>"
		,handler->r);
	if(handler->region.chromosome!=NULL)
	    {
	    ap_rputs("<h3>",handler->r);
	    ap_xmlPuts(handler->region.chromosome,handler->r);
	    ap_rprintf(handler->r,":%d-%d",
		    handler->region.p_beg_i0,
		    handler->region.p_end_i0
		);
	    ap_rputs("</h3><div class='faidx'>",handler->r);
	    }
	}


static void htmlEnd( struct faidx_callback_t* handler)
	{
	if(handler->region.chromosome!=NULL)
    	    {
	    ap_rputs("</div>",handler->r);
    	    }
	ap_rputs("</div>",handler->r);
	ap_rputs(html_address,handler->r);
	ap_rputs("</body></html>\n",handler->r);
	}

		
static int htmlShow(
	    struct faidx_callback_t* handler,
	    const char *seq,
	    int len
	    )
	{
	int i=0;
	if(seq==NULL) return -1;
	for(i=0;i< len;++i)
	    {
	    if(handler->length>0 && handler->length%60==0)
		{
		ap_rputs("<br/>",handler->r);
		}
	    if(ap_rputc(seq[i],handler->r)==-1)
		{
		return -1;
		}
	    handler->length++;
	    }
	return 0;
	}


/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int faidx_handler(request_rec *r);

/* Define our module as an entity and assign a function for registering hooks  */

module AP_MODULE_DECLARE_DATA   faidx_module =
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
    	ap_hook_handler(faidx_handler, NULL, NULL, APR_HOOK_LAST);
	}


static int faidx_handler(request_rec *r)
    {
    struct faidx_callback_t handler;
    faidx_t *faidx=NULL;
    HttpParamPtr httParams=NULL;
    ChromStartEnd chromStartEnd;
    int http_status=OK;
    memset((void*)&handler,0,sizeof(struct faidx_callback_t));
    memset((void*)&chromStartEnd,0,sizeof(ChromStartEnd));


    if (!r->handler || strcmp(r->handler, "faidx-handler")) return (DECLINED);

    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
    if( !(
    	str_ends_with(r->canonical_filename,".fasta") ||
        str_ends_with(r->canonical_filename,".fa")
       	))  return DECLINED;
    /* check file exists */
    if((http_status=fileExists(r->canonical_filename))!=OK)
	{
	return http_status;
	}
    /* check fasta index exist */
    if((http_status=fileExtExists(r->canonical_filename,".fai"))!=OK)
    	{
    	return http_status;
    	}
    httParams = HttpParamParseGET(r); 
    if(httParams==NULL) return DECLINED;
    handler.r=r;
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{
	const char* format=HttpParamGet(httParams,"format");
    	const char* region=HttpParamGet(httParams,"region");

    	if(format==NULL)
    		{
    		http_status=DECLINED;
    		break;
    		}
    	 else if(strcmp(format,"xml")==0)
    	 	{
    	 	handler.startdocument= xmlStart;
    	 	handler.enddocument= xmlEnd;
    	 	handler.show= xmlShow;
    	 	}
    	 else if(strcmp(format,"json")==0 || strcmp(format,"jsonp")==0)
    	 	{
    	 	handler.jsonp_callback=HttpParamGet(httParams,"callback");
    	 	handler.startdocument= jsonStart;
    	 	handler.enddocument= jsonEnd;
    	 	handler.show= jsonShow;
    	 	}
    	 else if(strcmp(format,"html")==0)
    	 	{
    	 	handler.startdocument= htmlStart;
    	 	handler.enddocument= htmlEnd;
    	 	handler.show= htmlShow;
    	 	}
    	 else
    	 	{
    	 	handler.startdocument= plainStart;
    	 	handler.enddocument= plainEnd;
    	 	handler.show= plainShow;
    	 	}
    	

    	faidx = fai_load(r->canonical_filename);
    	if(faidx==NULL)
    		{
    		http_status=HTTP_INTERNAL_SERVER_ERROR;
    		break;
    		}

    	if(region!=NULL)
    	    {
    	    if(parseRegion(region,&(handler.region))!=0)
    		{
    		http_status=HTTP_BAD_REQUEST;
    		break;
    		}
    	    }


    	handler.startdocument(&handler);
    	if(region!=NULL)
    	    {
	    int p_curr=handler.region.p_beg_i0;
	    while(p_curr<= handler.region.p_end_i0)
		{
		int ret=0;
	        int len=0;
		int p_next=MIN(p_curr+1000,handler.region.p_end_i0);
		char* dnastring= faidx_fetch_seq(faidx,
		    handler.region.chromosome,
		    p_curr,
		    p_next
		    ,&len);
		if(dnastring==NULL) break;
		if(len>0)
		    {
		    ret=handler.show(&handler,dnastring,len);
		    }
		free(dnastring);
		if(len<(p_next-p_curr)) break;
		p_curr+=len;
		if(ret<0) break;
		}
    	    }
	handler.enddocument(&handler);
    	} while(0);/* always abort */
    
    
    //cleanup
    HttpParamFree(httParams);
    if(faidx!=NULL) fai_destroy(faidx);
    free(handler.region.chromosome);
    return http_status;
    }
    
   
