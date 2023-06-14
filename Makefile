CC=/msys64/mingw64/bin/gcc
CFLAGS=-I/VulkanSDK/1.3.250.0/Include -mwindows -municode -m64 -g
LDFLAGS=-L/VulkanSDK/1.3.250.0/Lib
LDLIBS=-lvulkan-1

all: modeler

debug: CFLAGS += -DDEBUG -g
debug: modeler

modeler: main_win32.o modeler_win32.o
	${CC} $(CFLAGS) $(LDFLAGS) -o modeler.exe main_win32.o modeler_win32.o $(LDLIBS)

main_win32.o: src/main_win32.c src/modeler_win32.h
	${CC} $(CFLAGS) -c src/main_win32.c

modeler_win32.o: src/modeler_win32.c src/modeler_win32.h
	${CC} $(CFLAGS) -c src/modeler_win32.c

clean:
	rm -rf main_win32 main_win32.o modeler_win32.o