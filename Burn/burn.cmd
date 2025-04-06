::set fl=%1
set fl=..\Firmware\te.hex
avrdude\avrdude.exe -Cavrdude/avrdude.conf -v -V -cusbasp -patmega16 -Uflash:w:%fl%:i 
