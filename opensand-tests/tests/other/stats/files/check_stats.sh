#!/bin/sh


check_file_lines() {
	expected_lnum=$1
	name=$2

	if [ ! -f $name ]; then
		echo "$name log file does not exist!" 
		exit 1
	fi

	lnum=`wc -l < $name`
	if [ $lnum -ne $expected_lnum ]; then
		echo "$name log file has $lnum lines, expected $expected_lnum."
		exit 1
	fi
}

check_file_data() {
	expected_data=$1
	name=$2

	if [ ! -f $name ]; then
		echo "$name log file does not exist!"
		exit 1
	fi

	file_data=`cut -d " " -f 2- $name | tr "\n" "|"`

	if [ "$file_data" != "$expected_data" ]; then
		echo "$name log file has incorrect log data '$file_data'."
		exit 1
	fi
}

cd /tmp/opensand_tests/statistics/st1/test_prog || (echo "Failed to cd to the test_prog log dir"; exit 1)

pwd
ls -l

check_file_lines 1 double_probe.csv
check_file_data "log Hello, World.|log Hello, World.|" log.txt
check_file_data "|10.0|45.7999992371|224.800003052|1119.80004883|5594.79980469|27969.8007812|139844.796875|699219.8125|3496094.75|17480470.0|87402344.0|437011712.0|2185058560.0|10925292544.0|54626463744.0|2.73132322816e+11|1.36566158131e+12|6.82830803763e+12|3.41415401882e+13|1.70707703038e+14|" float_probe.csv
check_file_lines 1 int32_avg_probe.csv
check_file_lines 1 int32_dis_probe.csv
check_file_data "|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|" int32_last_probe.csv
check_file_lines 1 int32_max_probe.csv
check_file_lines 1 int32_min_probe.csv
check_file_lines 1 int32_sum_probe.csv

exit 0
