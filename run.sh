#!/bin/bash

[ ! -d outputs ] && mkdir outputs

DIR=outputs/drr
[ ! -d $DIR ] && mkdir $DIR

# Run the NS-3 Simulation
./waf --run "scratch/drr"
# Plot the trace figures
python2.7 scratch/plot_figures.py --dir $DIR/

echo "Simulations are done!"
# python3 -m http.server