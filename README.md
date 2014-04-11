
## Setup Apache Directory Listing
See also: http://perishablepress.com/better-default-directory-views-with-htaccess/

In the .htaccess file of the directory (server side) you want to have an index, add the following (source: http://paradox460.newsvine.com/_news/2008/04/05/1413490-how2-stylish-apache-directory-listings )

```
AddHandler example-handler .example 
AddHandler fastq-handler .fastq.gz
AddHandler fastq-handler .fastq



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
AddIcon /icons/compressed.gif x.example

#
# Adding Descriptions to Folders and Files
#
AddDescription "Binary Sequence Alignment Map <a href='http://samtools.github.io/hts-specs/SAMv1.pdf' target='_blank'>(BAM)</a>" .bam
AddDescription "Sequence Alignment Map <a href='http://samtools.github.io/hts-specs/SAMv1.pdf' target='_blank'>(SAM)</a>" .bam
AddDescription "<a href='http://en.wikipedia.org/wiki/FASTQ_format' target='_blank'>FastQ</a>" .fastq

```


