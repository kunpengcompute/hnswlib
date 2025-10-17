#!/bin/bash
ALG=$1
TEST=$2

function main() {
    # for data in glove-100-angular sift-128-euclidean deep-image-96-angular fashion-mnist-784-euclidean gist-960-euclidean; do
    for data in fashion-mnist-784-euclidean; do
        mkdir -p "output/${TEST}/${data}"
        for numa_node in 0 1 2 3; do
            numactl -N $numa_node -m $numa_node -- ./${TEST}_test ${ALG} ${data} > output/${TEST}/${data}/numa${numa_node}.txt &
        done
        wait
    done
}

main