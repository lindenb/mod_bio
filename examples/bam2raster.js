
function Bam2Raster()
	{
	this.minHDistance=2;
	this.minArrowWidth=2;
	this.maxArrowWidth=5;
	this.featureHeight=30;
	this.spaceYbetweenFeatures=4;
	this.hersheyFont=new Hershey();
	this.printBases=false;
	this.printName=false;
	this.WIDTH=1000;
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
		
	this.strokeColorizer={
		 getColor:function(rec)
			{
			    if(!rec.getReadPairedFlag() || rec.getProperPairFlag()) return Color.BLACK;
			    if(rec.getMateUnmappedFlag()) return Color.BLUE;
			    if(rec.getDuplicateReadFlag()) return Color.GREEN;
			    return Color.ORANGE;
			}
		};
    	}
    
   

	
Bam2Raster.this.build=function(records)
		{
		var iter,rows=[];
		var countReads=0;
		for(iter=0;iter<records.length;++iter)
			{
			var rec=records[iter];
			if(rec.getReadUnmappedFlag()) continue;
			
			if(this.interval.chrom!= rec.getReferenceName()) continue;
			if(rec.getAlignmentEnd() < this.interval.start ) continue;
			if(rec.getAlignmentStart() > this.interval.end ) continue;
			countReads++;			
			var y=0;
			for(y=0;y< rows.length;++y)
				{
				var  row=rows[y];
				var last=row.get(row.size()-1);
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
		
	
		
		var int ruler_height=""+this.interval.getEnd()).length()*20;
		var refw=(int)Math.max(1.0, this.WIDTH/(double)(1+this.interval.getEnd()-this.interval.getStart()));
		var margin_top=10+(refw*2)+ruler_height;
		var imageSize=	{
				"width":this.WIDTH,
				"height": margin_top+ rows.length*(this.spaceYbetweenFeatures+this.featureHeight)+this.spaceYbetweenFeatures
				};
		
		
		
		g.setColor(Color.WHITE);
		g.fillRect(0, 0, imageSize.width, imageSize.height);
		
		var ref2consensus={};
		//draw bases positions
		
		for(var x=this.interval.start;
			x<=this.interval.end;
			++x)
			{
			var oneBaseWidth=convertToX(x+1)-convertToX(x);
			//draw vertical line
			g.setColor(x%10==0?Color.BLACK:Color.LIGHT_GRAY);
			g.draw(new Line2D.Double(convertToX(x), 0, convertToX(x), imageSize.height));
			
			if((x-this.interval.getStart())%10==0)
				{
				g.setColor(Color.BLACK);
				var xStr=String.format("%,d",x);
				AffineTransform tr=g.getTransform();
				AffineTransform tr2=new AffineTransform(tr);
				tr2.translate(convertToX(x), 0);
				tr2.rotate(Math.PI/2.0);
				g.setTransform(tr2);
				this.hersheyFont.paint(g,
						xStr,
						0,
						0,
						ruler_height,
						oneBaseWidth
						);
				g.setTransform(tr);
				}
			
			//paint genomic sequence
			var c=this.genomicSequence.charAt(x-1);
			g.setColor(base2color(c));
			hersheyFont.paint(g,
					String.valueOf(c),
					convertToX(x)+1,
					ruler_height,
					oneBaseWidth-2,
					oneBaseWidth-2
					);
				
			}
		
		
		var y=margin_top+this.spaceYbetweenFeatures;
		for(var rowY in rows)
			{
			var row=rows[rowY];
			for(SAMRecord recIdx in rows[rowY])
				{
				
				double x0=left(rec);
				double x1=right(rec);
				double y0=y;
				double y1=y0+this.featureHeight;
				Shape shapeRec=null;
				if(x1-x0 < minArrowWidth)
					{
					shapeRec=new Rectangle2D.Double(x0, y0, x1-x0, y1-y0);
					}
				else
					{
					GeneralPath path=new GeneralPath();
					double arrow=Math.max(this.minArrowWidth,Math.min(this.maxArrowWidth, x1-x0));
					if(!rec.getReadNegativeStrandFlag())
						{
						path.moveTo(x0, y0);
						path.lineTo(x1-arrow,y0);
						path.lineTo(x1,(y0+y1)/2);
						path.lineTo(x1-arrow,y1);
						path.lineTo(x0,y1);
						}
					else
						{
						path.moveTo(x0+arrow, y0);
						path.lineTo(x0,(y0+y1)/2);
						path.lineTo(x0+arrow,y1);
						path.lineTo(x1,y1);
						path.lineTo(x1,y0);
						}
					path.closePath();
					shapeRec=path;
					}
				
				Stroke oldStroke=g.getStroke();
				g.setStroke(new BasicStroke(2f));
				
				Paint oldpaint=g.getPaint();
				LinearGradientPaint gradient=new LinearGradientPaint(
						0f, (float)shapeRec.getBounds2D().getY(),
						0f, (float)shapeRec.getBounds2D().getMaxY(),
						new float[]{0f,0.5f,1f},
						new Color[]{Color.DARK_GRAY,Color.WHITE,Color.DARK_GRAY}
						);
				g.setPaint(gradient);
				g.fill(shapeRec);
				g.setPaint(oldpaint);
				g.setColor(this.strokeColorizer.getColor(rec));
				g.draw(shapeRec);
				g.setStroke(oldStroke);
				
				Shape oldClip=g.getClip();
				g.setClip(shapeRec);
				
				
				Cigar cigar=rec.getCigar();
				if(cigar!=null)
					{
					byte bases[]=rec.getReadBases();
					int refpos=rec.getAlignmentStart();
					int readpos=0;
					for(CigarElement ce:cigar.getCigarElements())
						{
						switch(ce.getOperator())
							{
							case H: break;
							case S: readpos+=ce.getLength();break;
							case I:
								{
								g.setColor(Color.GREEN); 
								g.fill(new Rectangle2D.Double(
											convertToX(refpos),
											y0,
											2,
											y1-y0
											));
								readpos+=ce.getLength();
									
								
								break;
								}
							case D:
							case N:
							case P:
								{
								g.setColor(Color.ORANGE); 
								g.fill(new Rectangle2D.Double(
											convertToX(refpos),
											y0,
											convertToX(refpos+ce.getLength())-convertToX(refpos),
											y1-y0
											));
								
								refpos+=ce.getLength();
								break;
								}
							case EQ:
							case X:
							case M:
								{
								for(int i=0;i< ce.getLength();++i)
									{
									if(readpos>=bases.length)
										{
										System.err.println(rec.getReadName()+" "+rec.getCigarString()+" "+rec.getReadString());
										}
									
									char c1=(char)bases[readpos];
									
									/* handle consensus */
									Counter<Character> consensus=ref2consensus.get(refpos);
									if(consensus==null)
										{
										consensus=new 	Counter<Character>();
										ref2consensus.put(refpos,consensus);
										}
									consensus.incr(Character.toUpperCase(c1));
									
									
									char c2=genomicSequence.charAt(refpos-1);
									
									double mutW=convertToX(refpos+1)-convertToX(refpos);
									g.setColor(Color.BLACK);
									Shape mut= new Rectangle2D.Double(
											convertToX(refpos),
											y0,
											mutW,
											y1-y0
											);
									if(ce.getOperator()==CigarOperator.X ||
										(c2!='N' && c2!='n' && Character.toUpperCase(c1)!=Character.toUpperCase(c2)))
										{
										g.setColor(Color.RED);
										g.fill(mut);
										g.setColor(Color.WHITE);
										}
									
									//print read name instead of base
									if(printName)
										{
										
										if(readpos<rec.getReadName().length())
											{
											c1=rec.getReadName().charAt(readpos);
											c1=rec.getReadNegativeStrandFlag()?
													Character.toLowerCase(c1):Character.toUpperCase(c1);
											}
										else
											{
											c1=' ';
											}
										}
									this.hersheyFont.paint(g,String.valueOf(c1),mut);
									
									readpos++;
									refpos++;
									}
								break;
								}
							default: error("cigar element not handled:"+ce.getOperator());break;
							}
						}
					}
				
				
				
				g.setClip(oldClip);
				}
			y+=this.featureHeight+this.spaceYbetweenFeatures;
			}
		
		//print consensus
		for(var x=this.interval.getStart();x<=this.interval.getEnd() ;++x)
			{
			Counter<Character> cons=ref2consensus.get(x);
			if(cons==null || cons.getCountCategories()==0)
				{
				continue;
				}
			var oneBaseWidth=(this.convertToX(x+1)-this.convertToX(x))-1;

			var x0= this.convertToX(x)+1;
			for(Character c:cons.keySetDecreasing())
				{
				
				var weight=oneBaseWidth*(cons.count(c)/(double)cons.getTotal());
				g.setColor(Color.BLACK);
				
				if(this.genomicSequence!=null &&
					Character.toUpperCase(genomicSequence.charAt(x-1))!=Character.toUpperCase(c))
					{
					g.setColor(Color.RED);
					}
					
			
				this.hersheyFont.paint(g,
						String.valueOf(c),
						x0,
						ruler_height+refw,
						weight,
						oneBaseWidth-2
						);
				x0+=weight;
				}
				
			}
		
		
		g.dispose();
		return img;
		}
	

