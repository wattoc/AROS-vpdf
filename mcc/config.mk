
ifeq "$(shell test -f /gg/bin/ppc-morphos-gcc-3.4.6 && echo YES)" "YES"
	CC3 = i386-aros-gcc
else
	CC3=i386-aros-gcc
endif

CC= i386-aros-gcc
AS= i386-aros-as
AR= i386-aros-ar rcs
LD= i386-aros-ld

CFLAGS = -g -O2 -Wall -O2 -fno-strict-aliasing -DMUI_OBSOLETE
CFLAGS_LESS = -g -O2 -Wall -O2 -fno-strict-aliasing
INCLUDES = -I../showgirls/system -I.. -I../exif -I../../showgirls -I../../lairecvs/morphos/morphoswb/apps/showgirls/system/ -I../../lairecvs/morphos/morphoswb/apps/showgirls/ -I../aros/include -DMUI_OBSOLETE -I/home/craig/aros-linux-i386-dbg/bin/linux-i386/AROS/Developer/include/
LFLAGS=

# For building with the public SDK
INCLUDES += -I../include

