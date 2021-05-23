#!/bin/bash

[ ! -d outputs ] && mkdir outputs

DIR=outputs/drr
[ ! -d $DIR ] && mkdir $DIR

for QUEUEDISC in "fifo" "drr" "sfq"; do
	
	# Run the NS-3 Simulation
	./waf --run "scratch/drr --queueDisc=$QUEUEDISC"
done

for QUANTUM in 100 200 500; do

	# Run the NS-3 Simulation
	./waf --run "scratch/drr --queueDisc=drr --quantum=$QUANTUM"
done

# Plot the trace figures
python2.7 scratch/plot_figures.py --dir $DIR/

echo "Simulations are done!"
# python3 -m http.server