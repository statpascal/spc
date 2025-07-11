#/bin/bash

currpath=$(pwd)
pushd ~
set -e
for a in $(find ~/temp -name *.pp); do
    echo $a
    ${currpath}/obj/sp --no-run $a
done
for a in $(find ~ -name *.pas); do
    echo $a
    ${currpath}/obj/sp --no-run $a
done
popd
