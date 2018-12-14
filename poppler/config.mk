
#CC= ppc-morphos-gcc
CC= i386-aros-gcc
#CXX= ppc-morphos-g++-2.95.3
CXX= i386-aros-g++
AS= i386-aros-as
AR= i386-aros-ar rcs
LD= i386-aros-ld

# do NOT include optimization level here
CFLAGS = -DHAVE_CONFIG_H -D_NO_PPCINLINE -DHAVE_CONFIG_H -Wno-write-strings -Wall -O2 -fno-strict-aliasing 
CPPFLAGS = -DHAVE_CONFIG_H -D_NO_PPCINLINE -DHAVE_CONFIG_H -Wno-write-strings -Wall -O2 -fno-strict-aliasing
CFLAGS_LESS =  -Wall -O2 -fno-strict-aliasing
GLOBAL_INCLUDES = -I.. -I. -I../poppler -Iincludes/ -I../../ -I../../../../libs/cairo/cairo-src/MorphOS/include/cairo -I../../../../libs/cairo/cairo-src/MorphOS/include \
 -I../../../../../include  -I../../../../libs/freetype/library/include -I../../../../libs/fontconfig/MorphOS/include -I../../../../libs/freetype/include  -I/home/craig/aros-linux-i386-dbg/bin/linux-i386/AROS/Developer/include/
LFLAGS=





