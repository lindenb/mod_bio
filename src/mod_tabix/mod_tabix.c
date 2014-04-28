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
#include "htslib/tbx.h"
#include "r_utils.h"

enum E_bio_format {E_FORMAT_UNDEFINED,E_FORMAT_BED,E_FORMAT_VCF};

struct tabix_callback_t
	{
	/* incoming request */
	request_rec *r;
	/* number of records seen so far */
	int64_t count;
	/* limit number of records */
	int64_t limit;
	/** callback for JSON-P */
	const char* jsonp_callback;
	/* tabix handler */
	tbx_t *tbx;
	/* http params */
    	HttpParamPtr httParams;
    	/** format flag */
    	int file_format;
    
	
	void (*startdocument)( struct tabix_callback_t*);
	void (*enddocument)( struct tabix_callback_t*);
	void (*startheader)( struct tabix_callback_t*);
	int (*header)( struct tabix_callback_t*,const kstring_t *line);
	void (*enddheader)( struct tabix_callback_t*);
	void (*startbody)( struct tabix_callback_t*);
	void (*endbody)( struct tabix_callback_t*);
	int (*show)( struct tabix_callback_t*,const kstring_t *line);
	};



/** PLAIN handlers ***************************************************/

static void plainStartDocument( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/plain");
	}

#define PLAIN_DO_NOTHING(fun) static void plain##fun( struct tabix_callback_t* handler) {}
	
PLAIN_DO_NOTHING(StartBody)
PLAIN_DO_NOTHING(EndBody)
PLAIN_DO_NOTHING(StartHeader)
PLAIN_DO_NOTHING(EndHeader)
PLAIN_DO_NOTHING(EndDocument)

static int plainPrintHeader( struct tabix_callback_t* handler,const kstring_t *line)
	{
	if(line!=NULL && line->s!=NULL) ap_rputs(line->s,handler->r);
	return ap_rputc('\n',handler->r);
	}
static int plainPrintBody( struct tabix_callback_t* handler,const kstring_t *line)
	{
	if(line!=NULL && line->s!=NULL) ap_rputs(line->s,handler->r);
	return ap_rputc('\n',handler->r);
	}
/** XML handlers ***************************************************/

static void xmlStartDocument( struct tabix_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_XML);
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<tabix-file git-version=\"" MOD_BIO_VERSION "\">\n",handler->r);
	}

static void xmlEndDocument( struct tabix_callback_t* handler)
	{
	ap_rputs("</tabix-file>\n",handler->r);
	}

static void xmlStartHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("<header>\n",handler->r);
	}

static int xmlPrintHeader(struct tabix_callback_t* handler,const kstring_t *line)
	{
	ap_rputs("<line>",handler->r);
	ap_xmlPuts(line->s,handler->r);
	return ap_rputs("</line>\n",handler->r);
	}

static void xmlEndHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("</header>\n",handler->r);
	}


static void xmlStartBody( struct tabix_callback_t* handler)
	{
	ap_rputs("<body>\n",handler->r);
	}

static int xmlPrintBody(struct tabix_callback_t* handler,const kstring_t *line)
	{
	int i,prev=0,n_field=0;
	ap_rputs("<tr>",handler->r);
	for(i=0;i<= line->l;++i)
		{
		if(line->s[i]=='\t' || i==line->l)
			{
			ap_rputs("<td",handler->r);
			if(n_field==0 && (handler->file_format==E_FORMAT_VCF || handler->file_format==E_FORMAT_BED))
				{
				ap_rputs(" type=\"chrom\"",handler->r);
				}
			else if(n_field==1 && handler->file_format==E_FORMAT_VCF)
				{
				ap_rputs(" type=\"pos\"",handler->r);
				}
			else if(n_field==1 && handler->file_format==E_FORMAT_BED)
				{
				ap_rputs(" type=\"start\"",handler->r);
				}
			else if(n_field==2 && handler->file_format==E_FORMAT_BED)
				{
				ap_rputs(" type=\"end\"",handler->r);
				}
			ap_rputs(">",handler->r);
			ap_xmlNPuts(&(line->s)[prev],i-prev,handler->r);
			ap_rputs("</td>",handler->r);
			if(i==line->l) break;
			++n_field;
			prev=i+1;
			}
		}
	return ap_rputs("</tr>\n",handler->r);
	}


static void xmlEndBody( struct tabix_callback_t* handler)
	{
	ap_rputs("</body>\n",handler->r);
	}



/** json handlers ***************************************************/

static void jsonStartDocument( struct tabix_callback_t* handler)
	{
	if(handler->jsonp_callback!=NULL)
			{
			ap_set_content_type(handler->r, MIME_TYPE_JAVASCRIPT);
			ap_rputs(handler->jsonp_callback,handler->r);
			ap_rputc('(',handler->r);
			}
	else
			{
			ap_set_content_type(handler->r, MIME_TYPE_JSON);
			}
	ap_rputs("{",handler->r);
	}

static void jsonEndDocument( struct tabix_callback_t* handler)
	{
	ap_rputs("}\n",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}

static void jsonStartHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("\"header\":[",handler->r);
	}

static int jsonPrintHeader(struct tabix_callback_t* handler,const kstring_t *line)
	{
	if(handler->count>0) ap_rputs(",\n",handler->r);
	return ap_jsonQuote(line->s,handler->r);
	}

static void jsonEndHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("]\n",handler->r);
	}


static void jsonStartBody( struct tabix_callback_t* handler)
	{
	ap_rputs(",\"body\":[",handler->r);
	}

static int jsonPrintBody(struct tabix_callback_t* handler,const kstring_t *line)
	{
	int i,prev=0,n_fields=0;
	if(handler->count>0) ap_rputs(",\n",handler->r);
	ap_rputs("[",handler->r);

	for(i=0;i<= line->l;++i)
		{
		if(line->s[i]=='\t' || i==line->l)
			{
			if(n_fields>0) ap_rputc(',',handler->r);
			if(handler->file_format==E_FORMAT_VCF)
				{
				switch(n_fields)
					{
					case 1: ap_rwrite(&(line->s)[prev],i-prev,handler->r);break;
					case 5: {
						if(i-prev==1 && (line->s)[prev]=='.')
							{
							ap_rputs("null",handler->r);
							}
						else
							{
							ap_rwrite((void*)&(line->s)[prev],i-prev,handler->r);
							}
						break;
						}
					default:
						{
						if(i-prev==1 && (line->s)[prev]=='.')
							{
							ap_rputs("null",handler->r);
							}
						else
							{
							ap_jsonNQuote(&(line->s)[prev],i-prev,handler->r);
							}
						break;
						}
					}
				}
			else if(handler->file_format==E_FORMAT_BED)
				{
				switch(n_fields)
					{
					case 1:
					case 2: ap_rwrite(&(line->s)[prev],i-prev,handler->r);
						break;
					default:
						ap_jsonNQuote(&(line->s)[prev],i-prev,handler->r);
						break;
						
					}
				}
			else
				{
				ap_jsonNQuote(&(line->s)[prev],i-prev,handler->r);
				}
			
			if(i==line->l) break;
			prev=i+1;
			++n_fields;
			}
		}
	return ap_rputs("]",handler->r);
	}


static void jsonEndBody( struct tabix_callback_t* handler)
	{
	ap_rputs("]",handler->r);
	}
/** HTML handlers ***************************************************/
static void htmlStartDocument( struct tabix_callback_t* handler)
	{
	const char* region=HttpParamGet(handler->httParams,"region");

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
		"<label for='limit'>Limit:</label> <input id='limit' name='limit' type='number' value='10'/> "
		"<label for='region'>Region:</label> <input id='region' name='region' placeholder='chrom:start-end' "
		,handler->r);
	
	if(region!=NULL)
	    {
	    ap_rputs(" value=\"",handler->r);
	    ap_xmlPuts(region,handler->r);
	    ap_rputs("\"",handler->r);
	    }

	ap_rputs("/> <input type='submit'/></form><div>",handler->r);
	}

static void htmlEndDocument( struct tabix_callback_t* handler)
	{
	ap_rputs("</div>",handler->r);
	ap_rputs(html_address,handler->r);
	ap_rputs("</body></html>\n",handler->r);
	}

static void htmlStartHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("<h3>Header</h3><div class=\"fileheader\">",handler->r);
	}

static int htmlPrintHeader(struct tabix_callback_t* handler,const kstring_t *line)
	{
	ap_xmlPuts(line->s,handler->r);
	return ap_rputs("<br/>",handler->r);
	}

static void htmlEndHeader( struct tabix_callback_t* handler)
	{
	ap_rputs("</div>",handler->r);
	}


static void htmlStartBody( struct tabix_callback_t* handler)
	{
	ap_rputs("<div class=\"filebody\"><table>",handler->r);
	}

static int htmlPrintBody(struct tabix_callback_t* handler,const kstring_t *line)
	{
	int i,prev=0,n_field=0;
	ap_rprintf(handler->r,"<tr class=\"row%d\">",(int)(handler->count%2));
	for(i=0;i<= line->l;++i)
		{
		if(line->s[i]=='\t' || i==line->l)
			{
			int len=i-prev;
			int only_digit=1,j;
			ap_rputs("<td",handler->r);
			if(n_field==0 && (handler->file_format==E_FORMAT_VCF || handler->file_format==E_FORMAT_BED))
				{
				ap_rputs(" class=\"tbxc\"",handler->r);
				}
			else if(n_field==1 && handler->file_format==E_FORMAT_VCF)
				{
				ap_rputs(" class=\"tbxp\"",handler->r);
				}
			else if(n_field==1 && handler->file_format==E_FORMAT_BED)
				{
				ap_rputs(" class=\"tbxs\"",handler->r);
				}
			else if(n_field==2 && handler->file_format==E_FORMAT_BED)
				{
				ap_rputs(" class=\"tbxe\"",handler->r);
				}
			ap_rputs(">",handler->r);
			for(j=2;j< len && line->l >2 ;++j)
				{
				if(!isdigit(line->s[i+j]))
					{
					only_digit=0;
					break;
					}
				}
			// ncbi rs ?
			if(line->l >2 && only_digit==1 && line->s[0]=='r' && line->s[1]=='s')
				{
				ap_rputs("<a href=\"http://www.ncbi.nlm.nih.gov/projects/SNP/snp_ref.cgi?rs=",handler->r);
				ap_rwrite((void*)&(line->s)[prev+2],len-2,handler->r);
				ap_rputs("\">",handler->r);
				ap_rwrite((void*)&(line->s)[prev],len,handler->r);
				ap_rputs("</a>",handler->r);
				}
			else
				{
				ap_xmlNPuts(&(line->s)[prev],len,handler->r);
				}
			
			ap_rputs("</td>",handler->r);
			if(i==line->l) break;
			++n_field;
			prev=i+1;
			}
		}
	return ap_rputs("</tr>\n",handler->r);
	}


static void htmlEndBody( struct tabix_callback_t* handler)
	{
	ap_rputs("</table></div>",handler->r);
	}

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

#define SETUP_HANDLER(prefix)\
	handler.startdocument= prefix ## StartDocument;\
	handler.enddocument= prefix ## EndDocument;\
	handler.startbody=prefix ## StartBody;\
	handler.endbody=prefix ## EndBody;;\
	handler.show= prefix ## PrintBody;\
	handler.startheader=prefix ## StartHeader;\
	handler.enddheader=prefix ## EndHeader;\
	handler.header= prefix ## PrintHeader;
    	 	

/**
 * tabix workhorse function
 */
static int tabix_handler(request_rec *r)
    {
    htsFile *fp=NULL;
    hts_itr_t *itr=NULL;
    kstring_t line = {0,0,0};
    int print_header=1;
    int print_body=1;
    struct tabix_callback_t handler;
    int http_status=OK;
	
	memset((void*)&handler,0,sizeof(struct tabix_callback_t));
    handler.r=r;
    handler.limit=DEFAULT_LIMIT_RECORDS;
	
    if (!r->handler || strcmp(r->handler, "tabix-handler")) return (DECLINED);
    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
     /* file must be b-gzipped */
    if( !(
    	str_ends_with(r->canonical_filename,".gz")
       	))  return DECLINED;
    /* file must be indexed with tabix */
    if( !(
    	fileExtExists(r->canonical_filename,".tbi")
       	))  return 404;
   
    
   
    handler.httParams = HttpParamParseGET(r); 
    if(handler.httParams==NULL) return DECLINED;
    handler.file_format=E_FORMAT_UNDEFINED;
    if(str_ends_with(r->canonical_filename,".vcf.gz"))
    	{
    	handler.file_format=E_FORMAT_VCF;
    	}
    else if(str_ends_with(r->canonical_filename,".bed.gz"))
    	{
    	handler.file_format=E_FORMAT_BED;
    	}
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{
    	const char* format=HttpParamGet(handler.httParams,"format");
    	const char* limit=HttpParamGet(handler.httParams,"limit");
    	const char* region=HttpParamGet(handler.httParams,"region");
    	int iterator_was_requested=FALSE;
    	
    	
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
    	 	SETUP_HANDLER(xml);
    	 	}

    	 else if(strcmp(format,"json")==0 || strcmp(format,"jsonp")==0)
    	 	{
    	 	handler.jsonp_callback=HttpParamGet(handler.httParams,"callback");
    	 	SETUP_HANDLER(json);
    	 	}
    	 else if(strcmp(format,"html")==0)
    	 	{
    	 	SETUP_HANDLER(html);
    	 	}
    	 else
    	 	{
    	 	SETUP_HANDLER(plain);
    	 	}
    	
    	fp=hts_open(r->canonical_filename,"r");
    	if(fp==NULL)
    		{
    		http_status=HTTP_NOT_FOUND;
    		break;
    		}
    	//read index
    	handler.tbx = tbx_index_load(r->canonical_filename);
    	if(handler.tbx==NULL)
			{
			http_status=HTTP_INTERNAL_SERVER_ERROR;
			break;
			}
    	if(region!=NULL && !str_is_empty(region))
    		{
    		iterator_was_requested=TRUE;
    		itr = tbx_itr_querys(handler.tbx,region);
    		}

	
    	handler.startdocument(&handler);
    	if(print_header)
    	    {
    	    handler.startheader(&handler);
    	    while ( hts_getline(fp, KS_SEP_LINE, &line) >= 0 )
    	            {
		    if ( !line.l || line.s[0]!=handler.tbx->conf.meta_char ) break;
		    handler.header(&handler,&line);
		    handler.count++;
    	            }
    	    handler.enddheader(&handler);
    	    }
    	handler.count=0;//Reset 
    	if(print_body)
    	    {
    	    handler.startbody(&handler);
		    if(iterator_was_requested)
				{
				if(itr!=NULL)
					{
					while ((handler.limit==-1 || handler.count< handler.limit) && tbx_itr_next(fp, handler.tbx, itr, &line) >= 0)
						{
						if(handler.show(&handler,&line)<0) break;
						handler.count++;
						}
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
    HttpParamFree(handler.httParams);
    free(line.s);
    if(fp!=NULL) hts_close(fp);
    if(handler.tbx!=NULL) tbx_destroy(handler.tbx);
    return http_status;
    }
    
   
