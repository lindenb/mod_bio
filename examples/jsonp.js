
function GenomeBrowser()
	{
	this.width=1000;
	this.rowHeigt=20;
	this.chromStart=0;
	this.chromEnd=0;
	this.interval={"chrom":null,"start":0,"end":0};
	this.genomicSequence={
		str:null,
		start0:0,
		length:function()
			{
			return this.str==null?0:this.str.length;
			},
		charAt:function(idx)
			{
			if(this.str==null) return 'N';
			if(idx<this.start0) return 'N';
			idx -= this.start0;
			if(idx>= this.str.length ) return 'N';
			return this.str[idx];
			}
		};
	
	}
GenomeBrowser.prototype.baseToPixel=function(pos)
	{
	return ((pos-this.chromStart)/(this.chromEnd-this.chromStart))*this.width;
	};


GenomeBrowser.prototype.convertToX=function(genomic)
	{
	return this.WIDTH*(1.0*genomic-interval.getStart())/(1.0*(interval.getEnd()-interval.getStart()+1));
	};
	
GenomeBrowser.prototype.left=function(rec)
	{
	return this.convertToX(rec.getAlignmentStart());
	};

GenomeBrowser.prototype.right=function(rec)
	{
	return this.convertToX(rec.getAlignmentEnd());
	};
	
GenomeBrowser.prototype.right=base2color=function(c)
	{
	switch(c)
		{
		case 'n':case 'N': return "black";
		case 'a':case 'A': return "red";
		case 't':case 'T': return "green";
		case 'g':case 'G': return "yellow";
		case 'c':case 'C': return "blue";
		default: return "orange";
		}
	};
	

/* receive JSON data from server */
function mycallback(JSONdata)
  {
  


  }

function parsePosition(s)
	{
	var colon=s.indexOf(':');
	if(colon<1) return null;
	var ret={
		chrom: s.substr(0,colon),
		pos: parseInt( s.substr(colon+1).replace(/[,]/g, '') )
		};
	if(isNaN(ret.pos) || ret.pos <0) return null;
	return ret;
	}

/* send data to server */
function invokejsonp()
  {
  gconf.width=parseInt(document.getElementById('width').value);
  if(!isNaN(gconf.width)) gconf.width=1000;
  

  
  var bamE=document.getElementById('bam');
  var posE=document.getElementById('position');
  var region=parsePosition(posE.value);
  if(region==null)
  	{
  	document.getElementById('msg').innerHtml="BAD REGION";
  	return ;
  	}
  gconf.chromStart=region.pos;
  gconf.chromEnd=gconf.chromStart+100;
  var prev=document.getElementById('jsonpid');
  if(prev!=null) prev.parentNode.removeChild(prev);
  var ext=document.createElement("script");
  ext.setAttribute('id', 'jsonpid');
	var url= encodeURI(bamE.value)+'?format=json&callback=mycallback&region='+
  	encodeURI(region.chrom+":"+region.pos+"-"+(region.pos+100));
 
  ext.setAttribute('src',url);
  document.getElementsByTagName("head")[0].appendChild(ext);
  }
 
 
function moveView(side,percent)
	{
	var shift=Math.round(viewLength*percent);
	if(shift < 1) shift=1;
	if(side < 0)
		{
		newStart=viewStart-shift;
		if(newStart < chromStart) newStart=chromStart;
		}
	else
		{
		newStart=viewStart+shift;
		if(newStart + viewLength < chromEnd) newStart=chromEnd-shift;
		}
	viewStart=newStart;
	updateTitle(null);
	reload();
	}
  
 function searchTerm()
 	{
 	alert("ok");
 	}

