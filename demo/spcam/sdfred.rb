def mkhist( data, min, max, width )

  if width==0
    raise "ERROR: width is 0."
  end  

  nbin_1 = (max-min)/width ;
  nbin = nbin_1.to_i + 1;
  
  upperv = min - 0.5 * width;

  (0...nbin).map do |i|
    #if 0
    #lowerv=min+((float)i - 0.5)*width;
    #upperv=min+((float)i + 0.5)*width;
    #endif
    lowerv = upperv
    upperv = lowerv + width

    hist = 0
    # /* center=(lowerv+upperv)/2.0; */
    center = 0.5*(lowerv+upperv)

    data.each do |d|
      if d >= lowerv && d < upperv
        hist += 1
      end
    end

    [center,hist]
  end
end


def histmax( data, min, max, width )
  hist = mkhist( data, min, max, width )
  x_pos, y_max = hist[0]
  hist.each do |x,y|
    if y > y_max
      x_pos = x
      y_max = y
    end
  end
  x_pos
end


def select_obj( file, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )
  count = 0
  tmp2 = []
  puts "select_obj : reading: #{file}"
  IO.popen( "sort -k 7n #{file}" ) do |f|
    while s = f.gets
      a = s.split.map{|x| x.to_f}
      if a[3]>0 && a[7]>min_flux && a[7]<max_flux && a[6]>min_fwhm && a[6]<max_fwhm
        tmp2 << a[6]
        count += 1
        break if count >= number_of_obj
      end
    end
  end
  tmp2
end


def make_a_histogram(data, wid, file)
  hist_min = data.min
  hist_max = data.max
  data3 = mkhist( data, hist_min, hist_max, wid )

  c = 0.0
  d = 0.0
  f = file
  f.puts "\nPSF | number of images"
  data3.each do |psf,num|
    f.print "#{psf} |"
    num.times{f.print "*"}
    f.puts
    c += num
    d += psf*num
  end
  d / c
end

# step 11
# shotmatch7a.sh
MATCHSINGLE="match_single5"

def shotmatch( ames, bmes, nmin, nmax, out )
  aimg = File.basename(ames,".mes")
  bimg = File.basename(bmes,".mes")
  amessel = "a"+out
  bmessel = "b"+out

  #select B
  est = `estmatch #{aimg} #{bimg}`
  x0,y0,r,ff = est.split.map{|v| v.to_f}
  aX=`getkey NAXIS1 #{aimg}`.to_i
  aY=`getkey NAXIS2 #{aimg}`.to_i
  s = Math.sin(r)
  c = Math.cos(r)
  open(bmessel,"w") do |w|
    open(bmes) do |f|
      while l = f.gets
        a = l.split.map{|v| v.to_f}
        x = c*a[1] - s*a[2] + x0
        y = s*a[1] + c*a[2] + y0
        if 0<x && x<aX && 0<y && y<aY
          w.print l
        end
      end
    end
  end

  #select A
  est = `estmatch #{bimg} #{aimg}`
  x0,y0,r,ff = est.split.map{|v| v.to_f}
  bX=`getkey NAXIS1 #{bimg}` .to_i
  bY=`getkey NAXIS2 #{bimg}`.to_i
  s = Math.sin(r)
  c = Math.cos(r)
  open(amessel,"w") do |w|
    open(ames) do |f|
      while l = f.gets
        a = l.split.map{|v| v.to_f}
        x = c*a[1] - s*a[2] + x0;
        y = s*a[1] + c*a[2] + y0;
        if 0<x && x<bX && 0<y && y<bY
          w.print l
        end
      end
    end
  end

  sh "#{MATCHSINGLE} -nmax=#{nmax} -nmin=#{nmin} #{amessel} #{bmessel} | ruby -e \"print ARGF.read.sub('#{amessel}','#{aimg}').sub('#{bmessel}','#{bimg}')\" > #{out}"
  sh "rm #{amessel} #{bmessel}"

end

