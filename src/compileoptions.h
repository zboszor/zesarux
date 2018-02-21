
#ifndef OPTIONS_H
#define OPTIONS_H


#define COMPILE_STDOUT
#define COMPILE_SIMPLETEXT
#define COMPILE_CURSES
#define COMPILE_XWINDOWS
#define USE_XEXT
#define USE_XVIDMODE
#define USE_PTHREADS
#define USE_COCOA
#define COCOA_OPENGL
#define COMPILE_COREAUDIO
#define EMULATE_MEMPTR
#define EMULATE_VISUALMEM
#define EMULATE_CPU_STATS
#define EMULATE_CONTEND
#define PUTPIXELCACHE

#define COMPILATION_DATE "Wed Feb 21 11:16:40 CET 2018"
#define COMPILATION_SYSTEM "Darwin"
#define CONFIGURE_OPTIONS "./configure --enable-memptr --enable-visualmem --enable-cpustats --enable-cocoa-opengl"
#define COMPILE_VARIABLES " COMPILE_STDOUT COMPILE_SIMPLETEXT COMPILE_CURSES COMPILE_XWINDOWS USE_XEXT USE_XVIDMODE USE_PTHREADS USE_COCOA COCOA_OPENGL COMPILE_COREAUDIO EMULATE_MEMPTR EMULATE_VISUALMEM EMULATE_CPU_STATS EMULATE_CONTEND PUTPIXELCACHE"
#define COMPILE_INITIALCFLAGS ""
#define COMPILE_INITIALLDFLAGS ""
#define COMPILE_FINALCFLAGS " -Wall -Wextra -fsigned-char"
#define COMPILE_FINALLDFLAGS " -lncurses -lX11 -L/usr/X11R6/lib -L/opt/X11/lib -lXext -lXxf86vm -lpthread -framework CoreAudio -framework AudioUnit -framework CoreServices -framework Foundation -framework AppKit -framework OpenGL"
#define INSTALL_PREFIX "/usr/local"

#define LINES_SOURCE 250466

#define BUILDNUMBER "1519208195"


#endif

