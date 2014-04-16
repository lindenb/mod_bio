
function SamRecord(b)
	{
	this.b=b;
	this.alignEnd=null;
	};
SamRecord.prototype.toString=function()
	{
	return this.b.name+" "+this.getAlignmentStart()+"-"+this.getAlignmentEnd();
	};
	
SamRecord.prototype.isFlagSet=function(f)
	{
	return this.b.flag & f;
	};

SamRecord.prototype.getReadPairedFlag=function()
	{
	return this.isFlagSet(0x1);
	};

SamRecord.prototype.getProperPairFlag=function()
	{
	if(!this.getReadPairedFlag() ) return false;
	return this.isFlagSet(0x2);
	};

SamRecord.prototype.getReadUnmappedFlag=function()
	{
	return this.isFlagSet(0x4);
	};
SamRecord.prototype.getMateUnmappedFlag=function()
	{
	if(!this.getReadPairedFlag() ) return false;
	return   this.isFlagSet(0x8);
	};
	
SamRecord.prototype.getReadNegativeStrandFlag=function()
	{
	return this.isFlagSet(0x10);
	};

SamRecord.prototype.getNotPrimaryAlignmentFlag=function()
	{
	return this.isFlagSet(0x100);
	};


SamRecord.prototype.getSupplementaryAlignmentFlag=function()
	{
	return this.isFlagSet(0x200);
	};	

SamRecord.prototype.getDuplicateReadFlag=function()
	{
	return this.isFlagSet(0x400);
	};
	

SamRecord.prototype.getReadFailsVendorQualityCheckFlag=function()
	{
	return this.isFlagSet(0x800);
	};



SamRecord.prototype.getAlignmentStart=function()
	{
	return this.b.pos;
	};



	
SamRecord.prototype.getAlignmentEnd=function()
	{
	if(this.getReadUnmappedFlag()) return 0;
	if(this.alignEnd==null)
		{
		var cigar=this.getCigar();
		this.alignEnd=this.getAlignmentStart();
		for (var i in cigar)
		  	{
		  	var element=cigar[i];
			   	switch (element.op) {
				case 'M':
				case 'D':
				case 'N':
				case 'EQ':
				case 'X': this.alignEnd += element.count;break;
				default:break;
			   	}
			  }
            	}
        return this.alignEnd;
	};

SamRecord.prototype.getCigar=function()
	{
	if(this.getReadUnmappedFlag()) return [];
	if(!this.b.cigar)  return [];
        return this.b.cigar;
	};

SamRecord.prototype.getBaseAt=function(idx)
	{
	if(!this.b.sequence) return '?';
	if(idx<0 || idx>=this.b.sequence.length) return '?';
        return this.b.sequence.charAt(idx);
	};



