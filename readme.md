## MLX90621
Using the MLX90621 requires quite some calculations and seems hardly
possible on an Arduino Uno or something with an ATmega328 controller.
I decided to start with calculating ambient temperature without using
floating-point values, instead doing ``exact division''.

As I used an 8MHz Uno-variant, internal RC-operated, I felt the need
for having the EEPROM values, 256Bytes, available so RAM started to be
a bottleneck right away.
Luckily my sensor is the D-type, so fewer calculations were needed.

As bonuses I added a tested square-root routine and float-to-string
converter with the project, using about 10kB of flash and 1kB of RAM.
