require 'fileutils'

module Montage

  @@original_workflow = false

  def original_workflow=(cond)
    @@original_workflow=cond
  end

  module_function :'original_workflow='


  def read_overlap_tbl(table)
    result = []
    File.open(table) do |r|
      2.times{r.gets}
      while l=r.gets
        result << l.split
      end
    end
    result
  end

  module_function :read_overlap_tbl


  def write_fitfits_tbl(results, file)
    open(file,"w") do |f|
      f.puts '| plus|minus|       a    |      b     |      c     | crpix1  | crpix2  | xmin | xmax | ymin | ymax | xcenter | ycenter |  npixel |    rms     |    boxx    |    boxy    |  boxwidth  | boxheight  |   boxang   |'
      n = "([0-9.e-]+)"
      results.each do |a|
        name, r = a
        r = r[0] if r.kind_of?(Array)
        case name
        when Array
          idx = name
        when /\.(\d+)\.(\d+)\./
          idx = $1,$2
        else
          puts "unmach1 : #{name}"
          raise
        end
        if /a=#{n}, b=#{n}, c=#{n}, crpix1=#{n}, crpix2=#{n}, xmin=#{n}, xmax=#{n}, ymin=#{n}, ymax=#{n}, xcenter=#{n}, ycenter=#{n}, npixel=#{n}, rms=#{n}, boxx=#{n}, boxy=#{n}, boxwidth=#{n}, boxheight=#{n}, boxang=#{n}/ =~ r
          args = (idx+Regexp.last_match[1..-1]).map{|x| x.to_f}
          f.puts " %5d %5d %12.5e %12.5e %12.5e %9.2f %9.2f %6d %6d %6d %6d %9.2f %9.2f %9.0f %12.5e %12.1f %12.1f %12.1f %12.1f %12.1f" % args
        else
          puts "unmach2 : #{r}"
        end
      end
    end
  end

  module_function :write_fitfits_tbl


  def write_fits_tbl(results, fittxt_file, fits_file)
    if @@original_workflow
      sh "mConcatFit #{fittxt_file} #{fits_file} d"
    else
      write_fitfits_tbl(results, fits_file)
    end
  end

  module_function :write_fits_tbl


  def collect_imgtbl( t, ary )
    if ! @@original_workflow
      if t.kind_of? Rake::Task
        t = t.name
      end
      q = `mImgHdr #{t}`
      if q
        ary << q+" "+File.basename(t)
      else
        puts "IMGTBL fail"
      end
    end
  end
  module_function :collect_imgtbl

  def collect_imgtbl_fullname( t, ary )
    if ! @@original_workflow
      if t.kind_of? Rake::Task
        t = t.name
      end
      q = `mImgHdr #{t}`
      if q
        ary << q+" "+t
      else
        puts "IMGTBL fail"
      end
    end
  end
  module_function :collect_imgtbl_fullname


  def write_imgtbl(imgtbl, tblfile)
    maxlen = 0
    imgtbl.each{|x| maxlen=x.size if x.size>maxlen}
    nspc = maxlen-243
    nspc = 0 if nspc<0
    puts "Montage.write_imgtbl: writing to #{tblfile}..."
    open(tblfile,"w") do |f|
      f.puts '\datatype = fitshdr'
      f.puts "| cntr |      ra     |     dec     |      cra     |     cdec     |naxis1|naxis2| ctype1 | ctype2 |     crpix1    |     crpix2    |    crval1   |    crval2   |      cdelt1     |      cdelt2     |   crota2    |equinox |    size    | hdu  | fname"+" "*nspc+"|"
      f.puts "| int  |    double   |    double   |      char    |    char      | int  | int  |  char  |  char  |     double    |     double    |    double   |    double   |      double     |      double     |   double    |  double|     int    | int  | char "+" "*nspc+"|"
      imgtbl.each_with_index{|x,i| f.puts " %6d%s"%[i,x[7..-1]]}
    end
  end

  module_function :write_imgtbl


  def put_imgtbl( ary, dir, tblname )
    if @@original_workflow
      sh "mImgtbl #{dir} #{tblname}"
    elsif !ary.empty?
      write_imgtbl(ary, tblname)
    end
  end

  module_function :put_imgtbl


  def write_fittxt_tbl( file, dif_tbl )
    #file = "fittxt.tbl"
    if @@original_workflow
      File.open(file,"w") do |w|
        w.puts "| cntr1 | cntr2 |       stat            |"
        w.puts "|  int  |  int  |       char            |"
        dif_tbl.each do |c|
          fn = c[4].sub(/^diff\.(.*)\.fits$/,'fit.\1.txt')
          w.printf " %7d %7d %21s \n",c[0],c[1],fn
        end
      end
    end
  end

  module_function :write_fittxt_tbl


  def fitplane( files, task, tbl )
    if @@original_workflow
      fn = task.name.sub(/diff\.(.*)\.fits$/,'fit.\1.txt')
      sh "mFitplane -s #{fn} #{task.name}"
    else
      tbl << [files,`mFitplane #{task.name}`]
    end
  end

  module_function :fitplane

end
