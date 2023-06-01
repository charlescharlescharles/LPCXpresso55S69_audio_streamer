# LPCXpresso55S69 audio streamer
Project streams .wav audio files from a PC to the LLPC55S69-EVK and out the board audio jack. 

## Project file structure
board/ contains the MCUXpresso project required to program the LPCXpresso55S69.
pc/ contains the application used to stream audio data to the board. 

## Hardware setup
First connect the board to the PC via a UART cable. On the board end, attach the pins to the GND, TX and RX lines of header P24. On the PC end, plug the cable into any functional USB port. Plug headphones into the audio out port of the board. 

## Programming the board
Open the project audio_player in MCUXpresso. The SDK version used in developing the project is 2.9.0 - SDK can be generated on NXP's website. Build the project and enter a debug session. 

## Running the pc application 
TODO talk about setting up serial port. 

In the python module, hit space to play the audio file. Run the python script (you may need to use the sudo command). Make selections for the COM port and the file you wish to play. (To add more available files to the playable options, copy .wav files into the same directory as the python script python_module.py.)

## Supported audio file formats
- Currently only MONO audio files are supported. 
- Bit depths supported are: 16, 20, 24 and 32 bits. 
- Sampling frequencies supported are: 8, 11.025, 12, 16, 24, & 32 kHz.
- If there are with playback at higher bit depths and sampling frequencies, try increasing the baud rate. 

## Notes for future development
Currenty app only plays one file before stopping, and then you have to restart the app. I want to implement an infinite while loop that lets you choose another file to play as soon as playback of one file stops. 

Because of how the board is set up I can only use one DMA controller for this project. If I had two DMA controllers I could use the MCU to process the audio data. One idea I've had so far is to use a linear interpolation technique to change the bit depth or sampling frequency of the input file, allowing for wider support of audio file formats. 

Additionally I could use a lossless compression technique like Huffman Encoding to decrease the size of data transfers from PC to MCU, potentially allowing for higher sampling frequency input. 



