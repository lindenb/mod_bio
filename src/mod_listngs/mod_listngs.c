/**
 * Author: Pierre Lindenbaum PhD
 * April 2014
 * Motivation: apache2 module for Bioinformatics
 * WWW: http://github.com/lindenb/mod_bio
 */
 #include <zlib.h>  
#include <stdio.h> 
#include <stdint.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "r_utils.h"


#define INDEX_NGS_NAME "index-ngs"

struct listngs_callback_t
 	{
 	char *basedir;
 	request_rec *r;
 	const char* jsonp_callback;
 	HttpParamPtr httParams;
 	
 	
	int (*directory)( struct listngs_callback_t*,DIR *dir);
 	};


static int is_ngs_file(const char* filename)
	{
	return ( str_ends_with(filename,".bam") ||
	     	fileExtExists(filename,".tbi") ||
	     	str_ends_with(filename,".fastq.gz") ||
	     	str_ends_with(filename,".fq.gz") ||
	     	(str_ends_with(filename,".fasta") && fileExtExists(filename,".fai")) ||
	     	(str_ends_with(filename,".fa") && fileExtExists(filename,".fai"))?
	     	TRUE : FALSE );
	}

static int jsonDirectory( struct listngs_callback_t* handler,DIR *dir)
	{
	int count=0;
	struct dirent *entry;
	
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
	ap_rputs("{\"git-version\":",handler->r);
	ap_jsonQuote(MOD_BIO_VERSION,handler->r);
	ap_rputs(",\"content\":[",handler->r);
	
	
	
	while((entry = readdir(dir))!=NULL)
		{
		char* concatpath=NULL;
		struct stat statbuf;
		//skip hidden files
		if(entry->d_name[0]=='.') continue;
		
		concatpath=(char*)malloc(
			strlen(handler->basedir)+
			strlen(entry->d_name)+2);
		if(concatpath==NULL) break;
		sprintf(concatpath,"%s/%s",handler->basedir,entry->d_name);
		
		if(lstat(concatpath,&statbuf)!=0)
			{
			free(concatpath);
			break;
			}
		
		
		
		if(S_ISDIR(statbuf.st_mode))
			{
			size_t i,n=strlen(entry->d_name);
			 
			if(count>0) ap_rputc(',',handler->r);
			ap_rputc('\"',handler->r);
			for(i=0;i< n;++i)
				{
				ap_jsonEscapeC(entry->d_name[i],handler->r);
				}
			ap_rputs("/\"",handler->r);/* append '/' at end of directory */
			++count;
			}
		else if(S_ISREG(statbuf.st_mode) && is_ngs_file(concatpath))
			{
			if(count>0) ap_rputc(',',handler->r);
			ap_jsonQuote(entry->d_name,handler->r);
			++count;
			}
	
		free(concatpath);
		concatpath=NULL;
		}
	
	
	ap_rputs("]}",handler->r);
	if(handler->jsonp_callback!=NULL)
		{
		ap_rputs(");\n",handler->r);
		}
	return 0;
	}

static int xmlDirectory( struct listngs_callback_t* handler,DIR *dir)
	{
	struct dirent *entry;
	
	ap_set_content_type(handler->r, MIME_TYPE_XML);
	ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<ngs-listing git-version=\"" MOD_BIO_VERSION "\">\n",handler->r);	
	
	while((entry = readdir(dir))!=NULL)
		{
		char* concatpath=NULL;
		struct stat statbuf;
		//skip hidden files
		if(entry->d_name[0]=='.') continue;
		
		concatpath=(char*)malloc(
			strlen(handler->basedir)+
			strlen(entry->d_name)+2);
		if(concatpath==NULL) break;
		sprintf(concatpath,"%s/%s",handler->basedir,entry->d_name);
		
		if(lstat(concatpath,&statbuf)!=0)
			{
			free(concatpath);
			break;
			}

		if(S_ISDIR(statbuf.st_mode))
			{
			ap_rputs("<directory>",handler->r);
			ap_xmlPuts(entry->d_name,handler->r);
			ap_rputs("</directory>",handler->r);
			}
		else if(S_ISREG(statbuf.st_mode) && is_ngs_file(concatpath))
			{
			ap_rputs("<file>",handler->r);
			ap_xmlPuts(entry->d_name,handler->r);
			ap_rputs("</file>",handler->r);
			}
	
		free(concatpath);
		concatpath=NULL;
		}
	
	
	ap_rputs("</ngs-listing>\n",handler->r);
	return 0;
	}


/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int list_handler(request_rec *r);

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
    	ap_hook_handler(list_handler, NULL, NULL, APR_HOOK_LAST);
	}

#define SETUP_HANDLER(prefix)\
	handler.directory=prefix ## Directory


static int list_handler(request_rec *r)
    {
	struct listngs_callback_t handler;
    int http_status=OK;
    memset((void*)&handler,0,sizeof(struct listngs_callback_t));
    handler.r=r;
    
    
   // if (!r->handler || strcmp(r->handler, "listngs-handler")) return (DECLINED);
    
    
    if (strcmp(r->method, "GET")!=0) return DECLINED;
#ifdef ENABLE_LISTNGS
    if (strcmp(ENABLE_LISTNGS, "yes")!=0) return DECLINED;
#endif
    if(r->canonical_filename==NULL)  return DECLINED;
   

    
    	/* only one loop, we use this to cleanup the code, instead of using a goto statement */
	    do	{
	    	DIR *dir;
	    	char* slash=NULL;
   			
   			handler.httParams = HttpParamParseGET(r); 
    
		    
		    if(str_ends_with(r->canonical_filename,"/" INDEX_NGS_NAME ".json"))
		    	{
		    	handler.jsonp_callback=HttpParamGet(handler.httParams,"callback");
		    	SETUP_HANDLER(json);
		    	}
		    else if(str_ends_with(r->canonical_filename,"/" INDEX_NGS_NAME ".xml"))
		    	{
		    	SETUP_HANDLER(xml);	
		    	}
		    else
		    	{
		    	http_status=DECLINED;
		    	break;
		    	}
		   			
   			
   			
	    	/* get last file separator */
			slash=strrchr(r->canonical_filename,'/');
		    if(slash==NULL)
		    	{
		    	http_status=HTTP_INTERNAL_SERVER_ERROR;
		    	break;
		    	}
	    	handler.basedir=strndup(r->canonical_filename,slash-r->canonical_filename);
	    	if(handler.basedir==NULL)
		    	{
		    	http_status=HTTP_INTERNAL_SERVER_ERROR;
		    	break;
		    	}
	    	if ((dir = opendir(handler.basedir))==NULL)
       			{
       			http_status=HTTP_INTERNAL_SERVER_ERROR;
		    	break;
       			}
       		handler.directory(&handler,dir);
			closedir(dir);
    	} while(0);/* always abort */
    
    
    //cleanup
    HttpParamFree(handler.httParams);
    free(handler.basedir);
    return http_status;
    }
    
   
