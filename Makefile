HTSDIR=htslib
SAMTOOLS=samtools
include make.properties

LIBDIR?=/usr/local/lib
APXS?=apxs2
LIBTOOL?=/usr/share/apr-1.0/build/libtool

.PHONY:all build deploy samtools htslib clean

all: deploy 

deploy: build
	service apache2 restart

build: src/mod_fastq/mod_fastq.slo

src/mod_fastq/mod_fastq.slo : samtools htslib src/mod_fastq/mod_fastq.c src/r_utils.c  src/r_utils.h 
	${APXS} -Isamtools -I$(HTSDIR) -I$(HTSDIR)/htslib -Isrc -Lsamtools -L$(HTSDIR) \
		-Wl,-rpath -Wl,$(LIBDIR) -i -a -c -Wc,'-Wall -g'  $(filter %.c,$^) -lm -lz -lhts -lbam && \
	$(LIBTOOL) --finish $(LIBDIR)

##include $(HTSDIR)/htslib.mk

htslib:
	$(MAKE) -C $(HTSDIR) && \
	cp $(HTSDIR)/libhts.so $(LIBDIR)/libhts.so && \
	$(LIBTOOL) --finish $(LIBDIR)

samtools :
	$(MAKE) -C $(SAMTOOLS) && \
	gcc -nostdlib --shared -Wl,--whole-archive $(SAMTOOLS)/libbam.a -o $(LIBDIR)/libbam.so && \
	$(LIBTOOL) --finish $(LIBDIR)



clean:
	 $(MAKE) -C samtools clean
