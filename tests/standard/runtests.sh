#/bin/bash

SP=$1
ret_error=0
rm -f ../error.tmp

for name in *.sp; do
    echo "Testing:" $name
    fpc -O2 -Mdelphi $name -o/tmp/fpc.out >/dev/null 2>&1 
    if [ $? -ne 0 ]; then
        echo "Compilation of $name with FPC failed"
        exit 1
    fi
    
    infile=${name%.*}.in
    if [ ! -f $infile ]; then
        infile=/dev/null
    fi
    
    /tmp/fpc.out <$infile >/tmp/out.fpc
    $SP $name <$infile >/tmp/out.sp
    
    diff /tmp/out.fpc /tmp/out.sp 
    if [ $? -ne 0 ]; then
        echo "Regression of $name with FPC failed"
        ret_error=1
        touch ../error.tmp
    fi 
done
exit $ret_error

