arm-wince-pe-windres FreeSCI.rc -o freesci.rc.o

rem arm-wince-pe-gcc -include fixincl.h -mcpu=strongarm1100 -Dmain=main -O2 -I/usr/local/include/SDL -Dmain=SDL_main -include fixincl.h -mcpu=strongarm1100 -Dmain=main -O2  -o freesci.exe wince/freesci.rc.o config.o main.o engine/libsciengine.a gfx/libscigraphics.a         gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a  sound/libscisound.a scicore/libscicore.a -lm -L/usr/local/lib -lSDL -lg -lgcc -lsdlmain -lsdl -lsupc++ -lcoredll -lwinsock -laygshell -lpthread -L/usr/local/lib -lSDL -L/usr/local/lib -lSDL -lm -Wl,-s -staticlibs
