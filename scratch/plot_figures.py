'''
Plots packet count and throughput for flows in DRR experiment.

Original Author: Serhat Arslan (sarslan@stanford.edu)
Modified by: Ang Li & Erin Voss
'''

import argparse
import os
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('--dir', '-d',
                    help="Directory to find the trace files",
                    required=True,
                    action="store",
                    dest="dir")
args = parser.parse_args()

fifoPortToThroughput = {}
drrPortToThroughput = {}

with open(os.path.join(args.dir, 'fifo_receivedPacket.tr'),'r') as f:
    for line in f:
        receivedPacketRow = line.split(',')

        port = int(receivedPacketRow[1]) - 49152;
        size = int(receivedPacketRow[2]);
        if port in fifoPortToThroughput:
            fifoPortToThroughput[port] = fifoPortToThroughput[port] + size;
        else:
            fifoPortToThroughput[port] = size;

with open(os.path.join(args.dir, 'drr_receivedPacket.tr'),'r') as f:
    for line in f:
        receivedPacketRow = line.split(',')

        port = int(receivedPacketRow[1]) - 49152;
        size = int(receivedPacketRow[2]);
        if port in drrPortToThroughput:
            drrPortToThroughput[port] = drrPortToThroughput[port] + size;
        else:
            drrPortToThroughput[port] = size;

for k in fifoPortToThroughput:
    fifoPortToThroughput[k] = fifoPortToThroughput[k] / 1000


for k in drrPortToThroughput:
    drrPortToThroughput[k] = drrPortToThroughput[k] / 1000


print(fifoPortToThroughput)
print(drrPortToThroughput)

throughputFileName = os.path.join(args.dir, 'flows_throughput.png')

plt.figure()
plt.plot(fifoPortToThroughput.keys(), fifoPortToThroughput.values(), 'bs',
    drrPortToThroughput.keys(), drrPortToThroughput.values(), 'go')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.savefig(throughputFileName)
print('Saving ' + throughputFileName)
