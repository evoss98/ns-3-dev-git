#!/bin/bash

[ ! -d outputs ] && mkdir outputs

DIR=outputs/drr
[ ! -d $DIR ] && mkdir $DIR

for QUEUEDISC in "fifo" "drr" "sfq"; do
	
	# Run the NS-3 Simulation
	./waf --run "scratch/drr --queueDisc=$QUEUEDISC"
	./waf --run "scratch/drr --queueDisc=$QUEUEDISC --multiHost=true"
	./waf --run "scratch/drr --queueDisc=$QUEUEDISC --multiHost=true --multiHop=true"
done

for QUANTUM in 25 50 100 200 560 1000 5000 10000; do

	# Run the NS-3 Simulation
	./waf --run "scratch/drr --queueDisc=drr --quantum=$QUANTUM"
done

# Plot the trace figures
python2.7 scratch/plot_figures.py --dir $DIR/

echo "Simulations are done!"
# python3 -m http.server