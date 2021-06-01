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

def processData(fileName, dictToPopulate, multiHop=False):
    with open(os.path.join(args.dir, fileName),'r') as f:
        for line in f:
            receivedPacketRow = line.split(',')

            port = int(receivedPacketRow[1]) - 49152;
            size = int(receivedPacketRow[2]);
            if multiHop:
                ip = receivedPacketRow[3]
                if ip.split(".")[2] == '1':
                    port += 20
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

# print(fifoPortToThroughput)
# print(drrPortToThroughput)
# print(sfqPortToThroughput)

mainReproFileName = os.path.join(args.dir, 'figures/main_repro_figure.png')

plt.figure()
plt.plot(fifoPortToThroughput.keys(), fifoPortToThroughput.values(), 'bs-', label='fifo')
plt.plot(drrPortToThroughput.keys(), drrPortToThroughput.values(), 'go-', label='drr')
plt.legend(loc='upper right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Throughput for flows using quantum = 50')
plt.savefig(mainReproFileName)
print('Saving ' + mainReproFileName)


throughputFileName = os.path.join(args.dir, 'figures/single_flows_throughput_50.png')

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



fifoMultiHostAddressToThroughput = {}
drrMultiHostAddressToThroughput = {}
sfqMultiHostAddressToThroughput = {}

processData('fifo_50_multihost_receivedPacket.tr', fifoMultiHostAddressToThroughput, True)
processData('drr_50_multihost_receivedPacket.tr', drrMultiHostAddressToThroughput, True)
processData('sfq_50_multihost_receivedPacket.tr', sfqMultiHostAddressToThroughput, True)

multiHostThroughputFileName = os.path.join(args.dir, 'figures/multihost_flows_throughput_50.png')

plt.figure()
plt.plot(fifoMultiHostAddressToThroughput.keys(), fifoMultiHostAddressToThroughput.values(), 'bs-', label='fifo')
plt.plot(drrMultiHostAddressToThroughput.keys(), drrMultiHostAddressToThroughput.values(), 'go-', label='drr')
plt.plot(sfqMultiHostAddressToThroughput.keys(), sfqMultiHostAddressToThroughput.values(), 'r^-', label='sfq')
plt.legend(loc='upper right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Multi-host throughput using quantum = 50')
plt.savefig(multiHostThroughputFileName)
print('Saving ' + multiHostThroughputFileName)



fifoMultiHopAddressToThroughput = {}
drrMultiHopAddressToThroughput = {}
sfqMultiHopAddressToThroughput = {}

processData('fifo_50_multihost_multihop_receivedPacket.tr', fifoMultiHopAddressToThroughput, True)
processData('drr_50_multihost_multihop_receivedPacket.tr', drrMultiHopAddressToThroughput, True)
processData('sfq_50_multihost_multihop_receivedPacket.tr', sfqMultiHopAddressToThroughput, True)

multiHopThroughputFileName = os.path.join(args.dir, 'figures/multihop_flows_throughput_50.png')

plt.figure()
plt.plot(fifoMultiHopAddressToThroughput.keys(), fifoMultiHopAddressToThroughput.values(), 'bs-', label='fifo')
plt.plot(drrMultiHopAddressToThroughput.keys(), drrMultiHopAddressToThroughput.values(), 'go-', label='drr')
plt.plot(sfqMultiHopAddressToThroughput.keys(), sfqMultiHopAddressToThroughput.values(), 'r^-', label='sfq')
plt.legend(loc='upper right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Multi-hop throughput using quantum = 50')
plt.savefig(multiHopThroughputFileName)
print('Saving ' + multiHopThroughputFileName)



drr25PortToThroughput = {}
drr50PortToThroughput = {}
drr100PortToThroughput = {}
drr200PortToThroughput = {}
drr500PortToThroughput = {}
drr560PortToThroughput = {}
drr1000PortToThroughput = {}
drr5000PortToThroughput = {}
drr10000PortToThroughput = {}

processData('drr_25_receivedPacket.tr', drr25PortToThroughput)
processData('drr_50_receivedPacket.tr', drr50PortToThroughput)
processData('drr_100_receivedPacket.tr', drr100PortToThroughput)
processData('drr_200_receivedPacket.tr', drr200PortToThroughput)
processData('drr_500_receivedPacket.tr', drr500PortToThroughput)
processData('drr_560_receivedPacket.tr', drr560PortToThroughput)
processData('drr_1000_receivedPacket.tr', drr1000PortToThroughput)
processData('drr_5000_receivedPacket.tr', drr5000PortToThroughput)
processData('drr_10000_receivedPacket.tr', drr10000PortToThroughput)

drrQuantumFileName = os.path.join(args.dir, 'figures/drr_quantum_throughput.png')

plt.figure()
plt.plot(drr25PortToThroughput.keys(), drr25PortToThroughput.values(), 'c2-', label='quantum=25')
plt.plot(drr50PortToThroughput.keys(), drr50PortToThroughput.values(), 'bs-', label='quantum=50')
plt.plot(drr100PortToThroughput.keys(), drr100PortToThroughput.values(), 'go-', label='quantum=100')
plt.plot(drr200PortToThroughput.keys(), drr200PortToThroughput.values(), 'r^-', label='quantum=200')
plt.plot(drr500PortToThroughput.keys(), drr500PortToThroughput.values(), 'y^-', label='quantum=500')
plt.plot(drr560PortToThroughput.keys(), drr560PortToThroughput.values(), 'k>-', label='quantum=560')
plt.plot(drr1000PortToThroughput.keys(), drr1000PortToThroughput.values(), 'mx-', label='quantum=1000')
plt.plot(drr5000PortToThroughput.keys(), drr5000PortToThroughput.values(), 'r<-', label='quantum=5000')
plt.plot(drr10000PortToThroughput.keys(), drr10000PortToThroughput.values(), 'bD-', label='quantum=10000')

plt.legend(loc='lower right')
plt.ylabel('Total Throughput Received (Kbits)')
plt.xlabel('Flow')
plt.title('Different quantum values used in DRR')
plt.ylim(900, 1200)
plt.savefig(drrQuantumFileName)
print('Saving ' + drrQuantumFileName)



print("processing delay data...")

def processDelayFiles(files):
    dictionary_data = {}
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

    return dictionary_data


delayFileName = os.path.join(args.dir, 'figures/flows_avg_delay_50.png')

files = ["drr_50.xml", "fifo_50.xml", "sfq_50.xml"]
dictionary_data = processDelayFiles(files)
drr50delay = dictionary_data[files[0]]
fifo50delay = dictionary_data[files[1]]
sfq50delay = dictionary_data[files[2]]

plt.figure()
plt.plot(fifo50delay.keys(), fifo50delay.values(), 'bs-', label='fifo')
plt.plot(drr50delay.keys(), drr50delay.values(), 'go-', label='drr')
plt.plot(sfq50delay.keys(), sfq50delay.values(), 'r^-', label='sfq')
plt.legend(loc='upper right')
plt.ylabel('Avg Delay Per Packet (ns)')
plt.xlabel('Flow')
plt.title('Avg Delay Per Packet using quantum = 50')
plt.savefig(delayFileName)
print('Saving ' + delayFileName)


print("processing delay data for different quantum values...")

files = ["drr_25.xml", "drr_50.xml", "drr_100.xml", "drr_200.xml", "drr_500.xml", "drr_560.xml", "drr_1000.xml"]
dictionary_data = processDelayFiles(files)

delayWithQuantumFileName = os.path.join(args.dir, 'figures/flows_delay_quantum_values.png')

drr25delay = dictionary_data[files[0]]
drr50delay = dictionary_data[files[1]]
drr100delay = dictionary_data[files[2]]
drr200delay = dictionary_data[files[3]]
drr500delay = dictionary_data[files[4]]
drr560delay = dictionary_data[files[5]]
drr1000delay = dictionary_data[files[6]]

plt.figure()
plt.plot(drr25delay.keys(), drr25delay.values(), 'c2-', label='quantum=25')
plt.plot(drr50delay.keys(), drr50delay.values(), 'bs-', label='quantum=50')
plt.plot(drr100delay.keys(), drr100delay.values(), 'go-', label='quantum=100')
plt.plot(drr200delay.keys(), drr200delay.values(), 'r^-', label='quantum=200')
plt.plot(drr500delay.keys(), drr500delay.values(), 'yo-', label='quantum=500')
plt.plot(drr560delay.keys(), drr560delay.values(), 'k>-', label='quantum=560')
plt.plot(drr1000delay.keys(), drr1000delay.values(), 'mx-', label='quantum=1000')
plt.legend(loc='upper right')
plt.ylabel('Avg Delay Per Packet (ns)')
plt.xlabel('Flow')
plt.title('Avg Delay Per Packet using different quantum values')
plt.savefig(delayWithQuantumFileName)
print('Saving ' + delayWithQuantumFileName)
