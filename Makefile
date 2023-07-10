CFLAGS=-m64 -g
LDFLAGS=
LDLIBS=

ifeq ($(OS),Windows_NT)
	CC=/msys64/mingw64/bin/gcc
	CFLAGS+=-I/VulkanSDK/1.3.250.0/Include -mwindows -municode
	LDFLAGS+=-L/VulkanSDK/1.3.250.0/Lib
	LDLIBS+=-lvulkan-1
	ALL_TARGET=modeler.exe
	RM=/msys64/usr/bin/rm
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC=gcc
		LDLIBS+=-lvulkan -lwayland-client
		ALL_TARGET=modeler
	endif
	ifeq ($(UNAME_S),Darwin)
		CC=clang
		ALL_TARGET=modeler.a
	endif
endif

all: $(ALL_TARGET)

debug: CFLAGS += -DDEBUG -g
debug: all

renderloop.o: src/renderloop.c src/renderloop.h
	$(CC) $(CFLAGS) -c src/renderloop.c

modeler: main_wayland.o modeler_wayland.o instance.o surface_wayland.o physical_device.o device.o swapchain.o utils.o xdg-shell-protocol.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o modeler main_wayland.o modeler_wayland.o instance.o surface_wayland.o physical_device.o device.o swapchain.o utils.o xdg-shell-protocol.o $(LDLIBS)

modeler.exe: main_win32.o modeler_win32.o instance.o surface_win32.o physical_device.o device.o swapchain.o utils.o renderloop.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o modeler.exe main_win32.o modeler_win32.o instance.o surface_win32.o physical_device.o device.o swapchain.o utils.o renderloop.o $(LDLIBS)

main_wayland.o: src/main_wayland.c src/modeler_wayland.h xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c src/main_wayland.c

main_win32.o: src/main_win32.c src/modeler_win32.h
	$(CC) $(CFLAGS) -c src/main_win32.c

modeler_wayland.o: src/modeler_wayland.c src/modeler_wayland.h src/instance.h src/surface_wayland.h src/physical_device.h src/device.h
	$(CC) $(CFLAGS) -c src/modeler_wayland.c

modeler_win32.o: src/modeler_win32.c src/modeler_win32.h src/instance.h src/surface_win32.h src/physical_device.h src/device.h
	$(CC) $(CFLAGS) -c src/modeler_win32.c

instance.o: src/instance.c src/instance.h
	$(CC) $(CFLAGS) -c src/instance.c

surface_wayland.o: src/surface_wayland.c src/surface_wayland.h
	$(CC) $(CFLAGS) -c src/surface_wayland.c

surface_win32.o: src/surface_win32.c src/surface_win32.h
	$(CC) $(CFLAGS) -c src/surface_win32.c

physical_device.o: src/physical_device.c src/physical_device.h
	$(CC) $(CFLAGS) -c src/physical_device.c

device.o: src/device.c src/device.h
	$(CC) $(CFLAGS) -c src/device.c

swapchain.o: src/swapchain.c src/swapchain.h
	$(CC) $(CFLAGS) -c src/swapchain.c

utils.o: src/utils.c src/utils.h
	$(CC) $(CFLAGS) -c src/utils.c

xdg-shell-protocol.o: xdg-shell-protocol.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c xdg-shell-protocol.c

xdg-shell-protocol.c:
	wayland-scanner private-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c

xdg-shell-client-protocol.h:
	wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h

clean:
	$(RM) -rf instance.o physical_device.o device.o utils.o \
		modeler.exe main_win32.o modeler_win32.o surface_win32.o \
		modeler main_wayland.o  modeler_wayland.o  surface_wayland.o \
		xdg-shell-protocol.o xdg-shell-client-protocol.h  xdg-shell-protocol.c