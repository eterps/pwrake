require 'fileutils'

module Montage
 
  def read_overlap_tbl(table)
    r = open(table,"r")
    2.times{r.gets}
    result = []
    while l=r.gets
      c = l.split
      c[2] = "p/"+c[2]
      c[3] = "p/"+c[3]
      result << c
    end
    r.close
    result
  end

  def read_image_tbl(table)
    r = open(table,"r")
    l = r.gets
    if /^\\/ =~ l
	l = r.gets
    end

    columns = []
    l.scan( /\|([^|]*)/ ){
	columns << [$1.strip, $~.offset(1)]
    }

    pos = /\|(\s*fname\s*)\|/ =~ l
    len = $1.size

    while /^\|/ =~ l
	l = r.gets
    end

    table = []
    while l
	table << row = {}
	columns.each do |name,ofs|
	  row[name] = l[ofs[0]..ofs[1]].strip
	end
	l = r.gets
    end
    r.close
    table
  end

  def write_imgtbl(imgtbl, tblfile)
    basename = File.basename(tblfile)
    dirname = File.dirname(tblfile)
    tmpname = "/tmp/"+basename
    maxlen = 0
    imgtbl.each{|x| maxlen=x.size if x.size>maxlen}
    nspc = maxlen-243
    nspc = 0 if nspc<0
    open(tmpname,"w") do |f|
      f.puts '\datatype = fitshdr'
      f.puts "| cntr |      ra     |     dec     |      cra     |     cdec     |naxis1|naxis2| ctype1 | ctype2 |     crpix1    |     crpix2    |    crval1   |    crval2   |      cdelt1     |      cdelt2     |   crota2    |equinox |    size    | hdu  | fname"+" "*nspc+"|"
      f.puts "| int  |    double   |    double   |      char    |    char      | int  | int  |  char  |  char  |     double    |     double    |    double   |    double   |      double     |      double     |   double    |  double|     int    | int  | char "+" "*nspc+"|"
      imgtbl.each_with_index{|x,i| f.puts " %6d%s"%[i,x[7..-1]]}
    end
    FileUtils.mv(tmpname, dirname)
  end
  module_function :write_imgtbl


=begin
  def read_image_tbl(table)
    r = open(table,"r")
    l = r.gets
    if /^\\/ =~ l
      l = r.gets
    end
    pos = /\|(\s*fname\s*)\|/ =~ l
    len = $1.size

    while /^\|/ =~ l
      l = r.gets
    end

    result = []
    while l
      result << File.basename(l[pos+1,len].strip)
      l = r.gets
    end
    r.close
    result
  end
=end

  def write_fittxt_tbl(table, diffs)
    w = open("fittxt.tbl","w")
    w.puts "| cntr1 | cntr2 |       stat            |"
    w.puts "|  int  |  int  |       char            |"
    diffs.each do |c|
      w.printf " %7d %7d %21s \n",c[0],c[1],c[4].sub(/^diff\.(.*)\.fits$/,'fit.\1.txt')
    end
    w.close
  end

  module_function :read_image_tbl, :read_overlap_tbl, :write_fittxt_tbl


  #[struct stat="OK", a=-7.16799e-08, b=1.23707e-07, c=0.951076, crpix1=-2131.5, crpix2=4183, xmin=2131.5, xmax=2635.5, ymin=-4183, ymax=-4126, xcenter=2383.75, ycenter=-4154.34, npixel=21774, rms=0.000326946, boxx=2383.51, boxy=-4154.25, boxwidth=504.141, boxheight=53.0168, boxang=0.553565]

  def write_fitfits_tbl(results, file)
    #open("fitfits.tbl","w") do |f|
    open(file,"w") do |f|
      f.puts '| plus|minus|       a    |      b     |      c     | crpix1  | crpix2  | xmin | xmax | ymin | ymax | xcenter | ycenter |  npixel |    rms     |    boxx    |    boxy    |  boxwidth  | boxheight  |   boxang   |'
      n = "([0-9.e-]+)"
      results.each do |a|
	name, r = a
	begin
          case name
          when Array
            idx = name
	  when /\.(\d+)\.(\d+)\./
            idx = $1,$2
          else
	    puts "unmach : #{name}" 
	    raise
	  end
	  if /a=#{n}, b=#{n}, c=#{n}, crpix1=#{n}, crpix2=#{n}, xmin=#{n}, xmax=#{n}, ymin=#{n}, ymax=#{n}, xcenter=#{n}, ycenter=#{n}, npixel=#{n}, rms=#{n}, boxx=#{n}, boxy=#{n}, boxwidth=#{n}, boxheight=#{n}, boxang=#{n}/ !~ r[0]
	    puts "unmach : #{r}" 
	    raise
	  end
	  args = (idx+Regexp.last_match[1..-1]).map{|x| x.to_f}
	  f.puts " %5d %5d %12.5e %12.5e %12.5e %9.2f %9.2f %6d %6d %6d %6d %9.2f %9.2f %9.0f %12.5e %12.1f %12.1f %12.1f %12.1f %12.1f" % args
	rescue
	  
	  puts "Error in Montage.write_fitfits_tbl"
	end
      end
    end
  end
  module_function :write_fitfits_tbl


  @@original_imgtbl = false

  def collect_imgtbl( t, ary )
    if ! @@original_imgtbl
      q = t.rsh "mImgHdr #{t.name}"
      if q && q[0]
        ary << q[0]+" "+File.basename(t.name)
      else
        puts "IMGTBL fail"
      end
    end
  end
  module_function :collect_imgtbl

  def put_imgtbl( ary, dir, tblname )
    if @@original_imgtbl
      sh "mImgtbl #{dir} #{tblname}"
    elsif !ary.empty?
      #puts "Montage.write_imgtbl(ary,'#{tblname}') ary.size=#{ary.size}"
      Montage.write_imgtbl(ary, tblname)
    end
  end
  module_function :put_imgtbl
end
