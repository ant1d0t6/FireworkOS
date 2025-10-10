## License
This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

# FireworkOS
FireworkOS is an OS for boards from the Arduino family.
The SD card must be formatted in FAT32.

To prepare the base files, the card must have these files:
```
/mksys
/setup/help.bs
/setup/reboot.bs
/setup/shutdown.bs
```
To install the base OS, type:
```
/mksys
```
The connection is made via a tty port at a speed of 115200 baud.
### Pins for SD card:
ESP32 -- 15
Other boards -- 4


## Supported boards: (firmware has already been compiled for them)
* Arduino Mega 2560
* Arduino Due
* ESP32 Wroom
* Arduino Uno R4 Wi-FI

## BlackScript
Programs for it can be made in our BlackScript language.
* [Compiler for BS](BSC/bsc.py)
* [Syntax](BSSyntax.md)
