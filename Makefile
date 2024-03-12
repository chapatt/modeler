CFLAGS=-m64
CXXFLAGS+=-std=c++17
LDFLAGS=
LDLIBS=

ifeq ($(OS),Windows_NT)
	CC=/msys64/mingw64/bin/gcc
	CXX=/msys64/mingw64/bin/g++
	RM=/msys64/usr/bin/rm
	HEXDUMP=/msys64/usr/bin/hexdump
	GLSLC=/VulkanSDK/1.3.250.0/Bin/glslc
	CFLAGS+=-I/VulkanSDK/1.3.250.0/Include -mwindows -municode
	LDFLAGS+=-L/VulkanSDK/1.3.250.0/Lib
	LDLIBS+=-lvulkan-1 -ldwmapi
	ALL_TARGET=modeler.exe
	CFLAGS+=-DVK_USE_PLATFORM_WIN32_KHR
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC=gcc
		GLSLC=glslc
		LDLIBS+=-lvulkan -lwayland-client
		ALL_TARGET=modeler
		CFLAGS+=-DVK_USE_PLATFORM_WAYLAND_KHR
	endif
	ifeq ($(UNAME_S),Darwin)
		GLSLC=/Users/chase/VulkanSDK/1.3.250.0/macOS/bin/glslc
		CFLAGS+=-I/Users/chase/VulkanSDK/1.3.250.0/macOS/include
		LDLIBS+=-lvulkan
		ALL_TARGET=modeler.a
		CFLAGS+=-DVK_USE_PLATFORM_METAL_EXT
	endif
endif

CFLAGS+=-DENABLE_VSYNC
ifdef DEBUG
	CFLAGS+=-DDEBUG -g
	ALL_TARGET+=$(SPIRV_SHADERS)
else
	CFLAGS+=-DEMBED_SHADERS -O3
	LDFLAGS+=-static
	ALL_TARGET+=$(HEADER_SHADERS)
endif

SPIRV_SHADERS=window_border.vert.spv window_border.frag.spv triangle.vert.spv triangle.frag.spv
HEADER_SHADERS=shader_window_border.vert.h shader_window_border.frag.h shader_triangle.vert.h shader_triangle.frag.h
MODELER_OBJS=modeler.o instance.o surface.o physical_device.o device.o swapchain.o image.o image_view.o render_pass.o descriptor.o framebuffer.o command_pool.o command_buffer.o synchronization.o allocator.o input_event.o queue.o utils.o renderloop.o vma_implementation.o pipeline.o

all: $(ALL_TARGET)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

modeler: $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o xdg-shell-protocol.o imgui.a
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o imgui.a xdg-shell-protocol.o $(LDLIBS)

modeler.exe: $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o imgui.a
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler.exe $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o imgui.a $(LDLIBS)
	
modeler.a: $(MODELER_OBJS) modeler_metal.o surface_metal.o
	$(AR) rvs $@ $(MODELER_OBJS) modeler_metal.o surface_metal.o

main_wayland.o: src/main_wayland.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c src/main_wayland.c

modeler.o: src/modeler.c
	$(CC) $(CFLAGS) -c src/modeler.c

%.vert.spv: src/shaders/%.vert.glsl
	$(GLSLC) -fshader-stage=vert $< -o $@

%.frag.spv: src/shaders/%.frag.glsl
	$(GLSLC) -fshader-stage=frag $< -o $@

$(HEADER_SHADERS): shader_%.h: %.spv
	./hexdump_include.sh fragmentShaderBytes fragmentShaderSize $< > $@

imgui.a: cimgui.o cimgui_impl_vulkan.o imgui.o imgui_demo.o imgui_draw.o imgui_impl_modeler.o imgui_impl_vulkan.o imgui_tables.o imgui_widgets.o
	$(AR) rvs $@ cimgui.o cimgui_impl_vulkan.o imgui.o imgui_demo.o imgui_draw.o imgui_impl_modeler.o imgui_impl_vulkan.o imgui_tables.o imgui_widgets.o
cimgui.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/cimgui.cpp
cimgui_impl_vulkan.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/cimgui_impl_vulkan.cpp
imgui.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui.cpp
imgui_demo.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui.cpp src/imgui/imgui_demo.cpp
imgui_draw.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui_draw.cpp
imgui_impl_modeler.o: src/imgui/imgui_impl_modeler.c
	$(CC) $(CFLAGS) -c src/imgui/imgui_impl_modeler.c
imgui_impl_vulkan.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui_impl_vulkan.cpp
imgui_tables.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui_tables.cpp
imgui_widgets.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/imgui/imgui_widgets.cpp

xdg-shell-protocol.o: xdg-shell-protocol.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c xdg-shell-protocol.c

xdg-shell-protocol.c:
	wayland-scanner private-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c

xdg-shell-client-protocol.h:
	wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h

vma_implementation.o: src/vk_mem_alloc.h
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c src/vma_implementation.cpp

clean:
	$(RM) -rf modeler modeler.exe modeler.a main_wayland.o main_win32.o \
		modeler_win32.o modeler_wayland.o modeler_metal.o modeler.o \
		instance.o physical_device.o device.o utils.o utils_win32.o \
		allocator.o surface.o surface_win32.o surface_wayland.o surface_metal.o \
		swapchain.o image.o image_view.o render_pass.o descriptor.o pipeline.o framebuffer.o \
		command_pool.o command_buffer.o synchronization.o \
		input_event.o queue.o \
		xdg-shell-protocol.o xdg-shell-client-protocol.h  xdg-shell-protocol.c \
		window_border.vert.spv window_border.frag.spv shader_window_border.vert.h shader_window_border.frag.h \
		triangle.vert.spv triangle.frag.spv shader_triangle.vert.h shader_triangle.frag.h \
		imgui.a cimgui.o cimgui_impl_vulkan.o imgui.o imgui_demo.o imgui_draw.o imgui_impl_modeler.o imgui_impl_vulkan.o imgui_tables.o imgui_widgets.o \
		vma_implementation.o \
		renderloop.o
