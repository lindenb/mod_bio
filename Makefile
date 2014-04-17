
include make.properties
CC=gcc
HTSDIR=htslib
LIBDIR?=/usr/local/lib
APXS?=apxs2
LIBTOOL?=/usr/share/apr-1.0/build/libtool
CFLAGS= -Wall -g -Werror
.PHONY:all build deploy htslib clean _priv_example

export LD_RUN_PATH=${LIBDIR}

all: deploy 

deploy: build
	rm -rf /var/www/mod_bio
	cp -r resources  /var/www/mod_bio
	service apache2 restart

build:	$(foreach M,fastq tabix bam faidx, src/mod_${M}/mod_${M}.slo )


src/mod_bam/mod_bam.slo : \
		$(LIBDIR)/libhts.so \
		src/mod_bio_version.h \
		src/mod_bam/mod_bam.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -I$(HTSDIR) -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'${CFLAGS}'  $(filter %.c,$^) -lhts -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)


src/mod_faidx/mod_faidx.slo : \
		$(LIBDIR)/libhts.so \
		src/mod_bio_version.h \
		src/mod_faidx/mod_faidx.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -I$(HTSDIR) -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'${CFLAGS}'  $(filter %.c,$^) -lhts -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)


src/mod_fastq/mod_fastq.slo : \
		$(LIBDIR)/libhts.so \
		src/mod_bio_version.h \
		src/mod_fastq/mod_fastq.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -I$(HTSDIR) -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'${CFLAGS}'  $(filter %.c,$^) -lhts  -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)

src/mod_tabix/mod_tabix.slo : \
		$(LIBDIR)/libhts.so \
		src/mod_bio_version.h \
		src/mod_tabix/mod_tabix.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -I$(HTSDIR)  -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'${CFLAGS}'  $(filter %.c,$^) -lhts -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)



$(LIBDIR)/libhts.so:
	$(MAKE) -C $(HTSDIR) && \
	cp $(HTSDIR)/libhts.so $@


src/mod_bio_version.h:
	echo -n '#define MOD_BIO_VERSION "' > $@
	-git rev-parse HEAD  | tr -d "\n" >> $@
	echo '"' >> $@



clean:
	find src -type f -name "*.lo" -o -name "*.la"  -o -name "*.so" -o -name "*.o" -delete
	$(MAKE) -C $(HTSDIR) clean

_priv_example: 
	cp examples/.htaccess \
	   examples/rf.* \
	   examples/out*.fq.gz \
	  examples/bam2raster.js \
	examples/hershey.js \
	examples/jsonp.js \
	examples/samtools.js \
	examples/footer.html \
	examples/00EXAMPLE.html \
	~/public_html/mod_bio/

