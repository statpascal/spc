#/bin/bash

dir=afl_out/crashes

i=0
for a in ${dir}/id*
do
    let "i = $i + 1"
    count=$(printf "%04d" $i)
    mv $a ${dir}/crash${count}.sp
done
    
