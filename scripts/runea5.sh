#!/bin/bash
obj/sp --ea5 $1
rm -rf out.ea5
~/ti99/xdt99/xas99.py -R -i -q -L out.lst out.a99 -o out.ea5
~/ti99/emul99/bin/emul99 ~/ti99/emul99/bin/common.cfg mem_ext=1 disksim_dsr=~/ti99/emul99/roms/disksim.bin disksim_dir=~/src/statpascal cycle_count=a000-ffff cart_groms=~/ti99/emul99/modules/Ed-AssmG.Bin disksim_text=1
