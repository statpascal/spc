#!/bin/bash
obj/sp --cart $1
~/ti99/xdt99/xas99.py -R -b -q -L out.lst out.a99 -o cart.bin
~/ti99/emul99/bin/emul99 ~/ti99/emul99/bin/common.cfg mem_ext=1 disksim_dsr=~/ti99/emul99/roms/disksim.bin disksim_dir=~/src/statpascal cycle_count=6000-7fff cart_rom=cart.bin disksim_text=1
