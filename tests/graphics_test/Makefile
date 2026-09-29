TARGET = graphics_test
OBJS = main.o

PSPSDK = $(shell psp-config --pspsdk-path)
PSPBIN = $(shell psp-config --psp-prefix)/bin

CFLAGS = $(shell $(PSPBIN)/sdl2-config --cflags) -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = $(shell $(PSPBIN)/sdl2-config --libs)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = MiniDayZ-PSP Graphics Test

include $(PSPSDK)/lib/build.mak