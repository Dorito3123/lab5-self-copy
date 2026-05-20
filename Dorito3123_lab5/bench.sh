#!/bin/bash
set -e

APP="./app"
DATA="data/processed/docs.jsonl"
TOTAL=$(wc -l < "$DATA" | tr -d ' ')

echo "dataset: $TOTAL docs"

echo ""
echo "indexing time (s):"
printf "%-8s  %-10s  %-12s  %-10s\n" "docs" "avl" "rbtree" "btree"

for LIMIT in 10000 30000 50000; do
    [ "$LIMIT" -le "$TOTAL" ] || continue
    t1=$($APP index --type=avl   --limit="$LIMIT" 2>/dev/null | grep IndexTime | awk '{print $2}')
    t2=$($APP index --type=rb    --limit="$LIMIT" 2>/dev/null | grep IndexTime | awk '{print $2}')
    t3=$($APP index --type=btree --limit="$LIMIT" 2>/dev/null | grep IndexTime | awk '{print $2}')
    printf "%-8d  %-10s  %-12s  %-10s\n" "$LIMIT" "$t1" "$t2" "$t3"
done

echo ""
echo "search time (ms/query, 1000 runs):"
printf "%-8s  %-10s  %-12s  %-10s\n" "words" "avl" "rbtree" "btree"

for W in 1 2 3; do
    s1=$($APP bench --type=avl   --words="$W" 2>/dev/null | grep -o 'avg=[0-9.]*' | cut -d= -f2)
    s2=$($APP bench --type=rb    --words="$W" 2>/dev/null | grep -o 'avg=[0-9.]*' | cut -d= -f2)
    s3=$($APP bench --type=btree --words="$W" 2>/dev/null | grep -o 'avg=[0-9.]*' | cut -d= -f2)
    printf "%-8d  %-10s  %-12s  %-10s\n" "$W" "$s1" "$s2" "$s3"
done

echo ""
echo "memory (MB):"
printf "%-8s  %-10s  %-12s  %-10s\n" "docs" "avl" "rbtree" "btree"

for LIMIT in 10000 50000; do
    [ "$LIMIT" -le "$TOTAL" ] || continue
    m1=$($APP index --type=avl   --limit="$LIMIT" 2>/dev/null | grep Memory | awk '{print $2}')
    m2=$($APP index --type=rb    --limit="$LIMIT" 2>/dev/null | grep Memory | awk '{print $2}')
    m3=$($APP index --type=btree --limit="$LIMIT" 2>/dev/null | grep Memory | awk '{print $2}')
    printf "%-8d  %-10s  %-12s  %-10s\n" "$LIMIT" "$m1" "$m2" "$m3"
done
