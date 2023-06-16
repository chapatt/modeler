CC=/msys64/mingw64/bin/gcc
CFLAGS=-I/VulkanSDK/1.3.250.0/Include -mwindows -municode -m64 -g
LDFLAGS=-L/VulkanSDK/1.3.250.0/Lib
LDLIBS=-lvulkan-1

all: modeler

debug: CFLAGS += -DDEBUG -g
debug: modeler

modeler: main_win32.o modeler_win32.o instance.o surface_win32.o physical_device.o device.o utils.o
	${CC} $(CFLAGS) $(LDFLAGS) -o modeler.exe main_win32.o modeler_win32.o instance.o surface_win32.o physical_device.o device.o utils.o $(LDLIBS)

main_win32.o: src/main_win32.c src/modeler_win32.h
	${CC} $(CFLAGS) -c src/main_win32.c

modeler_win32.o: src/modeler_win32.c src/modeler_win32.h src/instance.h src/surface_win32.h src/physical_device.h src/device.h
	${CC} $(CFLAGS) -c src/modeler_win32.c

instance.o: src/instance.c src/instance.h
	${CC} $(CFLAGS) -c src/instance.c

surface_win32.o: src/surface_win32.c src/surface_win32.h
	${CC} $(CFLAGS) -c src/surface_win32.c

physical_device.o: src/physical_device.c src/physical_device.h
	${CC} $(CFLAGS) -c src/physical_device.c

device.o: src/device.c src/device.h
	${CC} $(CFLAGS) -c src/device.c

utils.o: src/utils.c src/utils.h
	${CC} $(CFLAGS) -c src/utils.c

clean:
	rm -rf modeler.exe main_win32.o modeler_win32.o instance.o surface_win32.o physical_device.o device.o utils.o