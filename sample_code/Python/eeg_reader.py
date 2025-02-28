"""
This code is highly inspired from the pieeg-club's provided code. Big thanks to Ildar Rakhmatulin for his help.

use `> source PIEEG-CLIENT/env/bin/activate`
"""


# UDP Parameters
TARGET_IP = "10.0.0.7";
UDP_PORT = 5005;

import spidev
from RPi import GPIO
from scipy.signal import butter, lfilter, iirnotch
import socket;

socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM);

#GPIO
GPIO.setwarnings(False) 
GPIO.setmode(GPIO.BOARD)
GPIO.setup(37, GPIO.IN)
#  setup SPI protocol
spi = spidev.SpiDev()
spi.open(0,0)
spi.max_speed_hz=600000
spi.lsbfirst=False
spi.mode=0b01
spi.bits_per_word = 8

# REgisters for ADS1299 
who_i_am=0x00
config1=0x01
config2=0X02
config3=0X03

reset=0x06
stop=0x0A
start=0x08
sdatac=0x11
rdatac=0x10
wakeup=0x02
rdata = 0x12

ch1set=0x05
ch2set=0x06
ch3set=0x07
ch4set=0x08
ch5set=0x09
ch6set=0x0A
ch7set=0x0B
ch8set=0x0C

data_test= 0x7FFFFF
data_check=0xFFFFFF
 
def send_command(command):
    send_data = [command]
    com_reg=spi.xfer(send_data)
def write_byte(register,data):                        
    write=0x40
    register_write=write|register
    data = [register_write,0x00,data]
    print (data)
    spi.xfer(data)
                                     
# command to ADS1299
send_command (wakeup)
send_command (stop)
send_command (reset)
send_command (sdatac)

# write registers to ADS1299
write_byte (0x14, 0x80) #GPIO
write_byte (config1, 0x96)
write_byte (config2, 0xD4)
write_byte (config3, 0xFF)
write_byte (0x04, 0x00)
write_byte (0x0D, 0x00)
write_byte (0x0E, 0x00)
write_byte (0x0F, 0x00)
write_byte (0x10, 0x00)
write_byte (0x11, 0x00)
write_byte (0x15, 0x20)
 
write_byte (0x17, 0x00)
write_byte (ch1set, 0x00)
write_byte (ch2set, 0x00)
write_byte (ch3set, 0x00)
write_byte (ch4set, 0x00)
write_byte (ch5set, 0x00)
write_byte (ch6set, 0x00)
write_byte (ch7set, 0x00)
write_byte (ch8set, 0x00)

send_command (rdatac)
send_command (start)

#1.2 Band-pass + notch filter 
notch_frequency = 60; # we use 60hz in montreal
quality_factor = 30;

highcut = 1
lowcut = 10

fps = 250


def butter_lowpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a
def butter_lowpass_filter(data, cutoff, fs, order=5):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)
    return y
def butter_highpass(cutoff, fs, order=3):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='high', analog=False)
    return b, a
def butter_highpass_filter(data, cutoff, fs, order=5):
    b, a = butter_highpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)  # Use lfilter for real-time processing
    return y

def notch_filter(data, notch_freq, fs, quality_factor=30):
    b, a = iirnotch(notch_freq, quality_factor, fs)
    y = lfilter(b, a, data)  # Use lfilter for real-time processing
    return y

while 1:
    GPIO.wait_for_edge(37, GPIO.FALLING)
    output = spi.readbytes(27)

    filtered_result = [];

    for a in range (3,25,3):
        voltage_1 = (output[a] << 8)| output[a+1]
        voltage_1 = (voltage_1 << 8)| output[a+2]

        convert_voltage = voltage_1 | data_test

        if convert_voltage == data_check:
            voltage_1_after_convert = (voltage_1-16777214)
        else:
            voltage_1_after_convert = voltage_1
        channel_num =  (a / 3)

        processed_value = round(1000000*4.5*(voltage_1_after_convert / 16777215), 2)

        # Apply filters (bandpass + notch)
        filtered_value = butter_highpass_filter([processed_value], highcut, fps)
        filtered_value = butter_lowpass_filter(filtered_value, lowcut, fps)
        filtered_value = notch_filter(filtered_value, notch_frequency, fps)

        filtered_result.append(round(filtered_value[0], 2))

    # Convert processed list to a space-separated string
    result = bytes(" ".join(map(str, filtered_result)), "utf-8")

    print(result)
    socket.sendto(result, (TARGET_IP, UDP_PORT));

spi.close()
              
                