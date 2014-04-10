#include <zlib.h>  
#include <stdio.h>  
#include "kseq.h"  





	// STEP 1: declare the type of file handler and the read() function  
	KSEQ_INIT(gzFile, gzread)  

	int main(int argc, char *argv[])  
	{  
	gzFile fp;  
	kseq_t *seq;  
	int l;  
	if (argc == 1) {  
	    fprintf(stderr, "Usage: %s <in.seq>\n", argv[0]);  
	    return 1;  
		}  
	fp = gzopen(argv[1], "r"); // STEP 2: open the file handler  
	seq = kseq_init(fp); // STEP 3: initialize seq  
	while ((l = kseq_read(seq)) >= 0) { // STEP 4: read sequence  
	    printf("name: %s\n", seq->name.s);  
	    if (seq->comment.l) printf("comment: %s\n", seq->comment.s);  
	    printf("seq: %s\n", seq->seq.s);  
	    if (seq->qual.l) printf("qual: %s\n", seq->qual.s);  
	}  
	printf("return value: %d\n", l);  
	kseq_destroy(seq); // STEP 5: destroy seq  
	gzclose(fp); // STEP 6: close the file handler  
	return 0;  
	}  
