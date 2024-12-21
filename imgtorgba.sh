INPUT=$1
OUTPUT=$2

magick $INPUT -depth 8 -colorspace RGB -alpha on $OUTPUT
