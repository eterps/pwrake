#! /bin/sh

# AIRMASS=airmass2.pl
# if you do not want to correct differential airmass,
# use following line instead
# AIRMASS=rotate.pl
AIRMASS=airmass2

if [ $# -ne 2 ]
then
echo "Usage: distcorr5.sh infile outfile"
exit
fi

file=$1
fileout=$2

### still test

case `getkey DATE-OBS $file` in
1999-0[1-6]*) ver=cas;;
1999*) ver=old;;
2000-0*) ver=old;;
2000-1*) ver=old2;;
2001-01*) ver=old2;;
2001-02*) ver=old3;;
2001-03*) ver=old4;;
2001*) ver=new;;
2002-0[1-7]*) ver=new;;
*) ver=new2;
esac

if [ "$ver" = "cas" ]
then
# no distortion correction needed.
exit 0
fi

if [ "$ver" = "new" ]
then
chip=`getkey DET-ID $file`;
case $chip in
0) x=-5310.051604; y=33.288532 ; t=0.001395;;
1) x=-3179.092082; y=40.946680 ; t=-0.000157;;
2) x=-1052.085930; y=-4084.168020 ; t=-0.000000;;
3) x=1073.302726; y=-4086.766838 ; t=-0.000000;;
4) x=1068.140361; y=47.513745 ; t=0.002169;;
5) x=-1018.567093; y=39.154287 ; t=0.000382;;
6) x=-5271.048187; y=-4084.741529 ; t=-0.000998;;
7) x=-3142.996541; y=-4087.737305 ; t=-0.000294;;
8) x=3235.998748; y=-4077.553770 ; t=0.001649;;
9) x=3195.190437; y=46.348975 ; t=0.000778;;
*)
esac
elif [ "$ver" = "new2" ]
then
## echo "NEW2"
chip=`getkey DET-ID $file`;
case $chip in
0) x=-5310.051604; y=23.288532 ; t=0.001395;;
1) x=-3179.092082; y=30.946680 ; t=-0.000157;;
2) x=-1052.085930; y=-4084.168020 ; t=-0.000000;;
3) x=1073.302726; y=-4086.766838 ; t=-0.000000;;
4) x=1068.140361; y=37.513745 ; t=0.002169;;
5) x=-1052.567093; y=29.154287 ; t=0.000382;;
6) x=-5305.048187; y=-4084.741529 ; t=-0.000998;;
7) x=-3176.996541; y=-4087.737305 ; t=-0.000294;;
8) x=3201.998748; y=-4077.553770 ; t=0.001649;;
9) x=3195.190437; y=36.348975 ; t=0.000778;;
*)
esac
else
## if [ "$ver" = "old" ]
## should be confirmed, someday...
x=`getkey CRPIX1 $file | awk '{printf("-1 * %f\n",$1)}' | bc -l`
y=`getkey CRPIX2 $file | awk '{printf("-1 * %f\n",$1)}' | bc -l`
t=0
fi

### Yagi fitted reverse parameter (C->I) to Miyazaki's-parameter(I->C)

distcorr5 -x=$x -y=$y -a2=7.16417e-08 -a3=3.03146e-10 -a4=5.69338e-14 -a5=-6.61572e-18 -a6=0 -a7=0 `$AIRMASS $file $t` $file $fileout
