#!/bin/sh
# this script assumes that you print the time in the following decimal format:
# uint64_t t = *timer;
# xil_printf("%04u/010u: your message with args\n", (uint32_t)(t>>32), (uint32_t)t, ...);
# e.g. "00 01: 7916/1025112031: claimed buffer with delay 0"
#
# or that you use print_time to output the time in hex format:
# print_time(read_global_timer());
# xil_printf("your message with args\n", ...);
# e.g. "00 01: 0346/BB6A4465: claimed buffer with delay 0"

sed -E 's!^([[:digit:]]{2} [[:digit:]]{2}): ([0-9A-F]+/[0-9A-F]+): !\2: \1: !g' $* | sort -n
