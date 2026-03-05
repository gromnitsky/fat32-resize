# shellcheck disable=2329,3014,3054

bats_require_minimum_version 1.5.0

fat32_resize=$BATS_TEST_DIRNAME/../_out/fat32-resize
adler32=$BATS_TEST_DIRNAME/adler32.rb # faster than sha*

@test "usage" {
    run -1 "$fat32_resize"
    [ "${lines[0]}" == "Usage:" ]
}

@test "info 600M" {
    run $fat32_resize info _out/junk/600M 1
    [ "$output" = "\
transport                 file
sectors                   1228800
partition_table           loop
partitions                1
sector_size               512
partition_start           0
partition_end             1228799
partition_length          1228800
fs_type                   fat32
fs_start                  0
fs_end                    1064951
fs_length                 1064952
partition_used_percent    86\
" ]
}

@test "info 600M wrong partition" {
    run -1 "$fat32_resize" info _out/junk/600M 0
    [ "$output" = "fat32-resize: ped_disk_get_partition" ]
}

@test "info 600M resize noop" {
    checksum1=`$adler32 _out/junk/600M`
    run $fat32_resize resize _out/junk/600M 1 0
    [ "$output" = "" ]
    checksum2=`$adler32 _out/junk/600M`
    [ "$checksum1" == "$checksum2" ]
}

@test "info 600M resize to 100%" {
    run $fat32_resize resize _out/junk/600M 1 100%
    [ "$output" = "" ]
    run $fat32_resize info _out/junk/600M 1
    [ "$output" = "\
transport                 file
sectors                   1228800
partition_table           loop
partitions                1
sector_size               512
partition_start           0
partition_end             1228799
partition_length          1228800
fs_type                   fat32
fs_start                  0
fs_end                    1228799
fs_length                 1228800
partition_used_percent    100\
" ]
}

@test "info 600M resize to 50%" {
    run $fat32_resize resize _out/junk/600M 1 50%
    [ "$output" = "" ]
    run $fat32_resize info _out/junk/600M 1
    [ "${lines[12]}" == "partition_used_percent    50" ]
}

f2() { awk '{print $2}'; }

@test "info 600M shrink/grow by 1 sector" {
    run $fat32_resize info _out/junk/600M 1
    len_orig=`echo "${lines[11]}" | f2`

    run $fat32_resize resize _out/junk/600M 1 -1

    run $fat32_resize info _out/junk/600M 1
    len_minus_1=`echo "${lines[11]}" | f2`
    [ "$(( len_orig - 1))" == "$len_minus_1" ]

    run $fat32_resize resize _out/junk/600M 1 +1

    run $fat32_resize info _out/junk/600M 1
    len_plus_1=`echo "${lines[11]}" | f2`
    [ "$len_orig" == "$len_plus_1" ]
}

@test "info 600M invalid resize params" {
    run -1 "$fat32_resize" resize _out/junk/600M 1 -100%
    [ "$output" = "fat32-resize: invalid SIZE percentage" ]
    run -1 "$fat32_resize" resize _out/junk/600M 1 101%
    [ "$output" = "fat32-resize: invalid SIZE percentage" ]
    run -1 "$fat32_resize" resize _out/junk/600M 1 100000000000000000
    [ "$output" = "fat32-resize: SIZE is too big" ]
    run -1 "$fat32_resize" resize _out/junk/600M 1 99999999999999999999999
    [ "$output" = "fat32-resize: SIZE out of range" ]
    run -1 "$fat32_resize" resize _out/junk/600M 1 525225
    [ "$output" = "fat32-resize: SIZE is too small" ]
    run -1 "$fat32_resize" resize _out/junk/600M 1 10%
    [ "$output" = "fat32-resize: SIZE is too small" ]
}

@test "info 1G resize to 100%" {
    run $fat32_resize resize _out/junk/1G 2 100%
    [ "$output" = "" ]
    run $fat32_resize info _out/junk/1G 2
    [ "$output" = "\
transport                 file
sectors                   2097152
partition_table           msdos
partitions                2
sector_size               512
partition_start           411648
partition_end             2097151
partition_length          1685504
fs_type                   fat32
fs_start                  411648
fs_end                    2097151
fs_length                 1685504
partition_used_percent    100\
" ]
}

@test "info 1G wrong fs" {
    run -1 "$fat32_resize" resize _out/junk/1G 1 100%
    [ "$output" = "fat32-resize: fat16" ]
}
