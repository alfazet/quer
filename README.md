# Quer
A QR code generator made entirely from scratch, following the [ISO-18004](https://www.iso.org/standard/83389.html) standard.

## Usage
- By default, quer reads input from stdin and writes the raw bytes of the output image to stdout.
- Quer follows the *UNIX philosophy*. So, to generate a QR code out of the content of `input.txt` and save it as `qr.png`, you can run `cat input.txt | quer > qr.png`.
- Input/output files can be passed as CLI arguments with `-i/-o`, e.g. `quer -i input.txt -o qr.png`.
- The error correction level of the code can be modified. Available levels are *low* `-l` (default), *medium* `-m`, *quartile* `-q` and *high* `-h`. Keep in mind that the higher the error correction level, the lower the capacity of the QR code.
- Changing the resolution (the width/height of one module (subsquare) of the code, in pixels, `ppm = 20` by default) is possible with `-p ppm`.

## Installation
```
# git clone https://github.com/alfazet/quer
# cd quer
# make install
```
The only "external" dependency needed is `libpng`. 

To uninstall, run `make uninstall`.
