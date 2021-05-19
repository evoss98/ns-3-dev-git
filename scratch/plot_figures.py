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

portToThroughput = {}

with open(os.path.join(args.dir, 'receivedPacket.tr'),'r') as f:
    for line in f:
        receivedPacketRow = line.split(',')

        port = receivedPacketRow[1];
        size = int(receivedPacketRow[2]);
        if port in portToThroughput:
            portToThroughput[port] = portToThroughput[port] + size;
        else:
            portToThroughput[port] = size;

for k in portToThroughput:
    portToThroughput[k] = portToThroughput[k] / 1000

x = []
for i in range(1, 21, 1):
    x.append(i)

print(portToThroughput.values())
throughputFileName = os.path.join(args.dir, 'flows_throughput.png')

plt.figure()
plt.plot(x, portToThroughput.values(), 'bs')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.savefig(throughputFileName)
print('Saving ' + throughputFileName)
