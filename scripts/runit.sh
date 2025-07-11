set -e
for a in afl_out/crashes/*.sp 
do 
    echo $a
    ./sp $a
done 
