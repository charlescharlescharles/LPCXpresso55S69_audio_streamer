##Author: Charles Elliott
##Date created: March 4, 2021

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
signal_stop = b'\xFC'
baudrate = 460800  #921600
filename = '11025Hz_16bit.wav'
portname = '/dev/cu.usbserial-FTA56VXN'
keypress = ''
PAUSE = 'paused'
PLAY = 'playing'
STOP = 'stopped'
state = PLAY
SPACE = 'space'
ESC = 'esc'
buffer_size = 0
starting_playback = 0

#main function
def main() : 
    #TODO CLI stuff:
    #   check current directory for file with port name in it
    #   if file doesn't exist, get list of available ports & make selection
    #   if file does exist, read portname and try to open it
    #
    #   after port is successfully opened, prompt user to enter choose wav 
    #   file to play
    #   play file
    #   
    # parse command line args

    #TODO implement checks later
   

    

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


    # choose file to play
    dir = os.getcwd() #get the current directory
    filelist = os.listdir(dir)
    wavlist = []
    for i in range(len(filelist)) : 
        strlen = len(filelist[i])
        if (filelist[i])[strlen - 4 : strlen] == '.wav' : 
            wavlist.append(filelist[i])

    

    if len(wavlist) == 0 : 
        print('there are no .wav files available to play. Put .wav files in the current directory and they will appear here as playable options')
        return




    #when file is done playing, select a new file to play
    while 1 : 
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

        global wav
        wav = wave.open(filename, 'rb')
        wav.rewind()

        configure()
        wavToSerial()


############################ FUNCTIONS #####################################

def space_callback() :
    global keypress
    if keypress == '' : 
        keypress = SPACE
    
def esc_callback() : 
    global keypress
    if keypress == '' : 
        keypress = ESC

def pause() : 
    global state
    state = PAUSE

def play() : 
    global state
    state = PLAY

def stop() : 
    global state
    state = STOP

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

# baud check, audio characteristic data, port setup, buf size, all in one
def configure() : 

    # check baud rate
    ser.read_until(signal_ready)
    MCU_baud = int.from_bytes(ser.read(3), byteorder = 'big')
    ser.reset_input_buffer()
    if baudrate != MCU_baud : 
        ser.write(signal_fault)
        print('baud check failed. python module baudrate is ' + str(baudrate) + 'while MCU baudrate is ' + str(MCU_baud))
        return -2
    else : 
        ser.write(signal_ready)

    # send audio characteristic data
    ser.read_until(signal_ready)
    bit_depth = wav.getsampwidth() * 8
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

    # get buffer size
    ser.read_until(signal_ready)
    global buffer_size
    buffer_size = int.from_bytes(ser.read(2), byteorder = 'big')
    print("buffer size: " + str(buffer_size))
    # setup keyboard interrupts


    keyboard.add_hotkey('space', space_callback, args=(), suppress=False, timeout=1, trigger_on_release=False)

    keyboard.add_hotkey('esc', esc_callback, args=(), suppress=False, timeout = 1, trigger_on_release = False)

    #TODO configure volume 





#send wav over serial to MCU
def wavToSerial() : 

    print('starting playback of: ' + filename)
    print('press space to play')

    global state
    state = STOP
    global starting_playback
    starting_playback = 1

    framesleft = wav.getnframes()

    #wait for user to hit play
    while state != PLAY :
        check_keyboard_input()

    #send start signal
    #ser.write(signal_play)

    #fill buffers
    #for i in range(2) : 
    #    ser.read_until(signal_ready)
    #    ser.reset_input_buffer()
    #    buf = wav.readframes(buffer_size)
    #    bytes_written = ser.write(buf)
    #    print(bytes_written)
    #    if bytes_written < buffer_size : 
    #        stop()

    # main while loop
    state = PLAY
    while (1) : 
        check_keyboard_input()  

        if state == PLAY : 
            # tell MCU that the state is PLAY
            ser.read_until(signal_ready)
            if framesleft > buffer_size : 
                ser.write()
            ser.write(signal_play)
            ser.reset_input_buffer()
            buf = wav.readframes(buffer_size)
            bytes_written = ser.write(buf)
            print(bytes_written)
            if bytes_written < buffer_size : 
                stop()


        
        elif state == PAUSE : 
            pass

        elif state == STOP :
            
            ser.read_until(signal_ready)
            ser.write(signal_stop)
            print('stopping playback')
            return


################################# BODY #####################################

main()
