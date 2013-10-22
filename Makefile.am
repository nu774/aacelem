ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = aacelem

aacelem_SOURCES = \
    src/bs.c                 \
    src/main.c               \
    faad2/libfaad/bits.c     \
    faad2/libfaad/common.c   \
    faad2/libfaad/decoder.c  \
    faad2/libfaad/error.c    \
    faad2/libfaad/huffman.c  \
    faad2/libfaad/mp4.c      \
    faad2/libfaad/specrec.c  \
    faad2/libfaad/syntax.c

AM_CPPFLAGS = -Ifaad2/include -Ifaad2/libfaad -DLC_ONLY_DECODER 

if XX_PLATFORM_POSIX
    aacelem_SOURCES += src/compat_posix.c
endif

if XX_PLATFORM_WIN32
    aacelem_SOURCES += src/compat_win32.c
endif

if XX_NO_GETOPT
    aacelem_SOURCES += getopt/getopt.c
    AM_CPPFLAGS += -Igetopt
endif
