HTSDIR=htslib
SAMTOOLS=samtools
include make.properties

LIBDIR?=/usr/local/lib
APXS?=apxs2
LIBTOOL?=/usr/share/apr-1.0/build/libtool

.PHONY:all build deploy samtools htslib clean

export LD_RUN_PATH=${LIBDIR}

all: deploy 

deploy: build
	service apache2 restart

build:	src/mod_fastq/mod_fastq.slo \
		src/mod_tabix/mod_tabix.slo \
		src/mod_faidx/mod_faidx.slo

src/mod_faidx/mod_faidx.slo : \
		samtools htslib \
		src/mod_bio_version.h \
		src/mod_faidx/mod_faidx.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -Isamtools -I$(HTSDIR) -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'-Wall -g'  $(filter %.c,$^) -lhts -lbam -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)


src/mod_fastq/mod_fastq.slo : \
		samtools htslib \
		src/mod_bio_version.h \
		src/mod_fastq/mod_fastq.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -Isamtools -I$(HTSDIR) -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'-Wall -g'  $(filter %.c,$^) -lhts -lbam -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)

src/mod_tabix/mod_tabix.slo : \
		samtools htslib \
		src/mod_bio_version.h \
		src/mod_tabix/mod_tabix.c \
		src/r_utils.c  \
		src/r_utils.h
	${APXS} -Isamtools -I$(HTSDIR)  -Isrc -L$(LIBDIR) \
		-i -a -c -Wc,'-Wall -g'  $(filter %.c,$^) -lhts -lbam -lm -lz  && \
	$(LIBTOOL) --finish $(LIBDIR)



htslib:
	$(MAKE) -C $(HTSDIR) && \
	cp $(HTSDIR)/libhts.so $(LIBDIR)/libhts.so && \
	$(LIBTOOL) --finish $(LIBDIR)

samtools :
	$(MAKE) -C $(SAMTOOLS) && \
	gcc -nostdlib --shared -Wl,--whole-archive $(SAMTOOLS)/libbam.a -o $(LIBDIR)/libbam.so && \
	$(LIBTOOL) --finish $(LIBDIR)

src/mod_bio_version.h:
	echo -n '#define MOD_BIO_VERSION "' > $@
	-git rev-parse HEAD  | tr -d "\n" >> $@
	echo '"' >> $@


examples/ex1.fa.fai:
	mkdir -p $(dir $@) && \
    cp $(SAMTOOLS)/examples/ex1.fa $(basename $@) && \
	$(SAMTOOLS)/samtools faidx $(basename $@)


clean:
	find src -type f -name "*.lo" -o -name "*.la"  -o -name "*.so" -o -name "*.o" -delete
	$(MAKE) -C samtools clean
