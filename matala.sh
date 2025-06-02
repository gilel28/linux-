#!/bin/bash


if [ -z "$1" ]; then
  echo "Usage: $0 <number_of_blocks>"
  exit 1
fi

NUM_BLOCKS=$1
OUTPUT_FILE="blocks.txt"
> "$OUTPUT_FILE"

BASE_URL="https://api.blockcypher.com/v1/btc/main"
CURRENT_HASH=$(curl -s "$BASE_URL" | grep -oP '"hash":\s*"\K[^"]+')

for ((i=0; i<NUM_BLOCKS; i++)); do
    BLOCK_JSON=$(curl -s "$BASE_URL/blocks/$CURRENT_HASH")

    HASH=$(echo "$BLOCK_JSON" | grep -oP '"hash":\s*"\K[^"]+')
    HEIGHT=$(echo "$BLOCK_JSON" | grep -oP '"height":\s*\K[0-9]+')
    TOTAL=$(echo "$BLOCK_JSON" | grep -oP '"total":\s*\K[0-9]+')
    TIME=$(echo "$BLOCK_JSON" | grep -oP '"time":\s*"\K[^"]+')
    RELAYED_BY=$(echo "$BLOCK_JSON" | grep -oP '"relayed_by":\s*"\K[^"]+')
    PREV_BLOCK=$(echo "$BLOCK_JSON" | grep -oP '"prev_block":\s*"\K[^"]+')

    echo "Block $((i+1)):" >> "$OUTPUT_FILE"
    echo "Hash: $HASH" >> "$OUTPUT_FILE"
    echo "Height: $HEIGHT" >> "$OUTPUT_FILE"
    echo "Total: $TOTAL" >> "$OUTPUT_FILE"
    echo "Time: $TIME" >> "$OUTPUT_FILE"
    echo "Relayed by: $RELAYED_BY" >> "$OUTPUT_FILE"
    echo "Previous Block: $PREV_BLOCK" >> "$OUTPUT_FILE"
    echo "------------------------" >> "$OUTPUT_FILE"

    CURRENT_HASH=$PREV_BLOCK
done

echo "Saved $NUM_BLOCKS blocks to $OUTPUT_FILE"

