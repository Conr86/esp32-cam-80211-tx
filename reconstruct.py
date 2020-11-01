#!/usr/bin/python
import http.server
import socketserver
from os import curdir, sep
from scapy.all import *
from PIL import Image, ImageTk
from io import BytesIO
import io
import threading
import tkinter as tk 

last_image = b''

data = {}

def PacketHandler(pkt):
    global data
    global mage
    if (pkt.addr2 == "b0:0b:b0:0b:b0:0b"):
        sn = pkt.SC // 16
        total = raw(pkt)[347]
        uid = raw(pkt)[348]
        print("Batch", uid, "Number", sn, "of", total) 

        if uid not in data:
            data[uid] = [0] * total
        
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
                last_image = image
                data[uid] = [0] * len(data[uid])

sniff(iface="wlan1mon", prn=PacketHandler)
 