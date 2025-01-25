CFLAGS=
CXXFLAGS+=-std=c++17
LDFLAGS=
LDLIBS=

ifdef ANDROID
	NDK=$(LOCALAPPDATA)/Android/Sdk/ndk/28.0.12674087
	TOOLCHAIN=$(NDK)/toolchains/llvm/prebuilt/windows-x86_64
	CC=$(TOOLCHAIN)/bin/clang
	CXX=$(TOOLCHAIN)/bin/clang++
	AR=$(TOOLCHAIN)/bin/llvm-ar
	CP=/msys64/usr/bin/cp
	HEXDUMP=/msys64/usr/bin/hexdump
	SED=sed
	GLSLC=/VulkanSDK/1.3.296.0/Bin/glslc
	TARGET=aarch64-linux-android
	API=21
	LDLIBS+=-lvulkan
	ALL_TARGET=modeler_android.a imgui.a
	CFLAGS+=--target=$(TARGET)$(API)
	CFLAGS+=-I$(TOOLCHAIN)/sysroot/usr/include
	CFLAGS+=-DVK_USE_PLATFORM_ANDROID_KHR
	CFLAGS+=-DANDROID
	CFLAGS+=-fPIC
else ifdef IOS
	SED=gsed
	CP=cp
	GLSLC=/Users/chase/VulkanSDK/1.3.296.0/iOS/bin/glslc
	CFLAGS+=-I/Users/chase/VulkanSDK/1.3.296.0/iOS/include
	LDLIBS+=-lvulkan
	ALL_TARGET=modeler.a
	CFLAGS+=-DVK_USE_PLATFORM_METAL_EXT
	CFLAGS+=--target=arm64-apple-ios-simulator
else ifeq ($(OS),Windows_NT)
	CC=/msys64/mingw64/bin/gcc
	CXX=/msys64/mingw64/bin/g++
	RM=/msys64/usr/bin/rm
	CP=/msys64/usr/bin/cp
	HEXDUMP=/msys64/usr/bin/hexdump
	SED=sed
	GLSLC=/VulkanSDK/1.3.296.0/Bin/glslc
	CFLAGS+=-I/VulkanSDK/1.3.296.0/Include -mwindows -municode
	LDFLAGS+=-L/VulkanSDK/1.3.296.0/Lib
	LDLIBS+=-lvulkan-1 -ldwmapi
	ALL_TARGET=modeler.exe
	CFLAGS+=-DVK_USE_PLATFORM_WIN32_KHR
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC=gcc
		SED=sed
		CP=cp
		GLSLC=glslc
		LDLIBS+=-lvulkan -lwayland-client -lwayland-cursor
		ALL_TARGET=modeler
		CFLAGS+=-DVK_USE_PLATFORM_WAYLAND_KHR -DDRAW_WINDOW_DECORATION
	endif
	ifeq ($(UNAME_S),Darwin)
		SED=gsed
		CP=cp
		GLSLC=/Users/chase/VulkanSDK/1.3.296.0/macOS/bin/glslc
		CFLAGS+=-I/Users/chase/VulkanSDK/1.3.296.0/macOS/include
		LDLIBS+=-lvulkan
		ALL_TARGET=modeler.a
		CFLAGS+=-DVK_USE_PLATFORM_METAL_EXT
	endif
endif

ifdef ENABLE_VSYNC
	CFLAGS+=-DENABLE_VSYNC
endif

SPIRV_SHADERS=window_border.vert.spv window_border.frag.spv chess_board.vert.spv chess_board.frag.spv phong.vert.spv phong.frag.spv
PNG_TEXTURES=pieces.png
OBJ_MESHES=teapot.obj pawn.obj
HEADER_SHADERS=shader_window_border.vert.h shader_window_border.frag.h shader_chess_board.vert.h shader_chess_board.frag.h shader_phong.vert.h shader_phong.frag.h
HEADER_TEXTURES=texture_pieces.h
MODELER_OBJS=modeler.o instance.o surface.o physical_device.o device.o swapchain.o image.o image_view.o render_pass.o descriptor.o framebuffer.o command_pool.o command_buffer.o synchronization.o allocator.o input_event.o queue.o utils.o vulkan_utils.o renderloop.o pipeline.o buffer.o sampler.o chess_board.o chess_engine.o matrix_utils.o
VENDOR_LIBS=vma_implementation.o lodepng.o tinyobj_implementation.o

MESHES=$(OBJ_MESHES)
ifdef DEBUG
	CFLAGS+=-DDEBUG -g
	SHADERS=$(SPIRV_SHADERS)
	TEXTURES=$(PNG_TEXTURES)
else
	CFLAGS+=-DEMBED_SHADERS
	SHADERS=$(HEADER_SHADERS)
	CFLAGS+=-DEMBED_TEXTURES
	TEXTURES=$(HEADER_TEXTURES)
	ifeq ($(OS),Windows_NT)
		LDFLAGS+=-static
	endif
endif

ifdef ENABLE_IMGUI
	CFLAGS+=-DENABLE_IMGUI
	IMGUI_LIBS+=imgui.a
endif

all: $(ALL_TARGET)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

modeler: $(SHADERS) $(TEXTURES) $(MESHES) $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o xdg-shell-protocol.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o xdg-shell-protocol.o $(VENDOR_LIBS) $(LDLIBS) $(IMGUI_LIBS)

modeler.exe: $(SHADERS) $(TEXTURES) $(MESHES) $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler.exe $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o $(VENDOR_LIBS) $(IMGUI_LIBS) $(LDLIBS)

modeler.a: $(SHADERS) $(TEXTURES) $(MESHES) $(MODELER_OBJS) modeler_metal.o surface_metal.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(AR) rvs $@ $(MODELER_OBJS) modeler_metal.o surface_metal.o $(VENDOR_LIBS)

modeler_android.a: $(SHADERS) $(TEXTURES) $(MESHES) $(MODELER_OBJS) modeler_android.o surface_android.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(AR) rvs $@ $(MODELER_OBJS) modeler_android.o surface_android.o $(VENDOR_LIBS)

main_wayland.o: src/main_wayland.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c src/main_wayland.c

%.vert.spv: src/shaders/%.vert.glsl
	$(GLSLC) -fshader-stage=vert $< -o $@

%.frag.spv: src/shaders/%.frag.glsl
	$(GLSLC) -fshader-stage=frag $< -o $@

%.rgba: src/textures/%.png
	./imgtorgba.sh $< $@

%.png: src/textures/%.png
	$(CP) $< $@

%.obj: src/meshes/%.obj
	$(CP) $< $@

$(HEADER_SHADERS): shader_%.h: %.spv
	./hexdump_include.sh "`echo $(basename $<)ShaderBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)ShaderSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

$(HEADER_TEXTURES): texture_%.h: %.png
	./hexdump_include.sh "`echo $(basename $<)TextureBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)TextureSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

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

tinyobj_implementation.o: src/tinyobj_loader_c.h
	$(CC) $(CFLAGS) -c src/tinyobj_implementation.c

.PHONY: clean clean-app clean-vendor
clean: clean-app clean-vendor

clean-app:
	$(RM) -rf modeler modeler.exe modeler.a modeler_android.a main_wayland.o main_win32.o \
		modeler_win32.o modeler_wayland.o modeler_metal.o modeler_android.o \
		surface_win32.o surface_wayland.o surface_metal.o surface_android.o \
		utils_win32.o \
		$(MODELER_OBJS) $(SPIRV_SHADERS) $(HEADER_SHADERS) $(PNG_TEXTURES) $(HEADER_TEXTURES) $(OBJ_MESHES)

clean-vendor:
	$(RM) -rf $(VENDOR_LIBS) \
		xdg-shell-protocol.o xdg-shell-client-protocol.h  xdg-shell-protocol.c \
		imgui.a cimgui.o cimgui_impl_vulkan.o imgui.o imgui_demo.o imgui_draw.o imgui_impl_modeler.o imgui_impl_vulkan.o imgui_tables.o imgui_widgets.o
