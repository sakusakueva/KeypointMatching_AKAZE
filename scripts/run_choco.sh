#!/bin/bash

DATA="choco"

for ((i=0 ; i<10 ; i++))
do
     ./keypoint_matching \
          --tmp data/${DATA}/template.png \
          --input data/${DATA}/data${i}.png \
          --best_match_size=50 \
          --use_color \
          --use_rect \
          --time
done

