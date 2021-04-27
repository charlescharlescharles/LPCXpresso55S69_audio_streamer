## Author: Charles Elliott
## Date created: March 2021

## Description: For use in the audio_player application. Script handles user input & 
##              sends audio file data over UART to the MCU. 

############################# IMPORTS #####################################

import wave
import serial
import serial.tools.list_ports
import time
import keyboard
import os
import sys

############################ VARIABLES #####################################

signal_ready = b'\xFF'
signal_fault = b'\xFE'
signal_play = b'\xFD'
signal_stop = bytearray(2)
baudrate =   921600
filename = '11025Hz_16bit.wav'
portname = '/dev/cu.usbserial-FTA56VXN'
keypress = ''
PAUSE = 'paused'
PLAY = 'playing'
STOP = 'stopped'
state = PLAY
SPACE = 'space'
ESC = 'esc'
buffer_size = 0 #number of frames the buffer on MCU side can hold
starting_playback = 0 #flag used for edge case
sampling_frequency = 0
bit_depth = 0
num_channels = 0
BUF_SIZE_BYTES = 4096 #buffer size on the MCU side
BITS_PER_BYTE = 8
TIMEOUT = 5 #seconds




############################ FUNCTIONS #####################################


# TODO support for other OS
def initialize_serial_port() : 

    #TODO 
    #   check current directory for file with port name in it
    #   if file doesn't exist, get list of available ports & make selection
    #   if file does exist, read portname and try to open it
    portlist = serial.tools.list_ports.comports()
    print('select the serial port connected to the MCU: ')  
    for i in range(len(portlist)) : 
        print(str(i) + '. ' + str(portlist[i].name))

    validinput = 0

    while not validinput : 
        portnum = int(input('Enter the number corresponding with the port you want to select: '))
        if portnum in range(len(portlist)) : 
            portname = '/dev/' + str(portlist[portnum].name)
            validinput = 1
        else :
            print('Invalid input')

    global ser
    ser = serial.Serial(portname)
    print('opening port: ' + ser.name)
    ser.baudrate = baudrate
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    #ser.timeout = TIMEOUT

# TODO change to "testing connection"
# check baud rate
def baud_check() : 

    print('testing connection...')

    ser.read_until(signal_ready)
    # TODO something with timeout
    print('connection established. checking baud... ')
    MCU_baud_bytes = ser.read(3)
    print(MCU_baud_bytes)
    MCU_baud = int.from_bytes(MCU_baud_bytes, byteorder = 'big')

    ser.reset_input_buffer()
    if baudrate != MCU_baud : 
        ser.write(signal_fault)
        print('baud check failed. python module baudrate is ' + str(baudrate) + 'while MCU baudrate is ' + str(MCU_baud))
        return -2
    else : 
        print('baud check passed')
        ser.write(signal_ready)


# get buffer size
def get_buffer_size() : 

    val = ser.read_until(signal_ready)
    global buffer_size
    buffer_size = int.from_bytes(ser.read(2), byteorder = 'big')
    print("buffer size: " + str(buffer_size))


# setup keyboard interrupts
def init_keyboard_interrupts() : 

    keyboard.add_hotkey('space', space_callback, args=(), suppress=False, timeout=1, trigger_on_release=False)
    keyboard.add_hotkey('esc', esc_callback, args=(), suppress=False, timeout=1, trigger_on_release=False)


# initialize everything non-file specific
def setup() : 

    initialize_serial_port()

    baud_check()

    init_keyboard_interrupts()
   

def send_playback_info() : 

    global sampling_frequency
    global bit_depth
    global num_channels


    # send audio characteristic data
    ser.read_until(signal_ready)
    bit_depth = wav.getsampwidth() * BITS_PER_BYTE
    print('bit depth: ' + str(bit_depth))
    bytes_bit_depth = bit_depth.to_bytes(1, byteorder = 'little', signed = False)
    sampling_frequency = wav.getframerate()
    print('sampling frequency: ' + str(sampling_frequency))
    bytes_sampling_frequency = sampling_frequency.to_bytes(4, byteorder = 'little', signed = False)
    num_channels = wav.getnchannels()
    print('number of channels: ' + str(num_channels))
    bytes_num_channels = num_channels.to_bytes(1, byteorder = 'little', signed = False)
    ser.write(bytes_bit_depth)
    ser.write(bytes_sampling_frequency)
    ser.write(bytes_num_channels)


    #TODO configure volume 


# get the wav files in the cwd
def get_wav_file_list() : 

    dir = os.getcwd() #get the current directory
    filelist = os.listdir(dir)
    global wavlist
    wavlist = []
    for i in range(len(filelist)) : 
        strlen = len(filelist[i])
        if (filelist[i])[strlen - 4 : strlen] == '.wav' : 
            wavlist.append(filelist[i])

    if len(wavlist) == 0 : 
        print('there are no .wav files available to play. Put .wav files in the current directory and they will appear here as playable options')
        return


# select a file for playback
def select_file() : 

    global filename
    print('select a file for playback: ' )

    for i in range(len(wavlist)) : 
        print(str(i) + '. ' + wavlist[i] )
    
    validinput = 0

    while not validinput : 
        filenum = int(input('Enter the number corresponding with the file you want to play: '))
        if filenum in range(len(wavlist)) : 
            filename = wavlist[filenum]
            validinput = 1
        else :
            print('Invalid input')

    print('selected file: ' + filename)


# start playback of selected file
def start_playback() : 

    global wav
    wav = wave.open(filename, 'rb')
    wav.rewind()

    # send start signal
    ser.write(signal_play)

    # send audio file characteristics
    send_playback_info()

    # stream data
    wavToSerial()


#
def space_callback() :
    global keypress
    if keypress == '' : 
        keypress = SPACE


#
def esc_callback() : 
    global keypress
    if keypress == '' : 
        keypress = ESC


#
def pause() : 
    global state
    state = PAUSE


#
def play() : 
    global state
    state = PLAY


#
def stop() : 
    global state
    state = STOP


#
def check_keyboard_input() : 
    global keypress
    global state
    global starting_playback
    if keypress == SPACE : 

        if state == PLAY : 
            keypress = ''
            pause()
        elif state == PAUSE : 
            keypress = ''
            play()
            #edge case: stop state at beginning of playback
        elif ((state == STOP) and starting_playback) :  
            keypress = ''
            starting_playback = 0
            play()
    if keypress == ESC : 
        keypress = ''
        stop()


#send wav over serial to MCU
def wavToSerial() : 

    print('starting playback of: ' + filename)
    print('press space to play')

    global state
    state = STOP
    global starting_playback
    starting_playback = 1

    framesleft = wav.getnframes()

    #calculate max frames in one transfer
    # how many frames fit into 4096 bytes?
    if bit_depth == 16 : 
        max_frames = 2048
    elif bit_depth == 20 :
        max_frames = 1632
    elif bit_depth == 24 : 
        max_frames = 1360
    elif bit_depth == 32 : 
        max_frames = 1024
    else : 
        print('invalid bit depth: ' + str(bit_depth))
        return

    #wait for user to hit play
    while state != PLAY :
        check_keyboard_input()

    # main while loop
    state = PLAY
    while (1) : 
        check_keyboard_input()  

        if state == PLAY :
            # tell MCU that the state is PLAY
            ser.read_until(signal_ready)

            if framesleft >= max_frames :

                ser.write(max_frames.to_bytes(2, byteorder = 'little', signed = False)) # sending max bytes
                ser.reset_input_buffer()
                framesleft -= max_frames
                buf = wav.readframes(max_frames)
                ser.read_until(signal_ready)
                bytes_written = ser.write(buf)
                print(bytes_written)

            elif framesleft > 0 :
                print('two')

                #TODO test
    
                bytesleft = framesleft * (bit_depth / BITS_PER_BYTE) * num_channels
                #print('bytes left: ' + str(bytesleft))

                extraframes = 0
                remainder = 0
                # pad with zeros to send multiples of 16 bytes
                if (bytesleft % 16) != 0 : 
                    remainder = 16 - (bytesleft % 16)
                    extraframes = remainder /( num_channels * (bit_depth / BITS_PER_BYTE) )
                    framesleft += extraframes
        
                ser.write(int(framesleft).to_bytes(2, byteorder = 'little', signed = False))
                ser.reset_input_buffer()
                buf = wav.readframes(max_frames) #TODO replace with framesleft?
                ser.read_until(signal_ready)
                bytes_written = ser.write(buf)

                # write extra bytes 
                if extraframes != 0 : 
                    ser.write(bytearray(int(remainder)))

                print(bytes_written)
                stop()

            else :
                ser.write(signal_stop)
                ser.reset_input_buffer()
                print('stopping playback')
                stop()
                return

            #ser.reset_input_buffer()
        elif state == PAUSE : 
            pass # pause state is just an absence of sending data

        elif state == STOP :
            ser.read_until(signal_ready)
            ser.write(signal_stop)
            ser.reset_input_buffer()
            print('stopping playback')
            return


#main function
def main() : 

    setup()

    # choose file to play
    get_wav_file_list()

    # play loop 
    while 1 : 

        select_file()

        start_playback()


################################# MAIN #####################################

main()
