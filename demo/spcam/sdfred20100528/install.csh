#! /bin/csh -f
#csh script for installing the softwares(spcamfirst) in sdfred/bin
#------------------------------------------------------
#
#       Created by Masami Ouchi
#               April 23, 2002
#
#------------------------------------------------------



#aquire a name of the directory for installation

if ( $#argv != 2 ) then
    echo "Usage:" $0 " [neko dir (ex. /home/kashikawa/bin/mosaic)][sdfred path(ex. /home/kashikawa/bin/sdfred)]" 
    exit 1
endif

if ( !(-r $1) ) then
    echo $0 " : Cannot open "$1": No such directory"
    exit 1
endif
if ( !(-r $2) ) then
    echo $0 " : Cannot open "$2": No such directory"
    exit 1
endif

set dir_neko = $1
set dir_sdfred = $2

set dir_neko_imp = `echo $dir_neko | sed s/"\/"/"\\\/"/g`

#if ( 0 ) then

#make Makefile
sed s/USER_DIR_NEKO/"${dir_neko_imp}"/ ${dir_sdfred}/spcamfirst/Makefile.in > ${dir_sdfred}/spcamfirst/Makefile

#compile softwares in spcamfirst
cd spcamfirst
make clean
make all
cd ../

#endif

#install binaries in bin/
cd bin
\ls -1 -F ../spcamfirst/ | grep "*" | sed s/"*"// | gawk '{print "rm -f "$1}' | csh -f

\ls -1 -F ../spcamfirst/ | grep "*" | sed s/"*"// | gawk '{print "ln -s ../spcamfirst/"$1}' | csh -f

cd ../



#change the name of directory for *.csh, *.awk and *.sex

set dir_default = /home/ouchi/bin
set dir_default_imp = `echo $dir_default | sed s/"\/"/"\\\/"/g`
set dir_sdfred_imp = `echo $dir_sdfred | sed s/"\/"/"\\\/"/g`

#for *.csh
foreach name ( overscansub mask mask_mkflat_HA mask_mkflat_A distcorr fwhmpsf fwhmpsf_batch  psfmatch psfmatch_batch blank skysb skysb_reg starselect limitmag mask_AGX namechange makemos assumemos makemos_adv makemos_adv_sep ffield spcamred meanchip meanobs comb_meanchip_meanobs makelog )

    sed s/"${dir_default_imp}"/"${dir_sdfred_imp}"/g ${dir_sdfred}/spcamfirstSH/${name}/${name}.csh.in > ${dir_sdfred}/spcamfirstSH/${name}/${name}.csh
    chmod 755 ${dir_sdfred}/spcamfirstSH/${name}/${name}.csh
    cd bin
    rm -f ${name}.csh
    ln -s ${dir_sdfred}/spcamfirstSH/${name}/${name}.csh
    cd ../

end

#for *.awk
foreach name ( overscansub )
    sed s/"${dir_default_imp}"/"${dir_sdfred_imp}"/g ${dir_sdfred}/spcamfirstSH/${name}/${name}.awk.in > ${dir_sdfred}/spcamfirstSH/${name}/${name}.awk
end

#for *.sex
foreach name ( mask_mkflat_HA fwhmpsf psfmatch skysb skysb_reg starselect limitmag )
    sed s/"${dir_default_imp}"/"${dir_sdfred_imp}"/g ${dir_sdfred}/spcamfirstSH/${name}/${name}.sex.in > ${dir_sdfred}/spcamfirstSH/${name}/${name}.sex
end

