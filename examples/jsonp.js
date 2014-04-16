/**
Author: Pierre Lindenbaum PhD
	@yokofakun
	http://plindenbaum.blogspot.com
Date: April 2014

The MIT License.

**/
function GenomeBrowser()
	{
	this.chromLength=100;
	this.width=1000;
	this.featureHeight=20;
	this.chromStart=0;
	this.chromEnd=0;
	this.bamFile='rf.bam';
	this.refFile='rf.fa';
	this.minHDistance=5;
	this.spaceYbetweenFeatures=2;
	this.minArrowWidth=2;
	this.maxArrowWidth=5;
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
	this.hershey=new Hershey();
	}

/** the browser oject*/
var gBrowse=new GenomeBrowser();

/* pretty print a number, with commas */
GenomeBrowser.prototype.niceNumber=function(n)
	{
	var y = "";
	var arr = parseInt(n).toString().split("");
	for(var i=0; i<arr.length; i++)
		{
	    	y += arr[i];
	    	if((arr.length-i-1)%3==0 && i<arr.length-1) y += ",";
		}
	return y;
	}

/* convert position to pixel */
GenomeBrowser.prototype.baseToPixel=function(pos)
	{
	return ((pos-this.interval.start)/(this.interval.end-this.interval.start))*this.width;
	};


	
GenomeBrowser.prototype.left=function(rec)
	{
	return this.baseToPixel(rec.getAlignmentStart());
	};

GenomeBrowser.prototype.right=function(rec)
	{
	return this.baseToPixel(rec.getAlignmentEnd());
	};

GenomeBrowser.prototype.getReadColor=function(rec)
	{
	if(rec.getMateUnmappedFlag()) return "green";
	if(rec.getNotPrimaryAlignmentFlag()) return "pink";
	if(rec.getDuplicateReadFlag()) return "fuchsia";
	if(rec.getSupplementaryAlignmentFlag()) return "skyblue";
	if(rec.getReadFailsVendorQualityCheckFlag()) return "khaki";
	return "black";
	}

GenomeBrowser.prototype.paint=function(records)
	{

	var iter,rows=[];

	for(iter in records)
		{
		
		var rec=new SamRecord(records[iter]);
		
		if(rec.getReadUnmappedFlag()) continue;
		
		if(rec.getAlignmentEnd() < this.interval.start ) continue;
		
		if(rec.getAlignmentStart() > this.interval.end ) continue;
		
		var y=0;
		for(y=0;y< rows.length;++y)
			{
			var row=rows[y];
			var last=row[row.length-1];
			if(this.right(last)+ this.minHDistance > this.left(rec)) continue;
			
			row.push(rec);
			rec=null;
			break;
			}
		if(rec!=null)
			{
			var row=[];
			row.push(rec);
			rows.push(row);
			}
		}
	
	
	
	var ruler_height=this.niceNumber(this.interval.end).length*20;
	
	var refw=this.width/(1.0+this.interval.end-this.interval.start);if(refw<1.0) refw=1;refw=parseInt(refw);
	
	var margin_top=10+(refw*2)+ruler_height;
	var imageSize=	{
			width:this.width,
			height: margin_top+ rows.length*(this.spaceYbetweenFeatures+this.featureHeight)+this.spaceYbetweenFeatures
			};
	var canvasdoc=document.getElementById("canvasdoc");
	canvasdoc.setAttribute("width",imageSize.width);
	canvasdoc.setAttribute("height",imageSize.height);
	if(!canvasdoc.getContext)return;
	var ctx = canvasdoc.getContext('2d');
	if(ctx==null) return;

	
	/* draw reference sequence */
		
	for(var x=this.interval.start;
		x<=this.interval.end;
		++x)
		{
		var oneBaseWidth=this.baseToPixel(x+1)-this.baseToPixel(x);
		//draw vertical line
		ctx.lineWidth=0.5;
		ctx.strokeStyle=(x%10==0?"black":"gray");
		ctx.beginPath();
		ctx.moveTo(this.baseToPixel(x), 0);
		ctx.lineTo(this.baseToPixel(x), imageSize.height);
		ctx.closePath();
		ctx.stroke();
		//draw base position
		if((x)%10==0)
			{
			ctx.strokeStyle="black";
			var xStr=this.niceNumber(x);
			ctx.save();
			
			
			ctx.translate(this.baseToPixel(x+1), 0);
			ctx.rotate(Math.PI/2.0);
			
			ctx.beginPath();
			this.hershey.paint(ctx,
					xStr,
					0,
					0,
					ruler_height,
					oneBaseWidth
					);
			ctx.stroke();
			ctx.restore();
			}
		
		//paint genomic sequence
		var c=this.genomicSequence.charAt(x-1);
		ctx.lineWidth=1;
		ctx.strokeStyle=(this.base2color(c));
		ctx.beginPath();
		this.hershey.paint(ctx,
				c,
				this.baseToPixel(x)+1,
				ruler_height,
				oneBaseWidth-2,
				oneBaseWidth-2
				);
		ctx.stroke();


		}
	
	ctx.lineWidth=1;
	var y=margin_top+this.spaceYbetweenFeatures;
	/** loop over each row */
	for(var rowY in rows)
		{
		var row=rows[rowY];
		for(x in row)
			{
			var rec=row[x];
			var x0=this.left(rec);

			var x1=this.right(rec);

			var y0=y;
			var y1=y0+this.featureHeight;
			var midY=y0+this.featureHeight/2.0;

			/* draw horizontal line */
			ctx.strokeStyle="black";

			ctx.moveTo(x0,midY);
			ctx.lineTo(x1,midY);
			ctx.stroke();
			
			
			
			/* draw record shape */
			ctx.beginPath();
			
			
			
			if(x1-x0 < this.minArrowWidth)
				{
				ctx.moveTo(x0,y0);
				ctx.lineTo(x1,y0);
				ctx.lineTo(x1,y1);
				ctx.lineTo(x0,y1);
				ctx.lineTo(x0,y0);
				}
			else
				{

				var arrow=Math.max(this.minArrowWidth,Math.min(this.maxArrowWidth, x1-x0));
				if(!rec.getReadNegativeStrandFlag())
					{
					ctx.moveTo(x0, y0);
					ctx.lineTo(x1-arrow,y0);
					ctx.lineTo(x1,midY);
					ctx.lineTo(x1-arrow,y1);
					ctx.lineTo(x0,y1);
					}
				else
					{
					ctx.moveTo(x0+arrow, y0);
					ctx.lineTo(x0,midY);
					ctx.lineTo(x0+arrow,y1);
					ctx.lineTo(x1,y1);
					ctx.lineTo(x1,y0);
					}

				
				}
			ctx.closePath();
			if(!rec.getReadPairedFlag() || rec.getProperPairFlag())
				{
				var grd=ctx.createLinearGradient(x0,y0,x0,y1);
				grd.addColorStop(0,"gray");
				grd.addColorStop(0.5,"white");
				grd.addColorStop(1.0,"gray");
			
				ctx.fillStyle=grd;
				}
			else
				{
				ctx.fillStyle="white";
				}
			ctx.fill();
			

			ctx.lineWidth=1;
			ctx.strokeStyle=this.getReadColor(rec);
			ctx.stroke();
			
			var cigarindex,cigar = rec.getCigar();
			if(cigar==null || cigar.length==0)
				{
				continue;
				}
			var refpos=rec.getAlignmentStart();
			var readpos=0;
			
			/* loop over all cigar */
			for(cigarindex in cigar)
				{
				var ce=cigar[cigarindex];
				var k=0;
				var mutW=this.baseToPixel(refpos+1)-this.baseToPixel(refpos);
				
				/* loop over this cigar-element */
				for(k=0;k< ce.count;++k)
					{
					var next_readpos=readpos;
					var next_refpos=refpos;
					
					

					
					ctx.lineWidth=1;
					ctx.strokeStyle="pink";
					switch(ce.op)
						{
						case 'P':break;
						case 'N':
						case 'D':
							{
							ctx.fillStyle="red";
							ctx.beginPath();
							ctx.fillRect(
								this.baseToPixel(refpos),y0,
								mutW,y1-y0
								);
							ctx.closePath();
							next_refpos++;
							break;
							}
						case 'I':
							{
							ctx.lineWidth=4;
							ctx.strokeStyle="yellow";
							ctx.beginPath();
							ctx.moveTo(this.baseToPixel(refpos),y0);
							ctx.lineTo(this.baseToPixel(refpos),y1);
							ctx.stroke();
							next_readpos++;
							break;
							}
						case 'S':
						case 'H':
							{
							ctx.lineWidth=3;
							ctx.strokeStyle="green";
							ctx.beginPath();
							ctx.moveTo(this.baseToPixel(refpos),y0);
							ctx.lineTo(this.baseToPixel(refpos),y1);
							ctx.stroke();
							if(ce.op=='S') next_readpos++;
							break;
							}
						case 'M':
						case 'X':
						case '=':
							{
							
							var c1=rec.getBaseAt(readpos);
							var c2=this.genomicSequence.charAt(refpos-1);
							if(rec.b.name=="55")
								{
								console.log(""+refpos+" "+readpos+" "+c1+" "+c2+" "+ce.op);
								}
							if(ce.op=='X' || (c1!='N' && c1!='n'  && c1.toUpperCase()!=c2.toUpperCase()))
								{
								ctx.lineWidth=1;
								ctx.strokeStyle="red";
								}
							else
								{
								ctx.lineWidth=0.3;
								ctx.strokeStyle="gray";
								}
							
							ctx.beginPath();
							this.hershey.paint(
								ctx,c1,
								this.baseToPixel(refpos),
								y0+2,
								mutW,
								(y1-y0)-4
								);
							ctx.stroke();
							
							next_readpos++;
							next_refpos++;
							break;
							}
						default: console.log("unknown operator "+ce.op);break; 
						}
					
					readpos=next_readpos;
					refpos=next_refpos;
					
					}
				}

			}
		y+=this.featureHeight+this.spaceYbetweenFeatures;
		}
	
	/* draw frame */
	ctx.strokeStyle="black";
	ctx.rect(0,0,imageSize.width,imageSize.height);
	ctx.stroke();

	
	var titleE=document.getElementById("browserTitle");
	titleE.innerHTML=this.interval.chrom+":"+this.interval.start+"-"+this.interval.end;
	var titleE=document.getElementById("interval");
	titleE.value=this.interval.chrom+":"+this.interval.start;
	
	}
		


GenomeBrowser.prototype.base2color=function(c)
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
	



GenomeBrowser.prototype.parsePosition=function(s)
	{
	var colon=s.indexOf(':');
	if(colon<1)
		{
		console.log("bad colon");
		return null;
		}
	var ret={
		chrom: s.substr(0,colon),
		start: parseInt( s.substr(colon+1).replace(/[,]/g, '') ),
		end:0
		};
	if(ret.chrom.length==0 || isNaN(ret.start) || ret.start <0)
		{
		console.log("bad "+ret.chrom+"/"+ret.start);
		}
	ret.end=ret.start+this.chromLength;
	return ret;
	}

GenomeBrowser.prototype.insertHtmlScript=function(scriptid,src)
	{
	var scriptElement=document.getElementById(scriptid);
 	if(scriptElement!=null) scriptElement.parentNode.removeChild(scriptElement);
  	scriptElement = document.createElement("script");
  	scriptElement.setAttribute('id', scriptid);
	scriptElement.setAttribute('src',src);
	console.log("inserting script.src="+src);
  	document.getElementsByTagName("head")[0].appendChild(scriptElement);
	}


/* receive JSON data from server */
function receiveJsonpBam(JSONdata)
  {
  console.log("ok got bam :"+JSONdata.records.length);
  gBrowse.paint(JSONdata.records);
  }

/* receive JSON data from server */
function receiveJsonpReference(JSONdata)
  {
  console.log("ok got data REF:"+JSONdata.sequence);
  gBrowse.genomicSequence.str=JSONdata.sequence;
  gBrowse.genomicSequence.start0=JSONdata.start;
    
  var url= gBrowse.bamFile+'?format=json&callback=receiveJsonpBam&region='+
  	encodeURI(gBrowse.interval.chrom+":"+(gBrowse.interval.start)+"-"+(gBrowse.interval.end));
  gBrowse.insertHtmlScript("scriptbam",url);
  }

GenomeBrowser.prototype.refresh=function()
  {
  if(this.interval.chrom==null) return;
  var url= this.refFile+'?format=json&callback=receiveJsonpReference&region='+
  	encodeURI(this.interval.chrom+":"+(this.interval.start-1)+"-"+(this.interval.end));
  this.insertHtmlScript("scriptchrom",url);
  }
 
 



/* send data to server */
function invokejsonp()
  {

  var bamE=document.getElementById('bam');
  var posE=document.getElementById('interval');
  
  var interval=gBrowse.parsePosition(posE.value);
  
  if(interval==null)
  	{
  	console.log("Bad region "+posE.value);
  	return ;
  	}
  	
  gBrowse.interval=interval;
  gBrowse.refresh();
  }
  
function moveView(side,percent)
	{
	if(gBrowse.interval.chrom==null) return;
	var viewLength=gBrowse.interval.end - gBrowse.interval.start;
	var newStart;
	var shift=Math.round(viewLength*percent);
	if(shift < 1) shift=1;
	if(side < 0)
		{
		newStart= gBrowse.interval.start-shift;
		if(newStart < 0) newStart=1;
		}
	else
		{
		newStart= gBrowse.interval.start+shift;
		}
	gBrowse.interval.start=newStart;
	gBrowse.interval.end=gBrowse.interval.start+gBrowse.chromLength;
	gBrowse.refresh();
	}
  
window.addEventListener("load",invokejsonp,false);
