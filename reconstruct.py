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

data = [0] * 16

def PacketHandler(pkt):
    global data
    global mage
    if (pkt.addr2 == "b0:0b:b0:0b:b0:0b"):
        sn = pkt.SC // 16
        total = raw(pkt)[347]
        #print("Number", sn, "of", total)
        data[sn] = raw(pkt)[74:330]
        #print(hexdump(data[sn]))
        if check(total):
            image = b''
            for i in range(0, total):
                image += data[i]
            print("Saved image")
            print(hexdump(image))
            f = open('test.jpg', 'wb')
            f.write(image)
            f.close()
            last_image = image
            data = [0] * 16
        
def check(total):
    for i in range(0, total):
        if (data[i] == 0):
            return False     
    return True

sniff(iface="wlan1", prn=PacketHandler)
 