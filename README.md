# IR Control

The code in this repository allows for sending infrared signals by controling an IR LED hooked up to the GPIO pins of a Raspberry Pi (Zero W). Code from [this page](https://elinux.org/RPi_GPIO_Code_Samples#Direct_register_access) was incredibly helpful for getting set up with controlling the GPIO pins on the Pi fast enough to send valid IR messages. [This article](https://www.sbprojects.net/knowledge/ir/nec.php) offers a good explanation of the NEC protocol for IR messages, which this program uses.

I'm running [Chili Johnson](chilij.com)'s great Ghetto Automation library to allow Alexa to control the Pi and turn the LED strip in my room on and off and change the colors.

## Usage

### Sending

ir-send can be used form the command line an must be provided a binary string to transmit in infrared. Also since controlling the GPIO pins requires root access, ir-send must be run as root.

Example:

Turning my lights on or off.

```shell
sudo ./ir-send 00000000111111110000001011111101
```

### Parsing

Also included in this repo is a short script for parsing the binary IR code from a CSV of on/off data with times. The CSV is of the kind that the program [Saleae Logic](https://www.saleae.com/downloads/) splits out when exported to CSV and the snippet should start after the init signal code (on for 9ms, off for 4.5ms).

![Recorded IR Snippet](https://raw.githubusercontent.com/henrywoody/ir-control/master/img/recorded-power-signal.png)

Power Signal recorded in Saleae Logic (note activation is low).

To use this script, put another CSV in the `ir-parse/signal-data/` directory and then run `ir-parse/ir_parser.py` and proved the name of the file, or leave the file name blank (just hit enter) to get codes for all files in `ir-parse/signal-data/`.

Example:

```shell
> python3 ir_parser.py
File name: power.csv
Code: 000000000111111110000001011111101
```

## Useful Codes

These codes are valid for my Supernight LED strip:

| Command         | Code                             |
| --------------- | -------------------------------- |
| Power (on/off)  | 00000000111111110000001011111101 |
| Brightness up   | 00000000111111110011101011000101 |
| Brightness down | 00000000111111111011101001000101 |
| Red             | 00000000111111110001101011100101 |
| Dark Orange     | 00000000111111110010101011010101 |
| Orange          | 00000000111111110000101011110101 |
| Light Blue      | 00000000111111111101100000100111 |
| White           | 00000000111111110010001011011101 |
| Pink            | 00000000111111110001001011101101 |
| Violet          | 00000000111111110101100010100111 |
| Fade            | 00000000111111111110000000011111 |

