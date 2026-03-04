set -e

img=$1
cp /dev/null "$img"
truncate -s 1G "$img"

offset=2048
p1=$(( (200 * 1024*1024)/512 ))
p2=$(( (512 * 1024*1024)/512 ))

sfdisk "$img" << EOF
label: dos
unit: sectors
sector-size: 512
: start=$offset,        size=$p1, type=b
: start=$((offset+p1)),           type=b
EOF

mkfs.fat -F16 --offset 2048    "$img" $((p1/2 - 1024))
mkfs.fat --offset $((2048+p1)) "$img" $((p2/2))
