#include <string.h>
#include <stdlib.h>
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

