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
  #p [file, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj]
  IO.popen( "sort -k 7n #{file}" ) do |f|
    while s = f.gets
      a = s.split.map{|x| x.to_f}
      #p a
      # $AWK '$4>0 && $8>'${min_flux}' && $8<'${max_flux}' && $7>'${min_fwhm}' && $7<'${max_fwhm}' {print $0}' tmp1 | head -${number_of_obj} > psfmatchout.lis
      if a[3]>0 && a[7]>min_flux && a[7]<max_flux && a[6]>min_fwhm && a[6]<max_fwhm
      # $AWK '{print $7}' psfmatchout.lis > tmp2
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


# step 6
# psfmatch.csh

=begin
def psfmatch( image_in, number_of_obj, min_flux, max_flux, min_fwhm, max_fwhm, result_fwhm, image_out )

  if !File.file?(image_in)
    raise "Error in psfmatch: file '#{image_in}' not found"
  end

  image_cat = File.basename(image_in,".fits")+".cat"
  image_chk = "check_" + image_in

#  tmp2 = select_obj( image_cat, MF_min_flux, MF_max_flux,
#                     MF_min_fwhm, MF_max_fwhm, MF_number_of_obj )
#  fwhmpsf = histmax( tmp2, MF_min_fwhm, MF_max_fwhm, 0.1 )

# measure fwhm_s
  sh "sex #{image_in} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"
  tmp2 = select_obj( image_cat, min_flux, max_flux,
                     min_fwhm, max_fwhm, number_of_obj )
  fwhmpsf = histmax( tmp2, min_fwhm, max_fwhm, 0.1 )

  #set the initial fwhm
  fwhm_f = fwhmpsf
  fwhm_s = fwhmpsf

  #calculate the difference between input and resulting FWHM_PSF
  puts "fwhm_s: #{fwhm_s}"
  diff_fwhm = result_fwhm - fwhm_s

  if diff_fwhm >= 0.1

    #measure fwhm_c
    gaussian_sigma_c = Math.sqrt(result_fwhm**2 - fwhm_s**2) / 2.35482
    
    sh "smth2 #{image_in} #{gaussian_sigma_c} #{image_out}"
    sh "sex #{image_out} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"

    tmp2 = select_obj( image_cat, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )
    fwhmpsf = histmax( tmp2, min_fwhm, max_fwhm, 0.1 )

    fwhm_c = fwhmpsf
    puts "fwhm_c=#{fwhm_c}"

    #calculate the difference between input and resulting FWHM_PSF
    diff_fwhm = result_fwhm - fwhm_c

    if diff_fwhm<0.1 && diff_fwhm>-0.1
      puts "ok"
      fwhm_f = fwhmpsf
      return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2 )
    end

    gaussian_sigma_u = gaussian_sigma_c * 2.0
    sh "smth2 #{image_in} #{gaussian_sigma_u} #{image_out}"
    sh "sex #{image_out} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"

    tmp2 = select_obj( image_cat, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )

    if tmp2.size < 5
      puts "estimate gaussian_sigma_u again"
      gaussian_sigma_u = gaussian_sigma_c * 1.5
      sh "smth2 #{image_in} #{gaussian_sigma_u} #{image_out}"
      sh "sex #{image_out} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"
      tmp2 = select_obj( image_cat, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )
    end
    #---

    fwhmpsf = histmax( tmp2, min_fwhm, max_fwhm, 0.1 )

    fwhm_u = fwhmpsf

    #calculate the difference between input and resulting FWHM_PSF
    puts "fwhm_u=#{fwhm_u}"
    diff_fwhm = result_fwhm - fwhm_u
    if diff_fwhm<0.1 && diff_fwhm>-0.1
      fwhm_f = fwhmpsf
      return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2 )
    end

    puts "fwhm_s=#{fwhm_s}"
    puts "fwhm_c=#{fwhm_c} gaussian_sigma_c=#{gaussian_sigma_c}"
    puts "fwhm_u=#{fwhm_u} gaussian_sigma_u=#{gaussian_sigma_u}"

    #determine fwhm1 and fwhm2
    if result_fwhm > fwhm_s && result_fwhm <= fwhm_c
      fwhm1 = fwhm_s
      fwhm2 = fwhm_c
      gaussian_sigma1 = gaussian_sigma_c / 1.5
      gaussian_sigma2 = gaussian_sigma_c
    end

    if result_fwhm <= fwhm_u && result_fwhm > fwhm_c
      fwhm1 = fwhm_c
      fwhm2 = fwhm_u
      gaussian_sigma1 = gaussian_sigma_c
      gaussian_sigma2 = gaussian_sigma_u
    end

    puts "fwhm1=#{fwhm1} gaussian_sigma1=#{gaussian_sigma1} "
    puts "fwhm2=#{fwhm2} gaussian_sigma2=#{gaussian_sigma2} "

    if !fwhm1 || !fwhm2 || !gaussian_sigma1 || !gaussian_sigma2
      raise "Error : failed to set parameter" 
    end

    #while diff_fwhm is larger than 0.1
    #calculate the difference between input and resulting FWHM_PSF
    puts "fwhm_s=#{fwhm_s}"
    diff_fwhm = result_fwhm - fwhm_s

    while (diff_fwhm>=0.1 || diff_fwhm<=-0.1) && gaussian_sigma2-gaussian_sigma1>=0.01

      puts "diff_fwhm=#{diff_fwhm}"
      puts "fwhm1=#{fwhm1} gaussian_sigma1=#{gaussian_sigma1} "
      puts "fwhm2=#{fwhm2} gaussian_sigma2=#{gaussian_sigma2} "

      gaussian_sigma_c = (gaussian_sigma1+gaussian_sigma2)/2.0

      puts "gaussian_sigma_c=#{gaussian_sigma_c}"

      #smooth the image
      status = nil
      sh "smth2 #{image_in} #{gaussian_sigma_c} #{image_out}" do |res,s|
        status = s.exitstatus
      end
      status_smth2 = status
      puts "status=#{status.inspect}"
      #in case if no smoothing is done
      if status_smth2 == 1
        cp image_in, image_out
        sh "sex #{image_out} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"
        tmp2 = select_obj( image_cat, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )
        fwhmpsf = histmax( tmp2, min_fwhm, max_fwhm, 0.1 )

        fwhm_f = fwhmpsf
        return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2 )
      end

      #measure FWHM_PSF
      sh "sex #{image_out} -c #{SDFREDSH}/psfmatch/psfmatch.sex -CATALOG_NAME #{image_cat} -CHECKIMAGE_NAME #{image_chk}> /dev/null"

      tmp2 = select_obj( image_cat, min_flux, max_flux, min_fwhm, max_fwhm, number_of_obj )

      fwhmpsf = histmax( tmp2, min_fwhm, max_fwhm, 0.1 )

      fwhm_c = fwhmpsf
      fwhm_f = fwhmpsf

      #calculate the difference between input and resulting FWHM_PSF
      puts "fwhm_c = #{fwhm_c}"
      diff_fwhm = result_fwhm - fwhm_c

      #determine fwhm1 and fwhm2 for the next smoothing
      if result_fwhm>fwhm1 && result_fwhm <= fwhm_c
        fwhm1 = fwhm1
        fwhm2 = fwhm_c
        gaussian_sigma1 = gaussian_sigma1
        gaussian_sigma2 = gaussian_sigma_c
      end
      if result_fwhm < fwhm2 && result_fwhm > fwhm_c
        fwhm1 = fwhm_c
        fwhm2 = fwhm2
        gaussian_sigma1 = gaussian_sigma_c
        gaussian_sigma2 = gaussian_sigma2
      end
    end

  else
    puts "seeing of #{image_in} is #{fwhm_s}, which is worse than #{result_fwhm} "
    puts "cp #{image_in} #{image_out}"
    cp image_in, image_out
  end

  return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2 )
end


def no_smooth( image, fwhmpsf, fwhm_f, tmp2 )
  upper_fwhmpsf = fwhm_f + 0.2
  lower_fwhmpsf = fwhm_f - 0.2
  hist = mkhist( tmp2, lower_fwhmpsf, upper_fwhmpsf, 0.1 ).map{|x| x[1]}
  result = [image, fwhmpsf] + hist
  puts "no_smooth : #{result.inspect}"
  return result
end

=end


# step 11
# shotmatch7a.sh
MATCHSINGLE="match_single5"

def shotmatch( ames, bmes, nmin, nmax, out )
  aimg = File.basename(ames,".mes")
  bimg = File.basename(bmes,".mes")
  #amessel = aimg+'.messel'
  #bmessel = bimg+'.messel'
  amessel = "a"+out
  bmessel = "b"+out

  #select B
  #puts "estmatch #{aimg} #{bimg}"
  est = `estmatch #{aimg} #{bimg}`
  x0,y0,r,ff = est.split.map{|v| v.to_f}
  #puts "x0=#{x0},y0=#{y0},r=#{r},ff=#{ff}"
  aX=`getkey NAXIS1 #{aimg}`.to_i
  aY=`getkey NAXIS2 #{aimg}`.to_i
  #puts "aX=#{aX} aY=#{aY}"
  s = Math.sin(r)
  c = Math.cos(r)
  open(bmessel,"w") do |w|
    open(bmes) do |f|
      while l = f.gets
        a = l.split.map{|v| v.to_f}
        #p a
        x = c*a[1] - s*a[2] + x0
        y = s*a[1] + c*a[2] + y0
        #p "x=#{x} y=#{y}"
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
  #puts "aX=#{aX} aY=#{aY}"
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

  #sh "#{MATCHSINGLE} -nmax=#{nmax} -nmin=#{nmin} #{amessel} #{bmessel} > #{out}"
  sh "#{MATCHSINGLE} -nmax=#{nmax} -nmin=#{nmin} #{amessel} #{bmessel} | ruby -e \"print ARGF.read.sub('#{amessel}','#{aimg}').sub('#{bmessel}','#{bimg}')\" > #{out}"
  sh "rm #{amessel} #{bmessel}"

  #s = IO.read(out).sub(amessel,aimg).sub(bmessel,bimg)
  #puts s
  #open(out,"w"){|f| f.write s}
end


# def shotmatch( ames, bmes, nmin, nmax, out )
#   aimg = File.basename(ames,".mes")
#   bimg = File.basename(bmes,".mes")
#   amessel = aimg+'.messel'
#   bmessel = bimg+'.messel'
# 
#   sh "overlap2 #{aimg} #{bimg}" do |res,status|
#     if status.to_i == 0
#       #select B
#       est = `estmatch #{aimg} #{bimg}`
#       x0,y0,r,ff = est.split.map{|v| v.to_f}
#       aX=`getkey NAXIS1 #{aimg}`.to_i
#       aY=`getkey NAXIS2 #{aimg}`.to_i
#       s = Math.sin(r)
#       c = Math.cos(r)
#       open(bmessel,"w") do |w|
#         open(bmes) do |f|
#           while l = f.gets
#             a = l.split.map{|v| v.to_f}
#             x = c*a[1] - s*a[2] + x0
#             y = s*a[1] + c*a[2] + y0
#             if 0<x && x<aX && 0<y && y<aY
#               w.print l
#             end
#           end
#         end
#       end
# 
#       #select A
#       est = `estmatch #{bimg} #{aimg}`
#       x0,y0,r,ff = est.split.map{|v| v.to_f}
#       bX=`getkey NAXIS1 #{bimg}` .to_i
#       bY=`getkey NAXIS2 #{bimg}`.to_i
#       s = Math.sin(r)
#       c = Math.cos(r)
#       open(amessel,"w") do |w|
#         open(ames) do |f|
#           while l = f.gets
#             a = l.split.map{|v| v.to_f}
#             x = c*a[1] - s*a[2] + x0;
#             y = s*a[1] + c*a[2] + y0;
#             if 0<x && x<bX && 0<y && y<bY
#               w.print l
#             end
#           end
#         end
#       end
#       
#       sh "#{MATCHSINGLE} -nmax=#{nmax} -nmin=#{nmin} #{amessel} #{bmessel} > #{out}"
#     else
#       sh "touch #{out}"
#     end
#   end
# end
