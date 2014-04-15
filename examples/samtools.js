
function SamRecord(b)
	{
	this.b=b;
	this.alignEnd=null;
	};

SamRecord.prototype.isFlagSet=function(f)
	{
	return this.b.flag & f;
	};
	
SamRecord.prototype.isUnmapped=function()
	{
	return this.isFlagSet(0x4);
	};

SamRecord.prototype.isMapped=function()
	{
	return !this.isUnmapped();
	};

SamRecord.prototype.getAlignmentStart=function()
	{
	return this.b.pos;
	};
	
SamRecord.prototype.getAlignmentEnd=function()
	{
	if(this.isUnmapped()) return 0;
	if(this.alignEnd==null)
		{
		this.alignEnd=this.getAlignmentStart();
		for (var i in this.b.cigar)
		  	{
		  	var element=this.b.cigar[i];
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

