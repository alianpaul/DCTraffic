#!/usr/bin/python3.5

# Usage: Generate the real flow packet cnt at each switch
# python3 packet-parser.py packet.tr 

import re
import sys


tr_name = sys.argv[1]

line_expr = r'''
^(\+|\-|r|d)     #start with action +/-/r/d
\s
(\d+.\d+)        #matches the time
\s
(\S+)            #device
\s
(ns3::EthernetHeader.*)  
(ns3::Ipv4Header.*)
(ns3::TcpHeader|ns3::UdpHeader.*)
(ns3::EthernetTrailer.*)
$        
'''

device_expr = r'''
^/NodeList/
(\d+)         #Node id
/DeviceList/
(\d+)         #Device id
'''

ip_expr = r'''
(\d+\.\d+\.\d+\.\d+)
\s
>
\s
(\d+\.\d+\.\d+\.\d+)
'''
port_expr = r'''
^ns3::
(Udp|Tcp)
.*
\s
(\d+)
\s
>
\s
(\d+)
'''

def extract_packet_info(line):
    pattern = re.compile(line_expr, re.VERBOSE)
    match = pattern.search(line).groups()
    
    action = match[0]
    time = match[1]
    device = match[2]
    eth_hd = match[3]
    ip_hd = match[4]
    trans_hd = match[5]

    #Construct the packet info
    packet_info = dict()
    packet_info['act'] = action 
    
    pattern = re.compile(device_expr, re.VERBOSE)
    (n_id, d_id) = pattern.search(device).groups()
    packet_info['nid'] = n_id
    packet_info['device'] = d_id

    pattern = re.compile(ip_expr, re.VERBOSE)
    (src_ip, dst_ip) = pattern.search(ip_hd).groups()
    packet_info['srcip'] = src_ip
    packet_info['dstip'] = dst_ip

    pattern = re.compile(port_expr, re.VERBOSE)
    (prot, src_port, dst_port) = pattern.search(trans_hd).groups()
    packet_info['prot'] = prot
    packet_info['srcport'] = src_port
    packet_info['dstport'] = dst_port

    
    return packet_info

    
        

with open(tr_name) as tr_file:

    print("Reading packets trace data")
    
    node_infos = dict()
    
    for line in tr_file:
        packet_info = extract_packet_info(line)
        nid = packet_info['nid']
        act = packet_info['act']
        flow = [ packet_info['srcip'],
                 packet_info['dstip'],
                 packet_info['prot'].upper(),
                 packet_info['srcport'],
                 packet_info['dstport'] ]
        
        flow_str = ' '.join(flow)

        if nid not in node_infos:
            node_infos[nid] = dict() #flow act dict
        if flow_str not in node_infos[nid]:
            node_infos[nid][flow_str] = ''

        node_infos[nid][flow_str] += act

    sw_packet_file_prefix = 'sw-'
    for nid, flows in node_infos.items():

        print("Constructing sw " + str(nid) + "'s flow data")
        filename = sw_packet_file_prefix + str(nid) + '-real.txt';
        
        with open(filename, 'w') as file:
            for flow, acts in flows.items():
                pcnt = 0
                for act in acts:
                    if act == '+' or act == 'd':
                        pcnt += 1
                file.write(flow +  ' ' + str(pcnt) + ' ' + acts +'\n')

        print("Construction complete, save at " + filename)
            
        

        
        
        
        

    
