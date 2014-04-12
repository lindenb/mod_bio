#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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


int ap_xmlPuts(const char* s,request_rec *r)
	{
	const char*p=s;
	if(r==NULL || s==NULL) return -1;
	while(*p!=0)
		{
		if(ap_xmlPutc(*p,r)==-1) return -1;
		++p;
		}
	return (int)(p-s);
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
	const char*p=s;
	if(s==NULL)
		{
		return ap_rputs("null",r);
		}
	ap_rputc('\"',r);
	while(*p!=0)
		{
		switch(*p)
			{
			case '\\': ap_rputs("\\\\",r); break;
			case '\t': ap_rputs("\\t",r); break;
			case '\n': ap_rputs("\\n",r); break;
			case '\"': ap_rputs("\"",r); break;
			default: ap_rputc(*p,r); break;
			}
		++p;
		}
	return ap_rputc('\"',r);
	}

int fileExists(const char* filename)
    {
    struct stat buf;
    /*  lstat() is identical to stat(), except that if path is a symbolic link,
       then the link itself is stat-ed, not the file that it refers to. */
    if(lstat(filename,&buf)!=0) return HTTP_NOT_FOUND;
    if(!S_ISREG(buf.st_mode)) return HTTP_BAD_REQUEST;
    //if(!S_IRUSR(buf.st_mode)) return HTTP_FORBIDDEN;
    return OK;
    }

int fileExtExists(const char* filename,const char* suffix)
    {
    int ret=0;
    size_t len1=strlen(filename);
    size_t len2=strlen(suffix);
    char* p=(char*)malloc(len1+len2+1);
    if(p==NULL) return HTTP_INTERNAL_SERVER_ERROR;
    memcpy((void*)p,(void*)filename,len1);
    memcpy((void*)&p[len1],(void*)suffix,len2);
    p[len1+len2]=0;
    ret=fileExists(filename);
    free(p);
    return ret;
    }


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
const char* html_address="<div>Pierre Lindenbaum <a href=\"https://github.com/lindenb/mod_bio\">https://github.com/lindenb/mod_bio</a> ."
	    "<span>Git-Version:" MOD_BIO_VERSION "</span></div>"
	    ;

const char* css_stylesheet=
	".fastqs{white-space:pre;font-family:monospace;}\n"
	".faidx{white-space:pre;font-family:monospace;}\n"
	".ba{color:red;}\n"
	".bt{color:green;}\n"
	".bg{color:yellow;}\n"
	".bc{color:blue;}\n"
	".bn{color:black;}\n"
	".seqname{color:black;}\n"
	".seqqual{color:gray;}\n"
	;
