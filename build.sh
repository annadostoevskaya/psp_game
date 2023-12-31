#!/bin/bash
# File: build.sh
# Author: github.com/annadostoevskaya
# Date: 08/29/2023 21:26:46
# Last Modified Date: 09/17/2023 02:27:59


set -ex

SRC=$(pwd)
BUILD_DIR="debug"
PSPDEV=$(psp-config --pspdev-path)
PSPSDK=$(psp-config --pspsdk-path)

[ ! -d "$SRC/$BUILD_DIR" ] && (mkdir "$SRC/$BUILD_DIR")

pushd "./$BUILD_DIR"

psp-g++ -I$PSPDEV/psp/include -I$PSPSDK/include \
    -G0 -Wall -Wextra -Werror \
    -DDEBUG_BUILD \
    -D_PSP_FW_VERSION=150 -g -c -o main.o $SRC/psp_main.cpp

psp-g++ -I$PSPDEV/psp/include -I$PSPSDK/include \
    -g -G0 -Wall -D_PSP_FW_VERSION=150  \
    -L$PSPDEV/psp/lib -L$PSPSDK/lib -specs=$PSPSDK/lib/prxspecs \
    -Wl,-q,-T$PSPSDK/lib/linkfile.prx  -Wl,-zmax-page-size=128  \
    main.o $PSPSDK/lib/prxexports.o  \
    -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspnet -lpspnet_apctl -lpspgu -o main.elf

psp-fixup-imports main.elf
psp-prxgen main.elf main.prx
mksfoex -d MEMSIZE=0  'PSP_DESMUME' PARAM.SFO
pack-pbp EBOOT.PBP PARAM.SFO NULL  \
    NULL NULL NULL  \
    NULL  main.prx NULL

popd

