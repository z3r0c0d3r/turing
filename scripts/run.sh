#!/bin/bash

time ../build/turing --Da 1 --Db 100 --dx 1.0 --dt 0.001 --width 100 --height 100 --steps 150 --tsteps 100 --outfile "data.bin"
python vis.py
