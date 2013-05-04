Osiris Cutdown System
======

This repository contains the source code for the main parts of the Osiris cutdown system, as used in Project Horus launches.

The Cutdown PCB design is a direct implementation of the circuit described in the RF22 library, but with a FET that allows passing current through a nichrome wire, and hence cutting the payload away from the balloon.
I've made some modifications to the RF22 library, mainly exposing some private functions which I find useful. 

The folder 'OsirisPayload' contains (surprisingly enough) the software that runs on the Osiris payload. The payload's operational cycle has it transmit a RTTY string giving system status, then listen for commands for 5 seconds.

Commands take the form:
$X <Comment>

Where X is a command number.

Currently, the command set is as follows:
* 0 - Do nothing, just pass the comment onto the downlinked RTTY string.
* 1 - Arm the payload for cutdown. This lasts for 5 cycles (about a minute)
* 2 - Fire FET for 2 seconds.
* 3 - Fire FET for 4 seconds.
* 4 - Fire FET for 6 seconds.
* 5 - Fire FET for 10 seconds.
* A - Increase listen time to 20 seconds.
* B - Set listen time to 5 seconds. (default)
* C - Set downlink RTTY baud rate to 50 baud.
* D - Set downlink RTTY baud rate to 300 baud. (default)
* E - Set downlink to RTTY (default)
* F - Set downlink to Morse ident only.


Uplink packets can be generated using the GroundStation code. Note that the RFM22B can only output 100mW, which is often not enough for a decodable upink.
To get around this problem, a power amplifier can be used, or packets can be recorded/replayed using a 70cm SSB transceiver. 
The uplink packets are modulated using 500 baud GMSK, and as such just fit within the passband of a commercial 70cm amateur rig, like an Icom IC-7000. 
To ensure the packet is fully within the passband when recording, use the 'Send message with RTTY preamble' option in the ground station code, and tune such that the RTTY signal is centered over 1500Hz on dl-fldigi.
Recorded packets can then be played back through the radio at much higher power than the RFM22B is capable of. 

All code is given without guarantee. If you're going to use it, make sure to do a lot of testing before flying it. 
