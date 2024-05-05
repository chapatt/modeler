DATA_VAR=$1
SIZE_VAR=$2
FILE=$3

echo "static const unsigned char ${DATA_VAR}[] = {"
hexdump -ve '1/1 "0x%02x, "' "$FILE"
echo "0x00}; static const int ${SIZE_VAR} = sizeof(${DATA_VAR}) - 1;"
