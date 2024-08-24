#! /bin/bash

# Get current time in the format of "YYYY-MM-DD HH:MM"
# parameters: none
# return: time
get_current_time() {
    date +"%Y-%m-%d %H:%M"
}

# Get TCP statistics
# parameters: none
# return: segments_in segments_out
get_tcp_stats() {
    netstat --statistics --tcp | grep -E '[0-9]+ segments (received|sent out)' | awk '{print $1}'
}

# Calculate the symbol of the difference between the current total and the previous total
# While cache the current total
# parameters: total
# return: none (set symbol)
calculate_symbol() {
    if ! [ -z $last_total ]; then
        local diff=$(($1 - last_total))
        if [ $diff -gt 10 ]; then
            symbol="+"
        elif [ $diff -lt -10 ]; then
            symbol="-"
        else
            symbol=" "
        fi
    else
        symbol=" "
    fi

    last_total=$1
}

# Get the TCP statistics of the last minute
# parameters: none
# return: none (set in_1min out_1min total_1min symbol)
gen_tcp_stats_last_min() {
    stats=($(get_tcp_stats))
    segments_in=${stats[0]}
    segments_out=${stats[1]}

    in_1min=$((segments_in - last_in))
    out_1min=$((segments_out - last_out))
    total_1min=$((in_1min + out_1min))
    calculate_symbol $total_1min

    last_in=$segments_in
    last_out=$segments_out
}

# Initialize the initial statistics
gen_tcp_stats_last_min
unset last_total
echo "Start at $(get_current_time)"

while true; do
    sleep 60

    gen_tcp_stats_last_min

    printf "%s %6d %6d %6d %s\n" "$(get_current_time)" $out_1min $in_1min $total_1min $symbol
done
