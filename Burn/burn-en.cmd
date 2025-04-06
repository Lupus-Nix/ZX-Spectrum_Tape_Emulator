set fl=..\Firmware\te-en.hex
avrdude\avrdude.exe -Cavrdude/avrdude.conf -v -V -cusbasp -patmega16 -Uflash:w:%fl%:i 
