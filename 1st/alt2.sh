tmp_file=$(mktemp -u)

wget -q \
    -O $tmp_file \
    http://m.86pm25.com/city/beijing.html

update_time=$(
    grep '更新' $tmp_file |
    grep -Eo '[0-9]{4}(/[0-9]{1,2}){2} [0-9]{1,2}(:[0-9]{2}){2}' |
    sed 's!/!-!g'
)

grep -P '"clear".*?dl1' $tmp_file |
sed -E "s!.*<div class=\"dl1\">([^<]+)</div>.*<div class=\"dl1\">([^<]+)μg/m³</div>.*!$update_time,\1,\2!"

rm $tmp_file
