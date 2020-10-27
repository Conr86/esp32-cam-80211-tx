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

data = [0] * 8

def PacketHandler(pkt):
    global data
    global mage
    if (pkt.addr2 == "b0:0b:b0:0b:b0:0b"):
        sn = pkt.SC // 16
        print("SN:", sn)
        print(raw(pkt)[74:330])
        data[sn] = raw(pkt)[74:330]
        print(hexdump(data[sn]))
        '''if check():
            image = b''
            for x in data:
                image += x
            print("Writing")
            last_image = image
            f = open('test.jpg', 'wb')
            f.write(image)
            f.close()
            data = [0] * 5'''
        
def check():
    for i in range(0, 5):
        if (data[i] == 0):
            return False     
    return True

kw = {
    "iface" : "wlan1",
    "prn": PacketHandler
}


def t():
    root = tk.Tk()
    while len(last_image) == 0:
        pass
    print("READY")
    test = BytesIO(last_image)
    image = Image.open(test)
    tkimage = ImageTk.PhotoImage(img)
    tk.Label(root, image=tkimage).pack()
    root.mainloop()
    
x = threading.Thread(target=sniff, kwargs=kw)
#y = threading.Thread(target=t)
x.start()
#y.start()

PORT_NUMBER = 8080

#This class will handles any incoming request from
#the browser 
class myHandler(http.server.SimpleHTTPRequestHandler):
    
    #Handler for the GET requests
    def do_GET(self):
        try:
            self.send_response(200)
            f = open('test.jpg', 'rb')
            self.send_header('Content-type', 'image/jpg')
            self.end_headers()
            self.wfile.write(f.read())
            f.close()
            return

        except IOError:
            self.send_error(404,'File Not Found: %s' % self.path)

Handler = myHandler
with socketserver.TCPServer(("", PORT_NUMBER), Handler) as httpd:
    print("Http Server Serving at port", PORT_NUMBER)
    httpd.serve_forever()
    
 