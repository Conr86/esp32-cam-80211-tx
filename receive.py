#!/usr/bin/python
from scapy.all import *

last_image = b''

data = {}

def PacketHandler(pkt):
    global data
    if (pkt.addr2 == "b0:0b:b0:0b:b0:0b"):
        sn = pkt.SC // 16
        total = raw(pkt)[347]
        uid = raw(pkt)[348]
        print("Snapshot", uid, "Packet", sn, "of", total) 

        if uid not in data:
            data[uid] = [0] * (total + 1)
        
        data[uid][sn] = raw(pkt)[74:330]
        
        #print(hexdump(data[uid][sn]))

        for uid in data:
            if all(data[uid]):
                image = b''
                for pkt in data[uid]:
                    image += pkt
                print("> Saved image!")
                #print(hexdump(image))
                f = open('test.jpg', 'wb')
                f.write(image)
                f.close()
                data[uid] = [0] * len(data[uid])

sniff(iface="wlan1mon", prn=PacketHandler)
 