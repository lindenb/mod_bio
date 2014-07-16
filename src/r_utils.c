#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "htslib/../version.h"
#include "r_utils.h"


int str_ends_with(const char* s,const char* suffix)
	{
	int len1;
	int len2;
	if(s==NULL || suffix==NULL) return 0;
	len1=strlen(s);
	len2=strlen(suffix);
	return len1 >= len2 && strcmp(&s[len1-len2], suffix)==0 ?1:0;
	}

int str_is_empty(const char* s)
	{
	if(s==NULL) return 1;
	while(*s!=0)
		{
		if(!isspace(*s)) return 0;
		++s;
		}
	return 1;
	}

static void unescape_space(char* s)
	{
	size_t x=0;
	if(s==NULL) return;
	for(x=0;s[x]!=0;++x)
		{
		if(s[x] == '+') s[x] = ' ';
		}
	}


HttpParamPtr HttpParamParseQuery(const char* query,size_t query_len)
    {
    size_t i=0UL;
    HttpParamPtr root=NULL;
    if(query==NULL || query_len==0) return NULL;
    while(i< query_len)
        {
        size_t eq=i;
        size_t amp=i;
        while(amp < query_len && query[amp]!='&')
            {
            if(eq==i && query[amp]=='=') eq=amp;
            amp++;
            }
        if(amp>i)
            {
            HttpParamPtr curr=(HttpParamPtr)malloc(sizeof(HttpParam));
            if(curr==NULL) break;
            memset((void*)curr,0,sizeof(HttpParam));
            curr->next=NULL;
            if(i<eq && eq<amp)
                {
                curr->key=strndup(&query[i],eq-i);
                curr->value=strndup(&query[eq+1],amp-(eq+1));
                }
            else
                {
                curr->key=strndup(&query[i],amp-i);
                curr->value=strdup("");
                }
            ap_unescape_url(curr->key);
            unescape_space(curr->value);
            ap_unescape_url(curr->value);
            
            curr->next=root;
            root=curr;
                
            }
           
        i=amp+1;
        }
   
    return root;
    }

const char* HttpParamGet(const HttpParamPtr root,const char* key)
	{
	HttpParamPtr p=(HttpParamPtr)root;
	if(p==NULL || key==NULL) return NULL;
	while(p!=NULL)
		{
		if(strcmp(p->key,key)==0) return p->value;
		p=p->next;
		}
	return NULL;
	}

HttpParamPtr HttpParamParseGET(request_rec *r)
	{
	if(strcmp(r->method, "GET")!=0) return NULL;
	if(r==NULL || r->args==NULL) return NULL;
	return HttpParamParseQuery(r->args,strlen(r->args));
	}
	
HttpParamPtr HttpParamFree(HttpParamPtr p)
	{
	if(p==NULL) return NULL;
	HttpParamFree(p->next);
	free(p->key);
	free(p->value);
	free(p);
	return NULL;
	}


int printDefaulthtmlHead(request_rec *r)
	{
	ap_rputs("<meta charset=\"utf-8\"/>",r);
	ap_rputs("<meta name=\"description\" content=\"mod_bio\"/>\n",r);
	ap_rputs("<meta name=\"git-version\" content=\"" MOD_BIO_VERSION "\">"
	    	"<meta name=\"hts-version\" content=\"" HTS_VERSION "\">",r);
	ap_rputs("<title>",r);
	ap_xmlPuts(r->uri,r);
	ap_rputs("</title>",r);	
	return ap_rputs("<link rel=\"stylesheet\" href=\"/mod_bio/css/style.css\"></link>",r);
	}

int ap_xmlPuts(const char* s,request_rec *r)
	{
	return ap_xmlNPuts(s,(s==NULL?0UL:strlen(s)),r);
	}

int ap_xmlNPuts(const char* s,size_t n,request_rec *r)
	{
	size_t i=0;
	if(r==NULL || s==NULL) return -1;
	for(i=0;i< n;++i)
		{
		if(ap_xmlPutc(s[i],r)==-1) return -1;
		}
	return (int)n;
	}


int ap_xmlPutc(char c,request_rec *r)
	{
	switch(c)
		{
		case '>': return ap_rputs("&gt;",r);
		case '<': return ap_rputs("&lt;",r);
		case '&': return ap_rputs("&amp;",r);
		case '\"': return ap_rputs("&quot;",r);
		case '\'': return ap_rputs("&apos;",r);
		default: return ap_rputc(c,r);
		}
	}

int ap_jsonQuote(const char* s,request_rec *r)
	{
	return ap_jsonNQuote(s,(s==NULL?0UL:strlen(s)),r);
	}
	
int ap_jsonNQuote(const char* s,size_t n,request_rec *r)
	{
	size_t i=0;
	if(s==NULL)
		{
		return ap_rputs("null",r);
		}
		
	ap_rputc('\"',r);
	for(i=0;i< n;++i)
		{
		ap_jsonEscapeC(s[i],r);
		}
	ap_rputc('\"',r);
	return (int)n;
	}

int ap_jsonEscapeC(char c,request_rec *r)
	{
	switch(c)
			{
			case '\\': return ap_rputs("\\\\",r); break;
			case '\t': return ap_rputs("\\t",r); break;
			case '\n': return ap_rputs("\\n",r); break;
			case '\"': return ap_rputs("\\\"",r); break;
			default: return ap_rputc(c,r); break;
			}
	return -1;//should never happen
	}

int fileExists(const char* filename)
    {
    struct stat buf;
    /*  lstat() is identical to stat(), except that if path is a symbolic link,
       then the link itself is stat-ed, not the file that it refers to. */
    if(lstat(filename,&buf)!=0) return FALSE;
    if(!S_ISREG(buf.st_mode)) return FALSE;
    //if(!S_IRUSR(buf.st_mode)) return FALSE;
    return TRUE;
    }

int fileExtExists(const char* filename,const char* suffix)
    {
    int ret=0;
    size_t len1=strlen(filename);
    size_t len2=strlen(suffix);
    char* p=(char*)malloc(len1+len2+1);
    if(p==NULL) return FALSE;
    memcpy((void*)p,(void*)filename,len1);
    memcpy((void*)&p[len1],(void*)suffix,len2);
    p[len1+len2]=0;
    ret=fileExists(p);
    free(p);
    return ret;
    }

int baseNameExists(const char* filename)
	{
	int ret=0;
	char* p=NULL;
	char* dot=NULL;
	if(filename==NULL) return FALSE;
	dot=strrchr(filename,'.');
	if(dot==NULL) return FALSE;
	p=strndup(filename,dot-filename);
	if(p==NULL) return FALSE;
	ret=fileExists(p);
	free(p);
	return ret;
	}

/** parses a genomic region chrom:start-end 
 * return -1 on error
 */ 
int parseRegion(const char* region,ChromStartEnd* pos)
    {
    char* colon;
    char* hyphen;
    if(region==NULL || pos==NULL) return -1;
    colon=strchr(region,':');
    hyphen=NULL;
    if(colon==NULL || colon==region)
	{
	return -1;
	}
    pos->chromosome=strndup(region,colon-region);

    hyphen=strchr(&colon[1],'-');
    if(hyphen==NULL)
		{
		free( pos->chromosome);
		pos->chromosome=NULL;
		return -1;
		}
    pos->p_beg_i0=atoi(&colon[1]);
    if(&hyphen[1]==0 || pos->p_beg_i0<0)
	{
	free( pos->chromosome);
	pos->chromosome=NULL;
	return -1;
	}
    pos->p_end_i0=atoi(&hyphen[1]);
    if(pos->p_end_i0< pos->p_beg_i0 || pos->p_end_i0<0)
	{
	free( pos->chromosome);
	pos->chromosome=NULL;
	return -1;
	}

    return 0;
    }

/* html fragments */
const char* html_address="<div class=\"aboutme\">Pierre Lindenbaum <a href=\"https://github.com/lindenb/mod_bio\">https://github.com/lindenb/mod_bio</a> ."
	    "<span class=\"version\">Git-Version:" MOD_BIO_VERSION "</span> "
	    "<span class=\"version\">Hts-Version:" HTS_VERSION "</span> "
	    "</div>"
	    ;
