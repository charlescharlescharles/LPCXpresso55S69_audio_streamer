Program name: audio_player
Author: Charles Elliott
Date: April 2021


Application description:
    This application was created for A2e Technologies. Its intended use is to 
    stream .wav audio files from a PC, to the LLPC55S69-EVK
    and out the board audio out. 

General use:
    First connect the board to the PC via a UART cable. On the board end, attach 
    the pins to the GND, TX and RX lines of header P24. On the PC end, plug 
    the cable into any functional USB port. If not on a mac system, you may have
    to find the COM port and manually enter it into the python script for the 
    app to work. 

    Plug headphones into the audio out port of the board. 


    Open the project audio_player in MCUXpresso. The SDK version used in developing
    the project is 2.9.0 - SDK can be generated on NXP's website. 
    Build the project and enter a debug session. 

    Run the python script (you may need to use the sudo command). Make selections for the COM port and the file you wish to play. 
    (To add more available files to the playable options, copy .wav files into the same 
    directory as the python script python_module.py.)

    Start the debug session. 

    In the python module, hit space to play the audio file. 


Hardware notes:
    Application was written on a 2019 Macbook Pro running macOS 10.15.7. 
    The board used is the LPCXpresso55S69 from NXP. 


Supported audio file formats:
    Currently only MONO audio files are supported. 

    Bit depths supported are: 16, 20, 24 and 32 bits. 

    Sampling frequencies supported are: 8, 11.025, 12, 16, 24, & 32 kHz.

    If there are with playback at higher bit depths and sampling frequencies, try 
    increasing the baud rate. 

Notes for future development:

    Currenty app only plays one file before stopping, and then you have to restart 
    the app. I want to implement an infinite while loop that lets you choose another
    file to play as soon as playback of one file stops. 

    Because of how the board is set up I can only use one DMA controller for this project. 
    If I had two DMA controllers I could use the MCU to process the audio data. One 
    idea I've had so far is to use a linear interpolation technique to change the bit depth
    or sampling frequency of the input file, allowing for wider support of audio file formats. 

    Additionally I could use a lossless compression technique like Huffman Encoding to decrease
    the size of data transfers from PC to MCU, potentially allowing for higher sampling 
    frequency input. 



