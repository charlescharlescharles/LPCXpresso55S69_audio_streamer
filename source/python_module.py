##Author: Charles Elliott
##Date created: March 4, 2021

import wave
import serial
import time

############################ VARIABLES #####################################
signalToWrite = b'\xFF'
signalToFault = b'\xFE'
baudrate = 460800  #921600

filename = '11025Hz_16bit.wav'
portname = '/dev/cu.usbserial-FTA56VXN'

#main function
def main() : 

    #retVal = wavToSerial('example_tones.wav', '/dev/cu.usbmodemDRAWCQOQ2')
    #retVal = wavToSerial('example_tones.wav', '/dev/cu.usbserial-CRAVBQEQ2')
    retVal = wavToSerial(filename , portname)

    if retVal == -1 :
        print('file is not .wav type. Please use .wav files only')
    elif retVal == -2 : 
        print('error: baud rates for MCU and python script are not equal')
    else :  
        print('file reading done')

############################ FUNCTIONS #####################################

#returns -1 if file is not correct type
def wavToSerial( filename, portname ):

    #check that file is .wav type
    strlen = len(filename)
    if filename[strlen - 4:strlen] != '.wav':
        return -1
    else:
        print('intput file is correct type')



    #open .wav file for reading
    #don't forget to close it!   
    wavRead = wave.open(filename, 'rb')
    wavRead.rewind() # set file marker to beginning of audio stream
    numFrames = wavRead.getnframes() # for loop later
    #open serial port
    ser = serial.Serial(portname)
    print(ser.name) #check port


    #configure port
    ser.baudrate = baudrate
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE

    #check that baud rates are the same
    ser.read_until(signalToWrite)
    MCU_baud = int.from_bytes(ser.read(3), byteorder = 'big' )
    print(MCU_baud)
    ser.reset_input_buffer()
    if (baudrate != MCU_baud) :
        ser.write(signalToFault)
        print('baud check failed')
        return -2
    else : 
        print('baud check passed')
        ser.write( signalToWrite )

    # send bit depth and sampling frequency over UART
    ser.read_until(signalToWrite)
    bit_depth = (wavRead.getsampwidth() * 8) # wavRead fn returns in bytes
    print('bit depth: ' + str(bit_depth))

    bytes_bit_depth = bit_depth.to_bytes(1, byteorder = 'little', signed = True)
    sampling_frequency = wavRead.getframerate()
    print('sampling frequency: ' + str(sampling_frequency))

    bytes_sampling_frequency = sampling_frequency.to_bytes(4, byteorder = 'little', signed = False)

    ser.write(bytes_bit_depth)
    ser.write(bytes_sampling_frequency)
    print('waiting for signal')

    # wait for the signal to send a packet
    while 1 :

        ser.read_until(signalToWrite)
        #print('signal received')
        numBytes = int.from_bytes(ser.read(2), byteorder = 'big')
        #print(int.from_bytes(numBytes, byteorder = 'big'))
        ser.reset_input_buffer()

        buf = wavRead.readframes(numBytes)
        #ser.write(b'\xFF')
        bytes_written = ser.write(buf)
        print(bytes_written)


        #TODO way to break out of while loop to clean up

    #tidy up
    wavRead.close()
    ser.close()
    return 0



################################# BODY #####################################

main()
