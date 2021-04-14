##Author: Charles Elliott
##Date created: March 4, 2021

import wave
import serial
import time
import keyboard

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
    
    global ser
    ser = serial.Serial(portname)
    print('opening port: ' + ser.name)
    ser.baudrate = baudrate
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE


    #TODO implement checks later
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
    if keypress == SPACE : 
        if state == PLAY : 
            keypress = ''
            pause()
        elif state == PAUSE : 
            keypress = ''
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

    print("yoinks")
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

    #send start signal
    #ser.read_until(signal_ready)
    #ser.write(signal_play)


    #fill buffers
    #for i in range(2) : 
     #   ser.read_until(signal_ready)
     ##   ser.reset_input_buffer()
     #   buf = wav.readframes(buffer_size)
     #   bytes_written = ser.write(buf)
     #   print(bytes_written)
     #   if bytes_written < buffer_size : 
     #       stop()

    state = PLAY
    # main while loop
    while (1) : 
        check_keyboard_input()  

        if state == PLAY : 
            # tell MCU that the state is PLAY
            #ser.write(signal_play)
            ser.read_until(signal_ready)
            ser.reset_input_buffer()
            buf = wav.readframes(buffer_size)
            bytes_written = ser.write(buf)
            print(bytes_written)
            if bytes_written < buffer_size : 
                stop()


        
        elif state == PAUSE : 
            pass

        elif state == STOP :
            ser.write(signal_stop)
            print('stopping playback')
            return


################################# BODY #####################################

main()
