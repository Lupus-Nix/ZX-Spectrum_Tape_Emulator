set fl=..\Firmware\te-ru.hex
avrdude\avrdude.exe -Cavrdude/avrdude.conf -v -V -cusbasp -patmega16 -Uflash:w:%fl%:i 
