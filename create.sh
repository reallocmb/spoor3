#!/bin/bash

for ((i=1; i<=320; i++))
do
    random_id=$((RANDOM % 320 + 1))

    id="${random_id}d"
    name="test${i}"
    ./bin/spoor -c "$name", "$id"
done

