'''
Plots packet count and throughput for flows in DRR experiment.

Original Author: Serhat Arslan (sarslan@stanford.edu)
Modified by: Ang Li & Erin Voss
'''

import argparse
import os
import matplotlib.pyplot as plt
import xml.etree.ElementTree as ET

parser = argparse.ArgumentParser()
parser.add_argument('--dir', '-d',
                    help="Directory to find the trace files",
                    required=True,
                    action="store",
                    dest="dir")
args = parser.parse_args()

def processData(fileName, dictToPopulate):
    with open(os.path.join(args.dir, fileName),'r') as f:
        for line in f:
            receivedPacketRow = line.split(',')

            port = int(receivedPacketRow[1]) - 49152;
            size = int(receivedPacketRow[2]);
            if port in dictToPopulate:
                dictToPopulate[port] = dictToPopulate[port] + size;
            else:
                dictToPopulate[port] = size;
        postProcessData(dictToPopulate)

def postProcessData(dictToPopulate):
    for k in dictToPopulate:
        dictToPopulate[k] = dictToPopulate[k] / 1000.0

fifoPortToThroughput = {}
drrPortToThroughput = {}
sfqPortToThroughput = {}

processData('fifo_50_receivedPacket.tr', fifoPortToThroughput)
processData('drr_50_receivedPacket.tr', drrPortToThroughput)
processData('sfq_50_receivedPacket.tr', sfqPortToThroughput)

print(fifoPortToThroughput)
print(drrPortToThroughput)
print(sfqPortToThroughput)

throughputFileName = os.path.join(args.dir, 'flows_throughput_50.png')

plt.figure()
plt.plot(fifoPortToThroughput.keys(), fifoPortToThroughput.values(), 'bs-', label='fifo')
plt.plot(drrPortToThroughput.keys(), drrPortToThroughput.values(), 'go-', label='drr')
plt.plot(sfqPortToThroughput.keys(), sfqPortToThroughput.values(), 'r^-', label='sfq')
plt.legend(loc='upper right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Throughput for flows using quantum = 50')
plt.savefig(throughputFileName)
print('Saving ' + throughputFileName)



drr50PortToThroughput = {}
drr100PortToThroughput = {}
drr200PortToThroughput = {}
drr500PortToThroughput = {}

processData('drr_50_receivedPacket.tr', drr50PortToThroughput)
processData('drr_100_receivedPacket.tr', drr100PortToThroughput)
processData('drr_200_receivedPacket.tr', drr200PortToThroughput)
processData('drr_500_receivedPacket.tr', drr500PortToThroughput)

drrQuantumFileName = os.path.join(args.dir, 'drr_quantum_throughput.png')

plt.figure()
plt.plot(drr50PortToThroughput.keys(), drr50PortToThroughput.values(), 'bs-', label='quantum=50')
plt.plot(drr100PortToThroughput.keys(), drr100PortToThroughput.values(), 'go-', label='quantum=100')
plt.plot(drr200PortToThroughput.keys(), drr200PortToThroughput.values(), 'r^-', label='quantum=200')
plt.plot(drr500PortToThroughput.keys(), drr500PortToThroughput.values(), 'y^-', label='quantum=500')

plt.legend(loc='lower right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Different quantum values used in DRR')
plt.ylim(900, 1200)
plt.savefig(drrQuantumFileName)
print('Saving ' + drrQuantumFileName)

#process delay data. drr_50.xml, fifo_50.xml, sfq_50.xml

files = ["drr_50.xml", "fifo_50.xml", "sfq_50.xml"]
dictionary_data = {}

print("processing delay data...")

for file_name in files:
    name = os.path.join(args.dir, file_name)
    dictionary = {}

    print("processing file: " + name)
    tree = ET.parse(name)
    root = tree.getroot()

    count = 0
    for flowStats in root:
        for flow in flowStats:
            if 'delaySum' in flow.attrib:
                count += 1
                delay_string = flow.attrib['delaySum']
                num_packets = flow.attrib['rxPackets']
                ns = delay_string.find("ns")
                delay_string = delay_string[0: ns]
                delay = float(delay_string)
                dictionary[count] = delay / float(num_packets)

    dictionary_data[file_name] = dictionary


delayFileName = os.path.join(args.dir, 'flows_delay_50.png')

drr50delay = dictionary_data[files[0]]
fifo50delay = dictionary_data[files[1]]
sfq50delay = dictionary_data[files[2]]

print(drr50delay)
print(fifo50delay)
print(sfq50delay)

plt.figure()
plt.plot(fifo50delay.keys(), fifo50delay.values(), 'bs-', label='fifo')
plt.plot(drr50delay.keys(), drr50delay.values(), 'go-', label='drr')
plt.plot(sfq50delay.keys(), sfq50delay.values(), 'r^-', label='sfq')
plt.legend(loc='upper right')
plt.ylabel('Avg Delay Sum (ns)')
plt.xlabel('Flow')
plt.title('Avg Delay Sum using quantum = 50')
plt.savefig(delayFileName)
print('Saving ' + delayFileName)
