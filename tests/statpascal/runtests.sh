#/bin/bash

SP=$1
ret_error=0

echo "Compiling C/C++ plugins..."
mkdir -p /tmp/sp-so
for name in *.cpp; do
    outfile=/tmp/sp-so/${name%.*}.so
    gcc -O2 -fPIC -I../../src -shared -o $outfile $name
done

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/tmp/sp-so

for name in *.sp; do
    echo "Testing:" $name
    $SP $name >/tmp/out.sp
    cmpfile=${name%.*}.out
    diff /tmp/out.sp $cmpfile
    if [ $? -ne 0 ]; then
        echo "Regression of $name failed"
        ret_error=1
        touch ../error.tmp
    fi 
done
exit $ret_error
