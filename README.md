# Cassettewright
*A tiny C application for reading and writing to and from cassette tape audio (signed 16-bit PCM, mono, 44100Hz, raw, no WAV header), via STDIN and STDOUT.* 
**WARNING: The audio this program generates is extremely loud. Write to file and open in something like Audacity where you can control the volume if you want to listen to it.**  

## Example Video 
* [Check It Out](https://youtu.be/0O2jbT4veNA)


## Requirements
* CMake
* Some sort of C build tooling. Your pick! I like LLVM/clang.

## Building 
* `git clone git@github.com:DrMelon/cassettewright.git --recusrive` (`--recursive` is **required**). 
* `cd cassettewright`
* `cmake .`
* `cmake --build .` 

## Running 
* `./cassettewright --help` -- Display usage.

### Writing to cassette tape: 
* `./cassettewright -w` -- Write mode; writes everything that comes in on STDIN to STDOUT as PCM audio until EOF.  
    * Example: `cat myfile.txt | ./cassettewright -w > cool_audio_file.raw`
    * Example (**LOUD**): `cat myfile.txt | ./cassettewright -w | aplay`  

### Reading from cassette tape:
* `./cassettewright -r` -- Read mode; writes everything that comes in on STDIN (as long as it is PCM data formatted correctly) back to normal bytes on STDOUT until EOF. 
    * Example: `cat cool_audio_file.raw | ./cassettewright -r > myfile.txt`
    * Example: `arecord | ./cassettewright -r > myfile.txt` 

### Verbose modes:
* `./cassettewright -rv` -- When reading from audio, this will output information about if the file achieved polarity lock, if it achieves or loses bit synchronization, and if it finds the header all to STDERR. 
* `./cassettewright -rx` -- When reading from audio, this will output the stream of bits discovered after polarity lock to STDERR. 

## Format 
* A pulse is 8 PCM samples long. 
 Bits are encoded as follows:
    * 1: Two pulses of positive followed by two pulses of negative. 
    * 0: One pulse of positive followed by one pulse of negative. 
* Bytes are encoded as follows:
    * `1xxxxxxxx0`, where `x` are the bits of each byte. The bookending 1 and 0 are used to understand where in the bitstream we are reading the signal, and is called "Bit-Sync".
* Data is encoded like this: 
    * 800 cycles of pulses, in a repeating positive-negative-negative-negative pattern. This is the *Polarity Lock*. 
    * 64 bytes of 0xFF. This is the *Lead-in*.
    * A 4-byte header, reading `0x04 0x20 0x06 0x09`. *If this is missed, there will be no output.* 
    * The actual data bytes. 

## Troubleshooting 
* *It fails to acquire polarity lock.* - Your signal may be too noisy, too weak, or not actually of the correct format. Ensure your audio data is signed 16-bit mono PCM @ 44100Hz with *no WAV header*. 
* *It achieves polarity lock, but then just repeatedly finds and loses bit-sync without outputting anything.* - Your signal is too noisy or too weak. Either amplify your signal using a tool like Audacity, or turn up the volume and tone dials on your cassette player. 
* *It's outputting stuff, but there are corrupted or missing areas.* - Your signal is either too noisy, too weak, or is subject to **DC Bias**. Normalize or severely amplify your audio with hardware or software and see if it improves. If it does not improve, it may be that you have recorded it onto the tape too quietly or too loudly. **Record onto the tape at about 70-75% for best results.** 


## Credits 
* [windytan](https://github.com/windytan/ctape) - Heavily inspired by ctape.
* [cofyc](https://github.com/cofyc/argparse) - Argparse library for command line flag parsing.
