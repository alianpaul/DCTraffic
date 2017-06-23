#!/usr/bin/python3.5

# Usage: check if the real flow packet count at the swtch is the same with the
# calculated flow packet count
# python3 check-flow.py start_sw_id end_sw_id

import sys

nid = int(sys.argv[1])
nid_stop  = int(sys.argv[2])

file_prefix = 'sw-'

file_real_postfix = '-real.txt'
file_cal_postfix = '.txt'


while nid <= nid_stop:

    print("Check node " + str(nid))
    #collect the real flow data at node nid
    real_flow = dict()
    with open(file_prefix + str(nid) + file_real_postfix) as real_file:
        for line in real_file:
            elems = line.split(' ')
            flow = ' '.join(elems[:-2])
            pcnt = int(elems[-2])

            if flow in real_flow:
                raise Exception("Duplicate flow in real flow set")
            else:
                real_flow[flow] = pcnt
                
    print("Real Flows Cnt: " + str(len(real_flow)))

    with open(file_prefix + str(nid) + file_cal_postfix) as cal_file:
        print(cal_file.readline() + cal_file.readline() + cal_file.readline())

        for line in cal_file:
            elems = line[:-1].split(' ')
            flow = ' '.join(elems[:-1])
            pcnt = int(elems[-1])

            if flow not in real_flow:
                print(flow + " is not in real flow")
            else:
                if real_flow[flow] != pcnt:
                    print(flow + " pcnt count is not match, real: " +
                          str(real_flow[flow]) + " cal: " + str(pcnt))
                del real_flow[flow]

        if len(real_flow) != 0:
            print("UnDected flows:")
            for flow, pcnt in real_flow.items():
                print(flow + str(pcnt))
            
                
    nid += 1
