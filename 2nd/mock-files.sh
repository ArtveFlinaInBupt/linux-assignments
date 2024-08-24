#! /bin/bash

rm -r test_dirs
mkdir test_dirs
cd test_dirs

for i in $(seq 1 5); do
    mkdir "subdir_$i"
    cd "subdir_$i"

    for j in $(seq 1 5); do
        file_size=$((1 + RANDOM % 1000))
        days_ago=$((RANDOM % 10))

        file_name="file_${i}_${j}__${file_size}bytes__${days_ago}daysago.txt"

        dd if=/dev/random of="$file_name" bs=1 count=$file_size 2>/dev/null

        touch_date=$(date -d "-$days_ago days" +%Y%m%d%H%M.%S)
        touch -t $touch_date "$file_name"
    done

    cd ..
done
