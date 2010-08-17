#! /bin/sh

if [ $# -lt 3 ]
then
echo '  Usage: shotmatch5.sh nmax a.mes b.mes ...'
exit 1
fi

# usage shotmatch5.sh a.mes b.mes .....

# parse options
nmax=`expr $1`
if [ $nmax -lt 1 ]
then
echo nmax=$nmax is too small
exit 1
fi

# nmin=3 # for MCCD2
nmin=4 # for Suprime

shift 
files=$*
## echo $files

if [ -z "$files" ]
then
echo no catalogs
exit 1
fi

MATRIX=test.dat
MATCHSINGLE=match_single5
MATCHSTACK=match_stack5

#### The binary path check works only on Linux ??

## binary path check
#for binary in $MATCHSINGLE $MATCHSTACK awk 
#do
#if which $binary > /dev/null ; 
#then 
#:
#else
#echo $binary is not found in your path. stop ; 
#exit 2 ;
#fi
#done

## not needed any longer
## ls -1 $files | awk '{print $1,NR-1;}' > tmp.lis
## num=`wc -l tmp.lis | awk '{print $1}'`

rm -f $MATRIX

for a in $files
do
for b in $files
do
if [ "$a" != "$b" ]
then
# should be quiet mode and tail is not needed
$MATCHSINGLE -nmax=$nmax -nmin=$nmin $a $b >> $MATRIX
echo '#' `tail -1 $MATRIX`
fi
done
done
echo
echo
$MATCHSTACK $MATRIX 
