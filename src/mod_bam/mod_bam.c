/**
 * Author: Pierre Lindenbaum PhD
 * April 2014
 * Motivation: apache2 module for Bioinformatics
 * WWW: http://github.com/lindenb/mod_bio
 */
#include <zlib.h>  
#include <stdio.h> 
#include <stdint.h> 
#include <ctype.h>
#include "htslib/sam.h"
#include "htslib/kstring.h"
#include "r_utils.h"

/** call back to print AUX data in BAM record */
struct aux_callback_t
	{
	request_rec *r;
	int (*print)( struct aux_callback_t*,const uint8_t* key,char type,const kstring_t* str, int index);
	};

struct bam_callback_t
	{
	HttpParamPtr httParams;
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
	ap_rputs(handler->header->text,handler->r);	
	}

static void plainEnd( struct bam_callback_t* handler)
	{
	}

		
static int plainShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{
	int ret=0;
	kstring_t ks = { 0, 0, NULL };
	if( sam_format1(handler->header, b,&ks)<0) return -1;
	ret=ap_rputs(ks.s,handler->r);// write the alignment to `r'
	ap_rputc('\n',handler->r);
	free(ks.s);
	return ret;
	}
	
#define NEWKS kstring_t str = { 0, 0, NULL }
#define FREEKS(n) free(str.s);s+=n
#define ECHO_AUX(type) callback->print(callback,key,type,&str,num_count++)
	
/* generic loop to print AUX data*/
int print_aux_data(struct aux_callback_t* callback, const  bam1_t *b)
	{
	int num_count=0;
	uint8_t *s;
	s = bam_get_aux(b); // aux
	while (s+4 <= b->data + b->l_data)
		{
		uint8_t type, key[2];
		key[0] = s[0]; key[1] = s[1];
		s += 2; type = *s++;
		
		if (type == 'A') {
			NEWKS;
			kputc(*s, &str);
			ECHO_AUX('A');
			FREEKS(1);
		} else if (type == 'C')
			{
			NEWKS;
			kputw(*s, &str);
			ECHO_AUX('i');
			FREEKS(1);			
			++s;
		} else if (type == 'c')
			{
			NEWKS;
			kputw(*(int8_t*)s, &str);
			ECHO_AUX('i');
			FREEKS(1);
			} 
		else if (type == 'S')
			{
			if (s+2 <= b->data + b->l_data) {
				NEWKS;
				kputw(*(uint16_t*)s, &str);
				ECHO_AUX('i');
				FREEKS(2);
			} else return -1;
		} else if (type == 's') {
			if (s+2 <= b->data + b->l_data) {
				NEWKS;
				kputw(*(int16_t*)s, &str);
				ECHO_AUX('i');
				FREEKS(2);
			} else return -1;
		} else if (type == 'I') {
			if (s+4 <= b->data + b->l_data) {
				NEWKS;
				kputw(*(uint32_t*)s, &str);
				ECHO_AUX('i');
				FREEKS(4);
			} else return -1;
		} else if (type == 'i') {
			if (s+4 <= b->data + b->l_data) {
				NEWKS;
				kputw(*(int32_t*)s, &str);
				ECHO_AUX('i');
				FREEKS(4);
			} else return -1;
		} else if (type == 'f'){
			if (s+4 <= b->data + b->l_data) {
				NEWKS;
				ksprintf(&str, "%g", *(float*)s);
				ECHO_AUX('f');
				FREEKS(4);
			} else return -1;
			
		} else if (type == 'd') {
			if (s+8 <= b->data + b->l_data)
				{
				NEWKS;
				ksprintf(&str, "%g", *(double*)s);
				ECHO_AUX('d');
				FREEKS(8);
			} else return -1;
		} else if (type == 'Z' || type == 'H')
			{
			NEWKS;
			while (s < b->data + b->l_data && *s) kputc(*s++, &str);
			ECHO_AUX((char)type);
			FREEKS(8);
			if (s >= b->data + b->l_data)
				{
				return -1;
				}
			++s;
		} else if (type == 'B') {
			uint8_t sub_type = *(s++);
			int32_t n;
			int i;
			memcpy(&n, s, 4);
			s += 4; // no point to the start of the array
			if (s + n >= b->data + b->l_data)
				return -1;
			
			for (i = 0; i < n; ++i) { // FIXME: for better performance, put the loop after "if"
				NEWKS;
				if ('c' == sub_type)	  { kputw(*(int8_t*)s, &str); ECHO_AUX(sub_type);FREEKS(1); }
				else if ('C' == sub_type) { kputw(*(uint8_t*)s, &str);ECHO_AUX(sub_type);FREEKS(1); }
				else if ('s' == sub_type) { kputw(*(int16_t*)s, &str); ECHO_AUX(sub_type);FREEKS(2); }
				else if ('S' == sub_type) { kputw(*(uint16_t*)s, &str); ECHO_AUX(sub_type);FREEKS(2); }
				else if ('i' == sub_type) { kputw(*(int32_t*)s, &str); ECHO_AUX(sub_type);FREEKS(4); }
				else if ('I' == sub_type) { kputuw(*(uint32_t*)s, &str);ECHO_AUX(sub_type);FREEKS(4); }
				else if ('f' == sub_type) { ksprintf(&str, "%g", *(float*)s); ECHO_AUX(sub_type);FREEKS(4); }
				else {free(str.s);}
				
			}
		}
	}
	return 0;
	}


/** XML handlers ***************************************************/


static void xmlStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, "text/xml");
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<sam-file git-version=\"" MOD_BIO_VERSION "\">\n<header>",handler->r);
	ap_xmlPuts(handler->header->text,handler->r);
	ap_rputs("</header><records>",handler->r);
	}


static void xmlEnd( struct bam_callback_t* handler)
	{
	ap_rputs("</records>\n</sam-file>\n",handler->r);
	}

#define ap_kputw(i,r)  ap_rprintf(r,"%d",i)
#define ap_kputuw(i,r)  ap_rprintf(r,"%u",i)
#define OPEN_TAG(tag)  ap_rputs("<" tag ">",handler->r)
#define CLOSE_TAG(tag)  ap_rputs("</" tag ">",handler->r)


#define SIMPLE_STR_TAG(tag,a) do{if((a)!=NULL) {\
     OPEN_TAG(tag);\
     ap_xmlPuts((a),handler->r);\
     CLOSE_TAG(tag);\
    }} while(0)
#define SIMPLE_INT_TAG(tag,a) do{\
     OPEN_TAG(tag);\
     ap_kputw(a,handler->r);\
     CLOSE_TAG(tag);\
    } while(0)


static int xmlPrintAux(
	struct aux_callback_t* handler,
	const uint8_t* key,
	char type,
	const kstring_t* str,
	int index)
	{
	ap_rputs("<aux name=\"",handler->r);
	ap_rwrite((void*)key,2,handler->r);
	ap_rprintf(handler->r,"\" type=\"%c\">",type);
	ap_xmlPuts(str->s,handler->r);\
	return ap_rputs("</aux>",handler->r);
	}

static int xmlShow(
	    struct bam_callback_t* handler,
	     const  bam1_t *b
	    )
	{
	struct aux_callback_t auxh;
	//uint8_t *s;
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
				bam_cigar_opchr(cigar[i]),
				bam_cigar_oplen(cigar[i])
				
				);
			}
		CLOSE_TAG("cigar-string");
		}
	if (c->mtid >= 0)
	    {
	    SIMPLE_STR_TAG("mate_chrom",handler->header->target_name[c->mtid]);
	    SIMPLE_INT_TAG("mate_pos",c->mpos + 1);
	    SIMPLE_INT_TAG("insert_size",c->isize + 1);
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
	auxh.r = handler->r;
	auxh.print = xmlPrintAux;
	print_aux_data(&auxh,b);
	CLOSE_TAG("aux-list");

	return CLOSE_TAG("sam");
	}
#undef CLOSE_TAG
#undef OPEN_TAG
#undef SIMPLE_STR_TAG
#undef SIMPLE_INT_TAG
#undef ap_kputw
#undef ap_kputuw

/** json handlers ***************************************************/
static void jsonStart( struct bam_callback_t* handler)
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
	ap_rputs("{\"header\":",handler->r);
	ap_jsonQuote(handler->header->text,handler->r);
	ap_rputs(",\"records\":[",handler->r);
	}


static void jsonEnd( struct bam_callback_t* handler)
	{
	ap_rputs("]}",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	}

#define ap_kputw(i,r)  ap_rprintf(r,"%d",i)
#define ap_kputuw(i,r)  ap_rprintf(r,"%u",i)
#define OPEN_TAG(s) ap_rputs(",\"" s "\":",handler->r);
#define SIMPLE_INT_TAG(tag,i) ap_rprintf(handler->r,",\"" tag "\":%d",i)
#define SIMPLE_STR_TAG(tag,s) do { if(s!=NULL) {OPEN_TAG(tag);ap_jsonQuote(s,handler->r);}} while(0)
		
		

static int jsonPrintAux(
	struct aux_callback_t* handler,
	const uint8_t* key,
	char type,
	const kstring_t* str,
	int index)
	{
	if(index>0) ap_rputc(',',handler->r);
	ap_rputs("{\"name\":\"",handler->r);
	ap_rwrite((void*)key,2,handler->r);
	ap_rputs("\",\"type\":\"",handler->r);
	ap_rprintf( handler->r,"%c:",type);
	ap_rputs("\",\"value\":",handler->r);
	switch(type)
		{
		case 'i':case 'I':case 'f':ap_rputs(str->s,handler->r);break;
		default:ap_jsonQuote(str->s,handler->r);break;
		}
	return ap_rputc('}',handler->r);;		
	}
static int jsonShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{
	int i=0;
	struct aux_callback_t auxh;
	//uint8_t *s;
	const bam1_core_t *c = &b->core;
	
	if(handler->count>0) ap_rputc(',',handler->r);
	ap_rputc('\n',handler->r);
	
	

	
	ap_rputs("{\"name\":",handler->r);
	ap_jsonQuote(bam_get_qname(b),handler->r);

	SIMPLE_INT_TAG("flag",c->flag);

	if (c->tid >= 0)
		{ // chr
		SIMPLE_STR_TAG("chrom",handler->header->target_name[c->tid]);
		}
	SIMPLE_INT_TAG("pos",c->pos + 1);
	SIMPLE_INT_TAG("mapq",c->qual);
	if (c->n_cigar) { // cigar
		
		OPEN_TAG("cigar");		
		ap_rputc('[',handler->r);
		uint32_t *cigar = bam_get_cigar(b);
		for (i = 0; i < c->n_cigar; ++i)
			{
			if(i>0) ap_rputc(',',handler->r);
			ap_rprintf( handler->r,"{\"op\":\"%c\",\"count\":%d}",
				bam_cigar_opchr(cigar[i]),
				bam_cigar_oplen(cigar[i])
				);
			}
		ap_rputc(']',handler->r);
		}
	if (c->mtid >= 0)
	    {
	    SIMPLE_STR_TAG("mate-chrom",handler->header->target_name[c->mtid]);
	    SIMPLE_INT_TAG("mate-pos",c->mpos + 1);
	    SIMPLE_INT_TAG("insert-size",c->isize + 1);
	    }

	if (c->l_qseq)
		{ // seq and qual
		OPEN_TAG("sequence");
		ap_rputc('\"',handler->r);
		uint8_t *s = bam_get_seq(b);
		for (i = 0; i < c->l_qseq; ++i) ap_rputc("=ACMGRSVTWYHKDBN"[bam_seqi(s, i)], handler->r);
		ap_rputc('\"',handler->r);

		s = bam_get_qual(b);
		if(!(s[0] == 0xff))
		    {
		   ap_rputs(",\"qual\":\"",handler->r);
		    for (i = 0; i < c->l_qseq; ++i) ap_rputc(s[i] + 33, handler->r);
			ap_rputc('\"',handler->r);
		    }
		}
	ap_rputs(",\"aux\":[",handler->r);
	auxh.r = handler->r;
	auxh.print = jsonPrintAux;
	print_aux_data(&auxh,b);
	ap_rputc(']',handler->r);//end aux-list

	return ap_rputc('}',handler->r);

	}
#undef CLOSE_TAG
#undef OPEN_TAG
#undef SIMPLE_STR_TAG
#undef SIMPLE_INT_TAG
#undef ap_kputw
#undef ap_kputuw


/** HTML handlers ***************************************************/
static void htmlStart( struct bam_callback_t* handler)
	{
	ap_set_content_type(handler->r, MIME_TYPE_HTML);
	ap_rputs("<!doctype html>\n<html lang=\"en\">",handler->r);
	ap_rputs("<head>",handler->r);
	printDefaulthtmlHead(handler->r);
	ap_rputs("</head>",handler->r);
	ap_rputs("<body>",handler->r);
	ap_rprintf(handler->r,
		"<form>"
		"<label for='format'>Format:</label> <select  id='format' name='format'>"
		"<option value='html'>html</option>"
		"<option value='json'>json</option>"
		"<option value='xml'>xml</option>"
		"<option value='text'>text</option>"
		"</select> "
		"<label for='limit'>Limit:</label> <input id='limit' name='limit' placeholder='max records' type=\"number\" value=\"%d\"/>"
		"<label for='region'>Region:</label> <input id='region' name='region' placeholder='chrom:start-end' ",
		DEFAULT_LIMIT_RECORDS
		);
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
	ap_rputs("<div class='fileheader'>"
		,handler->r);
	ap_xmlPuts(handler->header->text,handler->r);
	ap_rputs("</div><div><table>"
		"<thead><tr>"
		"<th>Name</th>"
		"<th>Flag</th>"
		"<th>Chrom</th>"
		"<th>Pos</th>"
		"<th>mapQ</th>"
		"<th>Cigar</th>"
		"<th>Mate Chrom</th>"
		"<th>Mate Pos</th>"
		"<th>TLen</th>"
		"<th>SEQ</th>"
		"<th>Qual</th>"
		"<th>Aux</th>"
		"</tr></thead><tbody>"
		,handler->r);
	}


static void htmlEnd( struct bam_callback_t* handler)
	{

	ap_rputs("<tbody></table></div>",handler->r);
	ap_rputs(html_address,handler->r);
	ap_rputs("</body></html>\n",handler->r);
	}

#define ap_kputw(i,r)  ap_rprintf(r,"%d",i)
#define ap_kputuw(i,r)  ap_rprintf(r,"%u",i)
#define OPEN_TAG(tag)  ap_rputs("<" tag ">",handler->r)
#define CLOSE_TAG(tag)  ap_rputs("</" tag ">",handler->r)


#define SIMPLE_STR_TAG(a) do{if((a)!=NULL) {\
     OPEN_TAG("td");\
     ap_xmlPuts((a),handler->r);\
     CLOSE_TAG("td");\
    }} while(0)
#define SIMPLE_INT_TAG(a) do{\
     OPEN_TAG("td");\
     ap_kputw(a,handler->r);\
     CLOSE_TAG("td");\
    } while(0)


static int htmlPrintAux(
	struct aux_callback_t* handler,
	const uint8_t* key,
	char type,
	const kstring_t* str,
	int index)
	{
	ap_rputs("<span>",handler->r);
	ap_rwrite((void*)key,2,handler->r);
	ap_rprintf( handler->r,":%c:",type);
	ap_xmlPuts(str->s,handler->r);\
	return ap_rputs("</span> ",handler->r);
	}


static int htmlShow(
	    struct bam_callback_t* handler,
	    const  bam1_t *b
	    )
	{
	struct aux_callback_t auxh;
	//uint8_t *s;
	int i;
	const bam1_core_t *c = &b->core;
	ap_rprintf( handler->r,"<tr class=\"row%d\">",
		(int)(handler->count%2)
		);

	SIMPLE_STR_TAG(bam_get_qname(b));

	SIMPLE_INT_TAG(c->flag);

	if (c->tid >= 0)
		{ // chr
		SIMPLE_STR_TAG(handler->header->target_name[c->tid]);
		}
	else
		{
		OPEN_TAG("td");CLOSE_TAG("td");
		}
	SIMPLE_INT_TAG(c->pos + 1);
	SIMPLE_INT_TAG(c->qual);
	
	OPEN_TAG("td");
	if (c->n_cigar) { // cigar
		
		uint32_t *cigar = bam_get_cigar(b);
		for (i = 0; i < c->n_cigar; ++i)
			{
			ap_rprintf( handler->r,
				"<span class=\"c%c\">%c%d</span>",
				tolower(bam_cigar_opchr(cigar[i])=='='?'m':bam_cigar_opchr(cigar[i])),
				bam_cigar_opchr(cigar[i]),
				bam_cigar_oplen(cigar[i])
				);
			}
		
		}
	if (c->mtid >= 0)
	    {
	    SIMPLE_STR_TAG(handler->header->target_name[c->mtid]);
	    SIMPLE_INT_TAG(c->mpos + 1);
	    SIMPLE_INT_TAG(c->isize + 1);
	    }
	   else
	  	{
	  	OPEN_TAG("td");CLOSE_TAG("td");
	  	OPEN_TAG("td");CLOSE_TAG("td");
	  	OPEN_TAG("td");CLOSE_TAG("td");
	    }
	
	if (c->l_qseq)
		{ // seq and qual
		OPEN_TAG("td");   
		uint8_t *s = bam_get_seq(b);
		for (i = 0; i < c->l_qseq; ++i)
			{
			char base="=ACMGRSVTWYHKDBN"[bam_seqi(s, i)];
			ap_rprintf(handler->r,"<span class=\"b%c\">%c</span>",tolower(base),base);
			}
		CLOSE_TAG("td");
		s = bam_get_qual(b);
		if(!(s[0] == 0xff))
		    {
		    OPEN_TAG("td");
		    for (i = 0; i < c->l_qseq; ++i) ap_rputc(s[i] + 33, handler->r);
		    CLOSE_TAG("td");
		    }
		else
			{
			OPEN_TAG("td");CLOSE_TAG("td");
			}
		}
	else
		{
	  	OPEN_TAG("td");CLOSE_TAG("td");
	  	OPEN_TAG("td");CLOSE_TAG("td");
		}
		
	OPEN_TAG("td");
	auxh.r = handler->r;
	auxh.print = htmlPrintAux;
	print_aux_data(&auxh,b);
	CLOSE_TAG("td");
	
	CLOSE_TAG("tr");
	return 0;
	}
#undef CLOSE_TAG
#undef OPEN_TAG
#undef SIMPLE_STR_TAG
#undef SIMPLE_INT_TAG
#undef ap_kputw
#undef ap_kputuw


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

#define SETUP_HANDLER(prefix)\
	handler.startdocument= prefix ## Start;\
	handler.enddocument= prefix ## End;\
	handler.show=prefix ## Show

/**
 * Workhorse  method for BAM
 *
 */
static int bam_handler(request_rec *r)
    {
    struct bam_callback_t handler;
    
   
    int http_status=OK;
    hts_itr_t *iter=NULL;
    hts_idx_t *idx=NULL;
    bam1_t *b=NULL;
    long limit=DEFAULT_LIMIT_RECORDS;
    memset((void*)&handler,0,sizeof(struct bam_callback_t));


    if (!r->handler || strcmp(r->handler, "bam-handler")) return (DECLINED);
    if (strcmp(r->method, "GET")!=0) return DECLINED;
    if(r->canonical_filename==NULL)  return DECLINED;
    if(!str_ends_with(r->canonical_filename,".bam"))  return DECLINED;
    /* check file exists */
    if(!fileExists(r->canonical_filename))
		{
		return HTTP_NOT_FOUND;
		}

	handler.r=r;
    handler.httParams = HttpParamParseGET(r); 
    if( handler.httParams==NULL) return DECLINED;
    
    
    /* only one loop, we use this to cleanup the code, instead of using a goto statement */
    do	{

	const char* format=HttpParamGet(handler.httParams,"format");
	const char* limit_str=HttpParamGet(handler.httParams,"limit");
    handler.region=HttpParamGet(handler.httParams,"region");
    int iterator_was_requested=FALSE;

    	b=bam_init1();
    	if(b==NULL)
    	    {
    	    http_status=HTTP_INTERNAL_SERVER_ERROR;
    	    break;
    	    }
    	if(limit_str!=NULL)
    		{
    		limit=atol(limit_str);
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


    	if(handler.region!=NULL && !str_is_empty(handler.region))
    	    {
    	    iterator_was_requested=TRUE;
    	    idx = bam_index_load(r->canonical_filename);
    	    if(idx!=NULL)
	    		{
	    		iter = bam_itr_querys(idx, handler.header, handler.region);
	    		}
    	   
    	    }


    	 handler.startdocument(&handler);
    	 while(limit==-1  || handler.count<limit)
    	     {
    	     int r;
		     if(!iterator_was_requested)
				 {
				 r = sam_read1(handler.samFile, handler.header, b);
				 }
		     else if(iter!=NULL)
				 {
				 r = bam_itr_next(handler.samFile, iter, b);
				 }
			 else
			 	{
			 	r=-1;
			 	}
		     if(r<0) break;
		     if(handler.show(&handler,b)<0) break;
		     handler.count++;
    	     }

    	 handler.enddocument(&handler);

    	} while(0);/* always abort */
    
    
    //cleanup
    HttpParamFree(handler.httParams);
    if(b!=NULL) bam_destroy1(b);
    if(iter!=NULL) hts_itr_destroy(iter);
    if(idx!=NULL) hts_idx_destroy(idx);
    if(handler.header!=NULL)  bam_hdr_destroy(handler.header);
    if(handler.samFile!=NULL) sam_close(handler.samFile);
    return http_status;
    }
    
   
