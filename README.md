# `esp32-cam-80211-tx`

## Introduction

This project aims to send camera frames from an ESP32-CAM to a laptop over a distance of potentially several kilometers, using no additional hardware on the ESP32 end. 
How do we achieve this? As it turns out, WiFi radio waves can actually make it a very long way if there's nothing in the way. So if you just have a powerful receiving antenna, this should be possible. 

The issue is, the standard IEEE 802.11 protocol (and by extension WiFi) doesn't support this sort of unidirectional communication. Usually, the receiver needs to send back and acknowledgement frames.

However, in the same way that your router sends out unidirectional data in the form of SSID broadcasts, so can we... if we're willing to use a somewhat unofficial function in the ESP32's API. 

The idea behind this, and the code to do some came from [a video](https://www.youtube.com/watch?v=tBfa4yk5TdU) and [repository](https://github.com/Jeija/esp32-80211-tx) published by [@Jeija](https://github.com/Jeija/).

## Project Description
This software broadcasts the ESP32-CAM's camera framebuffer as a JPEG (JFIF) buffer. This is achieved by capturing the framebuffer of the camera, splitting it up into several packets, manually assembling IEEE 802.11 beacon frames in `main.c`, with each camera frame packet, and finally broadcasting them via the currently unofficial `esp_wifi_80211_tx` API in Espressif's [esp32-wifi-lib](https://github.com/espressif/esp32-wifi-lib).

As far as I know, you need a WiFi card that supports monitor mode on the receiving end, in order to sniff these packets. Once in monitor mode, we can receive the frames using `receive.py`, which will reassemble the packets and save it as a JPG file.

### Compile / Flash
This project uses the [Espressif IoT Development Framework](https://github.com/espressif/esp-idf) and PlatformIO.

To setup the receiving end, you will need `python3-imaging` installed, and a wireless adaptor that supports monitor mode (I used an Alfa AWUS036NHA). The code to do this is pretty simple, it just does a live scan of beacon frames, filters by MAC address (which is software-defined in `main.c`) and then tries to reassemble the frames, and if successful, saves it as `test.jpg`.

## Project License: MIT
```
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
