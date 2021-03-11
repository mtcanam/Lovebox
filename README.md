Lovebox
===
Forked project to create a lovebox for my girlfriend for Valentine's day!.

## Composition 
Bit different than the original design. Parts were not soldered together, they were simply glued in place into a larger box. Otherwise, as below.


Below you can see where the OLED display, the light sensor with resistor and the servo motor are connected to the WeMos D1 Mini. The second photo is the soldered assembly. As you can see the micro-USB connector of the microcotroller board is at the ground. On top of the microcontroller a perfboard is soldered. Now the servo motor is added and also soldered to the perfboard. Notice the resistor and the light sensor that are also soldered to the board, with the light sensor pointing up. The display is soldered to another perfboard. The board provides a flat surface to hold the display in place. It is put on the motor and is connected to the other perfboard by using a Breakaway PCB Connector. It serves as a spacer to hold the other board above the motor.
![](https://i.imgur.com/6L2zcLs.png)
![](https://i.imgur.com/Y3Lg5tn.jpg)

![](https://i.imgur.com/y2joo0B.jpg)

## How it works

Quite a few alterations to the original methodology here. I had a colour OLED, so i wanted to be able to simply send a given image to the box. GIST seemed ineffective for this. Converted the project to an MQTT server that I use for other projects. Box simply subscribes to the lovebox/Message topic to receive messages. 

To send a color image, we can't just use the 1 and 0 from the original lovebox, we need to send colour codes for the pixels. The OLED is expecting unsigned 16-bit integers, and the library assumes that the values will be in 0x0000 format (hex of an RGB565 code). So we need to send an array of 128x128 6 character strings.

Since the memory is quite small (~50kb), we can't actually send the full image all at once using MQTT, as it exceeds the limit (and changing the limit will start writing to unintended locations!). Instead, a python script was written (bmp_to_hex_array) to take in an image (128px x 128px, 24 bit colour), and convert it to the hex codes. Then, the script sends each of the 128 lines of pixels to the MQTT server, with a line identifier as the first number (ie 101 0x0000 0x1234 .... 0x1235). The nodeMCU will process that line, print it to the screen, and wait for the next line.

The actual code in the nodeMCU is also slightly more complicated, as we needed to add several extra functions (like an auto-off as the OLED can burn in, and we can't just display the image when opened since the image can't be saved.
