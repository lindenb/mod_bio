
## Setup Apache Directory Listing
See also: http://perishablepress.com/better-default-directory-views-with-htaccess/

In the .htaccess file of the directory (server side) you want to have an index, add the following (source: http://paradox460.newsvine.com/_news/2008/04/05/1413490-how2-stylish-apache-directory-listings )

```
AddHandler fastq-handler .fastq.gz
AddHandler fastq-handler .fastq
AddHandler tabix-handler .gz


<IfModule mod_autoindex.c>
IndexOptions +HTMLTable
IndexOptions +SuppressRules
IndexOptions +SuppressHTMLPreamble


# SPECIFY HEADER FILE
ReadmeName footer.html

# IGNORE THESE FILES
IndexIgnore header.html footer.html .htaccess

</IfModule> 


AddIcon /icons/compressed.gif .bam
AddIcon /icons/compressed.gif .fastq


#
# Adding Descriptions to Folders and Files
#
AddDescription "Binary Sequence Alignment Map <a href='http://samtools.github.io/hts-specs/SAMv1.pdf' target='_blank'>(BAM)</a>" .bam
AddDescription "Sequence Alignment Map <a href='http://samtools.github.io/hts-specs/SAMv1.pdf' target='_blank'>(SAM)</a>" .sam
AddDescription "<a href='http://en.wikipedia.org/wiki/FASTQ_format' target='_blank'>FastQ</a>" .fastq
AddDescription "<a href='http://samtools.sourceforge.net/tabix.shtml' target='_blank'>Tabix index</a>" .tbi
```

$ git clone  "https://github.com/lindenb/mod_bio.git"
$ git submodule update --init --recursive

