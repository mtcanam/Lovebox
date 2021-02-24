import time

import numpy as np
from PIL import Image
from tkinter.filedialog import askopenfilename
import paho.mqtt.client as mqtt


def get_hex(tuple):
    # Convert to 0xrggb
    r = tuple[0]
    g = tuple[1]
    b = tuple[2]
    rHex = format(r // 16, 'x')
    gHex = format(r // 8, 'x')
    if (len(gHex) < 2):
        gHex = str(0) + gHex
    bHex = format(r // 16, 'x')
    return "0x" + (str(rHex) + str(gHex) + str(bHex)).upper()


def create_output_files():
    img_file = filename = askopenfilename()
    im = Image.open(img_file)
    img_array = np.asarray(im, dtype='uint16')
    out_array = np.empty((img_array.shape[0], img_array.shape[1]), dtype=object)
    out_string = ""
    for i in range(img_array.shape[0]):
        for j in range(img_array.shape[1]):
            tuple = img_array[i][j]
            hexString = get_hex(tuple)
            out_string = out_string + hexString + " "
        with open("../image_codes/output" + str(i) + ".txt", "w") as text_file:
            print(str(i) + "\n" + out_string, file=text_file)
        out_string = ""
    return img_array


def publish_output_files(client, img_array):
    for i in range(img_array.shape[0]):
        with open("../image_codes/output" + str(i) + ".txt", "r") as text_file:
            text_str = text_file.read()
        client.publish("LoveBox/test", payload=text_str, qos=0, retain=False)
        time.sleep(1)


def getInt(tuple):
    # Convert to 0xrggb
    r = tuple[0]
    g = tuple[1]
    b = tuple[2]
    rHex = r // 16
    if (len(str(rHex)) < 2):
        rHex = "0" + str(rHex)
    gHex = r // 8
    if (len(str(gHex)) < 2):
        gHex = "0" + str(gHex)
    bHex = r // 16
    if (len(str(bHex)) < 2):
        bHex = "0" + str(bHex)
    return str(rHex) + str(gHex) + str(bHex)


if __name__ == '__main__':
    client = mqtt.Client("pyclient")
    client.username_pw_set(username="", password="")
    client.connect(".cloudmqtt.com", port=, )
    img_array = create_output_files()
    publish_output_files(client, img_array)
