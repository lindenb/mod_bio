#include <zlib.h>  
#include <stdio.h> 
#include <stdint.h>  
#include "htslib/sam.h"
#include "r_utils.h"




struct bam_callback_t
	{
	samFile* samFile;
	bam_hdr_t *header;
	request_rec *r;
	long count;
	const char* region;
	const char* jsonp_callback;
	void (*startdocument)( struct bam_callback_t*);
	void (*enddocument)( struct bam_callback_t*);
	int (*show)( struct bam_callback_t*,const  bam1_t *b);
	};
/** PLAIN handlers ***************************************************/

static void plainStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/plain");

	}

static void plainEnd( struct bam_callback_t* handler)
	{
	}

		
static int plainShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{

	return 0;
	}

/** XML handlers ***************************************************/

static void xmlStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/xml");
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<sam-file>\n<header>",handler->r);
	ap_rputs("</header><records>",handler->r);
	}


static void xmlEnd( struct bam_callback_t* handler)
	{
	ap_rputs("</records>\n</sam-file>\n",handler->r);
	}

#define OPEN_TAG(a)  ap_rputs("<" tag ">",handler->r)
#define CLOSE_TAG(a)  ap_rputs("</" tag ">",handler->r)
#define SIMPLE_STR_TAG(tag,a) do{if((a)!=NULL) {\
     OPEN_TAG(tag);\
     ap_rputs((a),handler->r);\
     CLOSE_TAG(tag);\
    }} while(0)
#define SIMPLE_INT_TAG(tag,a) do{\
     OPEN_TAG(tag);\
     ap_rptintf(handler->r,"%d",a);\
     CLOSE_TAG(tag);\
    }} while(0)

static int xmlShow(
	    struct bam_callback_t* handler,
	     const  bam1_t *b
	    )
	{
	uint8_t *s;
	int i;
	const bam1_core_t *c = &b->core;
	OPEN_TAG("sam");

	SIMPLE_STR_TAG("name",bam_get_qname(b));

	SIMPLE_INT_TAG("flag",c->flag);

	if (c->tid >= 0)
		{ // chr
		SIMPLE_STR_TAG("chrom",handler->header->target_name[c->tid]);
		}
	SIMPLE_INT_TAG("pos",c->pos + 1);
	SIMPLE_INT_TAG("mapq",c->qual);
	if (c->n_cigar) { // cigar
		OPEN_TAG("cigar-string");
		uint32_t *cigar = bam_get_cigar(b);
		for (i = 0; i < c->n_cigar; ++i)
			{
			ap_rprintf( handler->r,"<cigar op='%c' count='%d'/>",
				bam_cigar_oplen(cigar[i])
				bam_cigar_opchr(cigar[i])
				);
			}
		CLOSE_TAG("cigar-string");
		}
	ap_rputc('\t', handler->r);
	if (c->mtid >= 0)
	    {
	    SIMPLE_STR_TAG("mate-chrom",handler->header->target_name[c->mtid]);
	    SIMPLE_INT_TAG("mate-pos",c->mpos + 1);
	    SIMPLE_INT_TAG("insert-size",c->isize + 1);
	    }

	if (c->l_qseq)
		{ // seq and qual
		OPEN_TAG("sequence");
		uint8_t *s = bam_get_seq(b);
		for (i = 0; i < c->l_qseq; ++i) ap_rputc("=ACMGRSVTWYHKDBN"[bam_seqi(s, i)], handler->r);
		CLOSE_TAG("sequence");

		s = bam_get_qual(b);
		if(!(s[0] == 0xff))
		    {
		    OPEN_TAG("qual");
		    for (i = 0; i < c->l_qseq; ++i) ap_rputc(s[i] + 33, handler->r);
		    CLOSE_TAG("qual");
		    }
		}
	OPEN_TAG("aux-list");
	s = bam_get_aux(b); // aux
	while (s+4 <= b->data + b->l_data) {
		uint8_t type, key[2];
		key[0] = s[0]; key[1] = s[1];
		s += 2; type = *s++;
		OPEN_TAG("aux");
		ap_rputc('\t', handler->r); ap_rputs((char*)key, 2, handler->r); ap_rputc(':', handler->r);
		if (type == 'A') {
			ap_rputs("A:", 2, handler->r);
			ap_rputc(*s, handler->r);
			++s;
		} else if (type == 'C') {
			ap_rputs("i:", 2, handler->r);
			kputw(*s, handler->r);
			++s;
		} else if (type == 'c') {
			ap_rputs("i:", 2, handler->r);
			kputw(*(int8_t*)s, handler->r);
			++s;
		} else if (type == 'S') {
			if (s+2 <= b->data + b->l_data) {
				ap_rputs("i:", 2, handler->r);
				kputw(*(uint16_t*)s, handler->r);
				s += 2;
			} else return -1;
		} else if (type == 's') {
			if (s+2 <= b->data + b->l_data) {
				ap_rputs("i:", 2, handler->r);
				kputw(*(int16_t*)s, handler->r);
				s += 2;
			} else return -1;
		} else if (type == 'I') {
			if (s+4 <= b->data + b->l_data) {
				ap_rputs("i:", 2, handler->r);
				kputuw(*(uint32_t*)s, handler->r);
				s += 4;
			} else return -1;
		} else if (type == 'i') {
			if (s+4 <= b->data + b->l_data) {
				ap_rputs("i:", 2, handler->r);
				kputw(*(int32_t*)s, handler->r);
				s += 4;
			} else return -1;
		} else if (type == 'f') {
			if (s+4 <= b->data + b->l_data) {
				ksprintf(handler->r, "f:%g", *(float*)s);
				s += 4;
			} else return -1;

		} else if (type == 'd') {
			if (s+8 <= b->data + b->l_data) {
				ksprintf(handler->r, "d:%g", *(double*)s);
				s += 8;
			} else return -1;
		} else if (type == 'Z' || type == 'H') {
			ap_rputc(type, handler->r); ap_rputc(':', handler->r);
			while (s < b->data + b->l_data && *s) ap_rputc(*s++, handler->r);
			if (s >= b->data + b->l_data)
				return -1;
			++s;
		} else if (type == 'B') {
			uint8_t sub_type = *(s++);
			int32_t n;
			memcpy(&n, s, 4);
			s += 4; // no point to the start of the array
			if (s + n >= b->data + b->l_data)
				return -1;
			ap_rputs("B:", 2, handler->r); ap_rputc(sub_type, handler->r); // write the typing
			for (i = 0; i < n; ++i) { // FIXME: for better performance, put the loop after "if"
				ap_rputc(',', handler->r);
				if ('c' == sub_type)	  { kputw(*(int8_t*)s, handler->r); ++s; }
				else if ('C' == sub_type) { kputw(*(uint8_t*)s, handler->r); ++s; }
				else if ('s' == sub_type) { kputw(*(int16_t*)s, handler->r); s += 2; }
				else if ('S' == sub_type) { kputw(*(uint16_t*)s, handler->r); s += 2; }
				else if ('i' == sub_type) { kputw(*(int32_t*)s, handler->r); s += 4; }
				else if ('I' == sub_type) { kputuw(*(uint32_t*)s, handler->r); s += 4; }
				else if ('f' == sub_type) { ksprintf(handler->r, "%g", *(float*)s); s += 4; }
			}

		}
	    CLOSE_TAG("aux");
	    }
	CLOSE_TAG("aux-list");

	CLOSE_TAG("sam");
	return 0;
	}

#undef SIMPLE_STR_TAG

/** json handlers ***************************************************/
static void jsonStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_JSON);
	if(handler->jsonp_callback!=NULL)
			{
			ap_rputs(handler->jsonp_callback,handler->r);
			ap_rputc('(',handler->r);
			}


	}


static void jsonEnd( struct bam_callback_t* handler)
	{

	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}


		
static int jsonShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{

	return 0;
	}

/** HTML handlers ***************************************************/
static void htmlStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_HTML);
	ap_rputs("<html>",handler->r);
	ap_rputs("<head><title>",handler->r);
	ap_xmlPuts(handler->r->uri,handler->r);
	ap_rputs("</title><style>",handler->r);
	ap_rputs(css_stylesheet,handler->r);
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
		"<label for='limit'>Limit:</label> <input id='limit' name='limit' placeholder='max records' type=\"number\" value=\"" #DEFAULT_LIMIT_RECORDS "\"/>"
		"<label for='region'>Region:</label> <input id='region' name='region' placeholder='chrom:start-end' "
		,handler->r);
	if(handler->region!=NULL)
	    {
	    ap_rputs(" value=\"",handler->r);
	    ap_xmlPuts(handler->region,handler->r);
	    ap_rputs("\"",handler->r);
	    }
	ap_rputs("/>"
		"<input type='submit'/></form><div>"
		,handler->r);
	if(handler->region!=NULL)
	    {
	    ap_rputs("<h3>",handler->r);
	    ap_xmlPuts(handler->region,handler->r);
	    ap_rputs("</h3>",handler->r);
	    }
	ap_rputs("<div class='bam'>"
		,handler->r);
	}


static void htmlEnd( struct bam_callback_t* handler)
	{

	ap_rputs("</div>",handler->r);
	ap_rputs(html_address,handler->r);
	ap_rputs("</body></html>\n",handler->r);
	}

		
static int htmlShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{

	return 0;
	}


/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int bam_handler(request_rec *r);

/* Define our module as an entity and assign a function for registering hooks  */

module AP_MODULE_DECLARE_DATA   bam_module =
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
    	ap_hook_handler(bam_handler, NULL, NULL, APR_HOOK_LAST);
	}


static int bam_handler(request_rec *r)
    {
    struct bam_callback_t handler;
    HttpParamPtr httParams=NULL;
    ChromStartEnd chromStartEnd;
    int http_status=OK;
    hts_itr_t *iter=NULL;
    hts_idx_t *idx=NULL;
    bam1_t *b=NULL;
    long limit=DEFAULT_LIMIT_RECORDS;
    memset((void*)&handler,0,sizeof(struct bam_callback_t));
    memset((void*)&chromStartEnd,0,sizeof(ChromStartEnd));


    if (!r->handler || strcmp(r->handler, "bam-handler")) return (DECLINED);
    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
    if( !(
    	str_ends_with(r->canonical_filename,".bam")
       	))  return DECLINED;
    /* check file exists */
    if((http_status=fileExists(r->canonical_filename))!=OK)
	{
	return http_status;
	}

    httParams = HttpParamParseGET(r); 
    if(httParams==NULL) return DECLINED;
    handler.r=r;
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{

	const char* format=HttpParamGet(httParams,"format");
    	handler.region=HttpParamGet(httParams,"region");

    	b=bam_init1();
    	if(b==NULL)
    	    {
    	    http_status=HTTP_INTERNAL_SERVER_ERROR;
    	    break;
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
    	

    	handler.samFile = sam_open(r->canonical_filename,"r");
    	if(handler.samFile==NULL)
    		{
    		http_status=HTTP_NOT_FOUND;
    		break;
    		}

    	handler.header= sam_hdr_read(handler.samFile);
    	if(handler.header==NULL)
		{
		http_status=HTTP_INTERNAL_SERVER_ERROR;
		break;
		}


    	if(handler.region!=NULL)
    	    {
    	    idx = bam_index_load(r->canonical_filename);
    	    if(idx==NULL)
    		{
    		http_status=HTTP_INTERNAL_SERVER_ERROR;
    		break;
    		}
    	    iter = bam_itr_querys(idx, handler.header, handler.region);
    	    if(iter==NULL)
    		{
    		http_status=HTTP_BAD_REQUEST;
    		break;
    		}
    	    }


    	 handler.startdocument(&handler);
    	 while(limit==-1  || handler.count<limit)
    	     {
    	     int r;
	     if(iter==NULL)
		 {
		 r = sam_read1(handler.samFile, handler.header, b);
		 }
	     else
		 {
		 r = bam_itr_next(handler.samFile, iter, b);
		 }
	     if(r<0) break;
	     if(handler.show(&handler,b)!=0) break;
	     handler.count++;
    	     }

    	 handler.enddocument(&handler);

    	} while(0);/* always abort */
    
    
    //cleanup
    HttpParamFree(httParams);
    if(b!=NULL) bam_destroy1(b);
    if(iter!=NULL) hts_itr_destroy(iter);
    if(idx!=NULL) hts_idx_destroy(idx);
    if(handler.header!=NULL)  bam_hdr_destroy(handler.header);
    if(handler.samFile!=NULL) sam_close(handler.samFile);
    return http_status;
    }
    
   
