avrdude\avrdude.exe -C avrdude\avrdude.conf -v -V -patmega16 -cusbasp -Pusb -D -Uflash:r:read.hex:i
avrdude\avrdude.exe -C avrdude\avrdude.conf -v -V -patmega16 -cusbasp -Pusb -D -Ueeprom:r:eeread.hex:i

::-F
