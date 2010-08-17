#!/home/tanakams/local/bin/ruby
# step 6
# psfmatch.csh


def psfmatch( image_in, number_of_obj, min_flux, max_flux, min_fwhm, max_fwhm, result_fwhm, image_out, logfile )

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
      return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2, logfile )
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
      return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2, logfile )
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
        return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2, logfile )
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

  return no_smooth( image_out, fwhmpsf, fwhm_f, tmp2, logfile )
end


def no_smooth( image, fwhmpsf, fwhm_f, tmp2, logfile )
  upper_fwhmpsf = fwhm_f + 0.2
  lower_fwhmpsf = fwhm_f - 0.2
  hist = mkhist( tmp2, lower_fwhmpsf, upper_fwhmpsf, 0.1 ).map{|x| x[1]}
  result = [image, fwhmpsf] + hist
  puts "no_smooth : #{result.inspect}"

  open(logfile,"w"){|f| f.puts result.join(' ')}
  puts "wrote result to #{logfile}"
  return result
end



if __FILE__ == $0
  require "fileutils"
  require "sdfred.rb"
  include FileUtils

  def sh(*cmd)
    raise if !system(*cmd)
  end

  SDFREDSH="/home/tanakams/local/sdfred20080610/sdfredSH"
  image_in = ARGV[0]
  number_of_obj = ARGV[1].to_i
  min_flux = ARGV[2].to_f
  max_flux = ARGV[3].to_f
  min_fwhm = ARGV[4].to_f
  max_fwhm = ARGV[5].to_f
  result_fwhm = ARGV[6].to_f
  image_out = ARGV[7]
  logfile = ARGV[8]
  psfmatch( image_in, number_of_obj, 
            min_flux, max_flux, min_fwhm, max_fwhm, result_fwhm, 
            image_out, logfile )
end
