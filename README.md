***mod_bio*** is a set of ***Apache modules*** ( http://httpd.apache.org/modules/ ) for Bioinformatics based on the <a href="https://github.com/samtools/htslib">htslib C</a> library. It provides a quick way to explore bioinformatics files and to develop ***web applications*** using  remote data.


The files currently managed by mod_bio are:

* ***BAM*** files ( http://www.ncbi.nlm.nih.gov/pubmed/19505943 )
* Genomic files indexed with ***Tabix*** (http://samtools.sourceforge.net/tabix.shtml)
* Fastq file
* Fasta files  indexed with ***samtools faidx*** 

Using the parameters `format=` the modules can export data as 

* plain text
* XML
* JSON and JSON-P  ( http://en.wikipedia.org/wiki/JSONP)
* HTML

By default, only a few records are printed. settings the parameter `limit=-1` returns all the records.


All modules (but mod_fastq) can be queried using a genomic-position using the parameters `region=(chrom:start-end)`



## What does it look like ?

When ***mod_bio*** is **not** installed, apache displays the following kind of screen

![without](doc/img/no_htaccess.jpg?raw=true)

when ***mod_bio*** is ***enabled***, some extra-hyperlinks are added:


![without](doc/img/with_htaccess.jpg?raw=true)

### An example: The Genome Browser

As an example, a javascript-based genome-browser using json-p is provided.

![gb](doc/img/jsonp01.jpg?raw=true)



# Modules

## mod_faidx
This apache module handles the fasta files indexed with   ***samtools faidx*** 


### Example: format=plain

http://localhost/path1/path2/rf.fa?format=plain&region=RF02%3A1-200

```text
>RF02:1-200
gctattaaaggCtcaATGGCGTACAGGAAACGTGGAGCGCGCCGTGAGGC
GAATATAAATAATAATGACCGAATGCAAGAGAAAGATGACGAGAAACAAG
ATCAAAACAATAGAATGCAGTTGTCTGATAAAGTACTTTCAAAGAAAGAG
GAAGTCGTAACCGACAGTCAAGAAGAAATTAAAATTGCTGATGAAGTGAA
```

### Example: format=html

http://localhost/path1/path2/rf.fa?format=html&region=RF02%3A1-200


![HTML](doc/img/bam_html_01.jpg?raw=true)


### Example: format=xml

http://localhost/path1/path2/rf.fa?format=html&region=RF02%3A1-200

```xml
<?xml version="1.0" encoding="UTF-8"?>
<faidx  git-version="11f13c8b9a8fb4dee7713c2860094c86fb3acc01">
  <sequence  chrom="RF02" start="1" end="200">gctattaaaggCtcaATGGCGTACAGGAAACGTGGAGCGCGCCGTGAGGCGAATATAAATAATAATGACCGAATGCAAGAGAAAGATGACGAGAAACAAGATCAAAACAATAGAATGCAGTTGTCTGATAAAGTACTTTCAAAGAAAGAGGAAGTCGTAACCGACAGTCAAGAAGAAATTAAAATTGCTGATGAAGTGAA</sequence>
</faidx>
```

### Example: format=json

http://localhost/path1/path2/rf.fa?format=json&region=RF02%3A1-200&callback=myfunction

```json
{"chrom":"RF02","start":1,"end":200,"sequence":"gctattaaaggCtcaATGGCGTACAGGAAACGTGGAGCGCGCCGTGAGGCGAATATAAATAATAATGACCGAATGCAAGAGAAAGATGACGAGAAACAAGATCAAAACAATAGAATGCAGTTGTCTGATAAAGTACTTTCAAAGAAAGAGGAAGTCGTAACCGACAGTCAAGAAGAAATTAAAATTGCTGATGAAGTGAA"}
```

### Example: format=json with callback (JSON-P)

http://localhost/path1/path2/rf.fa?format=json&region=RF02%3A1-200&callback=myfunction

```javascript
myfunction({"chrom":"RF02","start":1,"end":200,"sequence":"gctattaaaggCtcaATGGCGTACAGGAAACGTGGAGCGCGCCGTGAGGCGAATATAAATAATAATGACCGAATGCAAGAGAAAGATGACGAGAAACAAGATCAAAACAATAGAATGCAGTTGTCTGATAAAGTACTTTCAAAGAAAGAGGAAGTCGTAACCGACAGTCAAGAAGAAATTAAAATTGCTGATGAAGTGAA"});
```

## mod_fastq
This apache module handles the fastq files




### Example: format=plain

http://localhost/path1/path2/out.read1.fq.gz?format=text&limit=2

```text
@1/1
CTTAGGGCGATGTTACACTCTTTTTATCTAATAACTCTATGGTACTATATTCAGACGTGTCCAAGTGGGA
+
2222222222222222222222222222222222222222222222222222222222222222222222
@2/1
ACAATACAGCAGTAACTGAACATATGTTTCAATATTTACAGACGACGTGAGAGAAACATATGCGCGAATG
+
2222222222222222222222222222222222222222222222222222222222222222222222
```

### Example: format=html

http://localhost/path1/path2/out.read1.fq.gz?format=html&limit=2

![HTML](doc/img/fastq_html_01.jpg?raw=true)

### Example: format=xml

http://localhost/path1/path2/out.read1.fq.gz?format=xml&limit=2

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- version 11f13c8b9a8fb4dee7713c2860094c86fb3acc01-->
<fastqs>
  <fastq>
    <name>1/1</name>
    <seq>CTTAGGGCGATGTTACACTCTTTTTATCTAATAACTCTATGGTACTATATTCAGACGTGTCCAAGTGGGA</seq>
    <qual>2222222222222222222222222222222222222222222222222222222222222222222222</qual>
  </fastq>
  <fastq>
    <name>2/1</name>
    <seq>ACAATACAGCAGTAACTGAACATATGTTTCAATATTTACAGACGACGTGAGAGAAACATATGCGCGAATG</seq>
    <qual>2222222222222222222222222222222222222222222222222222222222222222222222</qual>
  </fastq>
</fastqs>
```

### Example: format=json

http://localhost/path1/path2/out.read1.fq.gz?format=json&limit=2

```json
[
    {
        "len": 70, 
        "name": "1/1", 
        "qual": "2222222222222222222222222222222222222222222222222222222222222222222222", 
        "seq": "CTTAGGGCGATGTTACACTCTTTTTATCTAATAACTCTATGGTACTATATTCAGACGTGTCCAAGTGGGA"
    }, 
    {
        "len": 70, 
        "name": "2/1", 
        "qual": "2222222222222222222222222222222222222222222222222222222222222222222222", 
        "seq": "ACAATACAGCAGTAACTGAACATATGTTTCAATATTTACAGACGACGTGAGAGAAACATATGCGCGAATG"
    }
]
```

### Example: format=json with callback (JSON-P)
 
http://localhost/path1/path2/out.read1.fq.gz?format=json&limit=2&callback=myfun

```javascript
myfun([
{"name":"1/1","len":70,"seq":"CTTAGGGCGATGTTACACTCTTTTTATCTAATAACTCTATGGTACTATATTCAGACGTGTCCAAGTGGGA","qual":"2222222222222222222222222222222222222222222222222222222222222222222222"},
{"name":"2/1","len":70,"seq":"ACAATACAGCAGTAACTGAACATATGTTTCAATATTTACAGACGACGTGAGAGAAACATATGCGCGAATG","qual":"2222222222222222222222222222222222222222222222222222222222222222222222"}
]
);
```

## mod_bam
This apache module handle the bam files indexed with `samtools index`


### Example: format=plain

http://localhost/path1/path2/rf.bam?format=plain&limit=5&region=RF02:200-300
```text
@HD	VN:1.3	SO:coordinate
@SQ	SN:RF01	LN:3302
@SQ	SN:RF02	LN:2687
@SQ	SN:RF03	LN:2592
@SQ	SN:RF04	LN:2362
@SQ	SN:RF05	LN:1579
@SQ	SN:RF06	LN:1356
@SQ	SN:RF07	LN:1074
@SQ	SN:RF08	LN:1059
@SQ	SN:RF09	LN:1062
@SQ	SN:RF10	LN:751
@SQ	SN:RF11	LN:666
293	99	RF02	136	60	70M	=	538	473	ACTTTCAAAGAAACAGGAAGTCGTAACCCACAGTCAAGAAGAAATTAAAATTGTTGATGAAGTGAAGAAA	2222222222222222222222222222222222222222222222222222222222222222222222	NM:i:3	AS:i:55	XS:i:0
247	99	RF02	141	60	70M	=	451	379	CAAAGAAACAGGAAGTCGTAACCGACAGGCAAGAAGAAATTAAAATTTCTGATGAAGTGAAGAAATCGAC	2222222222222222222222222222222222222222222222222222222222222222222222	NM:i:3	AS:i:55	XS:i:0
211	99	RF02	156	60	70M	=	581	497	TCGAAACCCACAGTCAAGAAGAAATTAAAATTGTTGATGAAGTGAAGAAATCGACGAAGGAAGACTCTAA	2222222222222222222222222222222222222222222222222222222222222222222222	NM:i:5	AS:i:46	XS:i:0
228	99	RF02	186	60	70M	=	683	567	TTGCTGATGAAGTGAATAAATCGACGAAGGAAGAATCTAAACAATTGCTTGAAGTTTTGAAAACAAAACA	2222222222222222222222222222222222222222222222222222222222222222222222	NM:i:3	AS:i:58	XS:i:0
315	99	RF02	194	60	70M	=	532	409	GAAGTGAAGAAATCGACGAAGGAAGAATCTAAACAATTTCTTGAAGTTTTTAAAACAAAACAAGCGCACC	2222222222222222222222222222222222222222222222222222222222222222222222	NM:i:5	AS:i:45	XS:i:0
```


### Example: format=html

http://localhost/path1/path2/rf.bam?format=html&limit=5&region=RF02:200-300

![HTML](doc/img/bam_html_01.jpg?raw=true)


### Example: format=xml

http://localhost/path1/path2/rf.bam?format=xml&limit=5&region=RF02:200-300

```xml
<?xml version="1.0" encoding="UTF-8"?>
<sam-file git-version="11f13c8b9a8fb4dee7713c2860094c86fb3acc01">
  <header>@HD	VN:1.3	SO:coordinate
@SQ	SN:RF01	LN:3302
@SQ	SN:RF02	LN:2687
(...)
@SQ	SN:RF10	LN:751
@SQ	SN:RF11	LN:666
</header>
  <records>
    <sam>
      <name>293</name>
      <flag>99</flag>
      <chrom>RF02</chrom>
      <pos>136</pos>
      <mapq>60</mapq>
      <cigar-string>
        <cigar op="M" count="70"/>
      </cigar-string>
      <mate_chrom>RF02</mate_chrom>
      <mate_pos>538</mate_pos>
      <insert_size>474</insert_size>
      <sequence>ACTTTCAAAGAAACAGGAAGTCGTAACCCACAGTCAAGAAGAAATTAAAATTGTTGATGAAGTGAAGAAA</sequence>
      <qual>2222222222222222222222222222222222222222222222222222222222222222222222</qual>
      <aux-list>
        <aux name="NM" type="i">3</aux>
        <aux name="XS" type="i">0</aux>
      </aux-list>
    </sam>
    (...)
  </records>
</sam-file>
```

### Example: format=json


http://localhost/path1/path2/rf.bam?format=json&limit=2&region=RF02:200-300

```json
{
    "header": "@HD\tVN:1.3\tSO:coordinate\n@SQ\tSN:RF01\tLN:3302\n@SQ\tSN:RF02\tLN:2687\n@SQ\tSN:RF03\tLN:2592\n@SQ\tSN:RF04\tLN:2362\n@SQ\tSN:RF05\tLN:1579\n@SQ\tSN:RF06\tLN:1356\n@SQ\tSN:RF07\tLN:1074\n@SQ\tSN:RF08\tLN:1059\n@SQ\tSN:RF09\tLN:1062\n@SQ\tSN:RF10\tLN:751\n@SQ\tSN:RF11\tLN:666\n", 
    "records": [
        {
            "aux": [
                {
                    "name": "NM", 
                    "type": "i:", 
                    "value": 3
                }, 
                {
                    "name": "XS", 
                    "type": "i:", 
                    "value": 0
                }
            ], 
            "chrom": "RF02", 
            "cigar": [
                {
                    "count": 70, 
                    "op": "M"
                }
            ], 
            "flag": 99, 
            "insert-size": 474, 
            "mapq": 60, 
            "mate-chrom": "RF02", 
            "mate-pos": 538, 
            "name": "293", 
            "pos": 136, 
            "qual": "2222222222222222222222222222222222222222222222222222222222222222222222", 
            "sequence": "ACTTTCAAAGAAACAGGAAGTCGTAACCCACAGTCAAGAAGAAATTAAAATTGTTGATGAAGTGAAGAAA"
        }, 
        {
            "aux": [
                {
                    "name": "NM", 
                    "type": "i:", 
                    "value": 3
                }, 
                {
                    "name": "XS", 
                    "type": "i:", 
                    "value": 0
                }
            ], 
            "chrom": "RF02", 
            "cigar": [
                {
                    "count": 70, 
                    "op": "M"
                }
            ], 
            "flag": 99, 
            "insert-size": 380, 
            "mapq": 60, 
            "mate-chrom": "RF02", 
            "mate-pos": 451, 
            "name": "247", 
            "pos": 141, 
            "qual": "2222222222222222222222222222222222222222222222222222222222222222222222", 
            "sequence": "CAAAGAAACAGGAAGTCGTAACCGACAGGCAAGAAGAAATTAAAATTTCTGATGAAGTGAAGAAATCGAC"
        }
    ]
}
```

### Example: format=json with callback (JSON-P)

http://localhost/path1/path2/rf.bam?format=json&limit=2&region=RF02:200-300&callback=myfun

```javascript
myfun({"header":"@HD\tVN:1.3\tSO:coordinate\n@SQ\tSN:RF01\tLN:3302\n@SQ\tSN:RF02\tLN:2687\n@SQ\tSN:RF03\tLN:2592\n@SQ\tSN:RF04\tLN:2362\n@SQ\tSN:RF05\tLN:1579\n@SQ\tSN:RF06\tLN:1356\n@SQ\tSN:RF07\tLN:1074\n@SQ\tSN:RF08\tLN:1059\n@SQ\tSN:RF09\tLN:1062\n@SQ\tSN:RF10\tLN:751\n@SQ\tSN:RF11\tLN:666\n","records":[
{"name":"293","flag":99,"chrom":"RF02","pos":136,"mapq":60,"cigar":[{"op":"M","count":70}],"mate-chrom":"RF02","mate-pos":538,"insert-size":474,"sequence":"ACTTTCAAAGAAACAGGAAGTCGTAACCCACAGTCAAGAAGAAATTAAAATTGTTGATGAAGTGAAGAAA","qual":"2222222222222222222222222222222222222222222222222222222222222222222222","aux":[{"name":"NM","type":"i:","value":3},{"name":"XS","type":"i:","value":0}]},
{"name":"247","flag":99,"chrom":"RF02","pos":141,"mapq":60,"cigar":[{"op":"M","count":70}],"mate-chrom":"RF02","mate-pos":451,"insert-size":380,"sequence":"CAAAGAAACAGGAAGTCGTAACCGACAGGCAAGAAGAAATTAAAATTTCTGATGAAGTGAAGAAATCGAC","qual":"2222222222222222222222222222222222222222222222222222222222222222222222","aux":[{"name":"NM","type":"i:","value":3},{"name":"XS","type":"i:","value":0}]}]});
```


## mod_tabix
This apache module handle the bam files indexed with ***tabix*** .

### Example: format=text

http://localhost/~lindenb/modules/rf.vcf.gz?format=plain&limit=2&region=RF02:200-300&callback=myfun


```
##fileformat=VCFv4.1
##samtoolsVersion=0.1.19-44428cd
##reference=file://rf.fa
(...)
##FORMAT=<ID=PL,Number=G,Type=Integer,Description="List of Phred-scaled genotype likelihoods">
#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO	FORMAT	rf.bam
RF02	214	.	A	G	10.4	.	DP=3;VDB=6.570053e-02;AF1=1;AC1=2;DP4=0,0,3,0;MQ=60;FQ=-36	GT:PL:DP:SP:GQ	1/1:42,9,0:3:0:13
RF02	254	.	G	C	24.3	.	DP=5;VDB=8.892797e-02;AF1=1;AC1=2;DP4=0,0,5,0;MQ=60;FQ=-42	GT:PL:DP:SP:GQ	1/1:57,15,0:5:0:27
```


### Example: format=html

http://localhost/path1/path2/rf.vcf?format=html&limit=2&region=RF02:200-300

![HTML](doc/img/vcf_html_01.jpg?raw=true)

### Example: format=xml

http://localhost/path1/path2/rf.vcf.gz?format=xml&limit=2&region=RF02:200-300

```xml
<?xml version="1.0" encoding="UTF-8"?>
<tabix-file>
  <header>
    <line>##fileformat=VCFv4.1</line>
    <line>##samtoolsVersion=0.1.19-44428cd</line>
    (...)
    <line>##FORMAT=&lt;ID=PL,Number=G,Type=Integer,Description="List of Phred-scaled genotype likelihoods"&gt;</line>
    <line>#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO	FORMAT	rf.bam</line>
  </header>
  <body>
    <tr>
      <td type="chrom">RF02</td>
      <td type="pos">214</td>
      <td>.</td>
      <td>A</td>
      <td>G</td>
      <td>10.4</td>
      <td>.</td>
      <td>DP=3;VDB=6.570053e-02;AF1=1;AC1=2;DP4=0,0,3,0;MQ=60;FQ=-36</td>
      <td>GT:PL:DP:SP:GQ</td>
      <td>1/1:42,9,0:3:0:13</td>
    </tr>
    <tr>
      <td type="chrom">RF02</td>
      <td type="pos">254</td>
      <td>.</td>
      <td>G</td>
      <td>C</td>
      <td>24.3</td>
      <td>.</td>
      <td>DP=5;VDB=8.892797e-02;AF1=1;AC1=2;DP4=0,0,5,0;MQ=60;FQ=-42</td>
      <td>GT:PL:DP:SP:GQ</td>
      <td>1/1:57,15,0:5:0:27</td>
    </tr>
  </body>
</tabix-file>
```

### Example: format=json

http://localhost/path1/path2/rf.vcf.gz?format=json&limit=2&region=RF02:200-300

```json
{
    "body": [
        [
            "RF02", 
            214, 
            null, 
            "A", 
            "G", 
            10.4, 
            null, 
            "DP=3;VDB=6.570053e-02;AF1=1;AC1=2;DP4=0,0,3,0;MQ=60;FQ=-36", 
            "GT:PL:DP:SP:GQ", 
            "1/1:42,9,0:3:0:13"
        ], 
        [
            "RF02", 
            254, 
            null, 
            "G", 
            "C", 
            24.3, 
            null, 
            "DP=5;VDB=8.892797e-02;AF1=1;AC1=2;DP4=0,0,5,0;MQ=60;FQ=-42", 
            "GT:PL:DP:SP:GQ", 
            "1/1:57,15,0:5:0:27"
        ]
    ], 
    "header": [
        "##fileformat=VCFv4.1", 
        "##samtoolsVersion=0.1.19-44428cd", 
        "##reference=file://rf.fa", 
        (...)
        "##FORMAT=<ID=PL,Number=G,Type=Integer,Description=\"List of Phred-scaled genotype likelihoods\">", 
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\trf.bam"
    ]
}
```

### Example: format=json with callback (JSON-P)

http://localhost/path1/path2/rf.vcf.gz?format=json&limit=2&region=RF02:200-300&callback=myfun

```javascript
myfun({"header":["##fileformat=VCFv4.1",
"##samtoolsVersion=0.1.19-44428cd",
"##reference=file://rf.fa",
"##contig=<ID=RF01,length=3302>",
"##contig=<ID=RF02,length=2687>",
(...)
"##FORMAT=<ID=SP,Number=1,Type=Integer,Description=\"Phred-scaled strand bias P-value\">",
"##FORMAT=<ID=PL,Number=G,Type=Integer,Description=\"List of Phred-scaled genotype likelihoods\">",
"#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\trf.bam"]
,"body":[["RF02",214,null,"A","G",10.4,null,"DP=3;VDB=6.570053e-02;AF1=1;AC1=2;DP4=0,0,3,0;MQ=60;FQ=-36","GT:PL:DP:SP:GQ","1/1:42,9,0:3:0:13"],
["RF02",254,null,"G","C",24.3,null,"DP=5;VDB=8.892797e-02;AF1=1;AC1=2;DP4=0,0,5,0;MQ=60;FQ=-42","GT:PL:DP:SP:GQ","1/1:57,15,0:5:0:27"]]}
);
```



## Installation




![with](with_htaccess.jpg?raw=true)

![without](no_htaccess.jpg?raw=true)


Clone the project


$ git clone  "https://github.com/lindenb/mod_bio.git"
$ git submodule update --init --recursive

## Setup Apache Directory Listing


