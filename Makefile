CFLAGS=
CXXFLAGS+=-std=c++17
LDFLAGS=
LDLIBS=

ifeq ($(OS),Windows_NT)
	HEXDUMP=/msys64/usr/bin/hexdump
	RM=/msys64/usr/bin/rm
	CP=/msys64/usr/bin/cp
	SED=sed
	ANDROID_NDK=/Android/Sdk/ndk/28.0.12674087
	ANDROID_TOOLCHAIN=$(ANDROID_NDK)/toolchains/llvm/prebuilt/windows-x86_64
	VULKAN_SDK=/VulkanSDK/1.3.296.0
	GLSLC=$(VULKAN_SDK)/Bin/glslc
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CP=cp
		SED=sed
		ANDROID_NDK=$${HOME}/Android/Sdk/ndk/28.0.12674087
		ANDROID_TOOLCHAIN=$(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64
		GLSLC=glslc
	endif
	ifeq ($(UNAME_S),Darwin)
		CP=cp
		SED=gsed
		ANDROID_NDK=$${HOME}/Library/Android/sdk/ndk/28.0.12674087
		ANDROID_TOOLCHAIN=$(ANDROID_NDK)/toolchains/llvm/prebuilt/darwin-x86_64
		VULKAN_SDK=$${HOME}/VulkanSDK/1.3.296.0
		GLSLC=$(VULKAN_SDK)/macOS/bin/glslc
	endif
endif

ifdef ANDROID
	CC=$(ANDROID_TOOLCHAIN)/bin/clang
	CXX=$(ANDROID_TOOLCHAIN)/bin/clang++
	AR=$(ANDROID_TOOLCHAIN)/bin/llvm-ar
	TARGET=aarch64-linux-android
	API=21
	LDLIBS+=-lvulkan
	ALL_TARGET=modeler_android.a imgui.a
	CFLAGS+=--target=$(TARGET)$(API)
	CFLAGS+=-I$(ANDROID_TOOLCHAIN)/sysroot/usr/include
	CFLAGS+=-DVK_USE_PLATFORM_ANDROID_KHR
	CFLAGS+=-DANDROID
	CFLAGS+=-fPIC
else ifdef IOS
	CFLAGS+=-I$(VULKAN_SDK)/iOS/include
	LDLIBS+=-lvulkan
	ALL_TARGET=modeler.a
	CFLAGS+=-DVK_USE_PLATFORM_METAL_EXT
	CFLAGS+=--target=arm64-apple-ios-simulator
else ifeq ($(OS),Windows_NT)
	CC=/msys64/mingw64/bin/gcc
	CXX=/msys64/mingw64/bin/g++
	CFLAGS+=-I$(VULKAN_SDK)/Include -mwindows -municode
	LDFLAGS+=-L$(VULKAN_SDK)/Lib
	LDLIBS+=-lvulkan-1 -ldwmapi
	ALL_TARGET=modeler.exe
	CFLAGS+=-DVK_USE_PLATFORM_WIN32_KHR
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC=gcc
		LDLIBS+=-lvulkan -lwayland-client -lwayland-cursor
		ALL_TARGET=modeler
		CFLAGS+=-DVK_USE_PLATFORM_WAYLAND_KHR -DDRAW_WINDOW_BORDER
	endif
	ifeq ($(UNAME_S),Darwin)
		CFLAGS+=-I$(VULKAN_SDK)/macOS/include
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
OBJ_MESHES=teapot.obj pawn.obj knight.obj bishop.obj rook.obj queen.obj king.obj
TTF_FONTS=roboto.ttf
HEADER_SHADERS=shader_window_border.vert.h shader_window_border.frag.h shader_chess_board.vert.h shader_chess_board.frag.h shader_phong.vert.h shader_phong.frag.h
HEADER_TEXTURES=texture_pieces.h
HEADER_MESHES=mesh_pawn.h mesh_knight.h mesh_bishop.h mesh_rook.h mesh_queen.h mesh_king.h
HEADER_FONTS=font_roboto.h
MODELER_OBJS=modeler.o instance.o surface.o physical_device.o device.o swapchain.o image.o image_view.o render_pass.o descriptor.o framebuffer.o command_pool.o command_buffer.o synchronization.o allocator.o input_event.o queue.o utils.o vulkan_utils.o renderloop.o pipeline.o buffer.o sampler.o chess_board.o chess_engine.o matrix_utils.o
VENDOR_LIBS=vma_implementation.o lodepng.o tinyobj_implementation.o

ifdef EMBED_RESOURCES
	CFLAGS+=-DEMBED_SHADERS
	SHADERS=$(HEADER_SHADERS)
	CFLAGS+=-DEMBED_TEXTURES
	TEXTURES=$(HEADER_TEXTURES)
	CFLAGS+=-DEMBED_MESHES
	MESHES=$(HEADER_MESHES)
	CFLAGS+=-DEMBED_FONTS
	FONTS=$(HEADER_FONTS)
else
	SHADERS=$(SPIRV_SHADERS)
	TEXTURES=$(PNG_TEXTURES)
	MESHES=$(OBJ_MESHES)
	FONTS=$(TTF_FONTS)
endif

ifdef DEBUG
	CFLAGS+=-DDEBUG -g
else
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

modeler: $(SHADERS) $(TEXTURES) $(MESHES) $(FONTS) $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o xdg-shell-protocol.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler $(MODELER_OBJS) main_wayland.o modeler_wayland.o surface_wayland.o xdg-shell-protocol.o $(VENDOR_LIBS) $(LDLIBS) $(IMGUI_LIBS)

modeler.exe: $(SHADERS) $(TEXTURES) $(MESHES) $(FONTS) $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o modeler.exe $(MODELER_OBJS) main_win32.o modeler_win32.o surface_win32.o utils_win32.o $(VENDOR_LIBS) $(IMGUI_LIBS) $(LDLIBS)

modeler.a: $(SHADERS) $(TEXTURES) $(MESHES) $(FONTS) $(MODELER_OBJS) modeler_metal.o surface_metal.o $(VENDOR_LIBS) $(IMGUI_LIBS)
	$(AR) rvs $@ $(MODELER_OBJS) modeler_metal.o surface_metal.o $(VENDOR_LIBS)

modeler_android.a: $(SHADERS) $(TEXTURES) $(MESHES) $(FONTS) $(MODELER_OBJS) modeler_android.o surface_android.o $(VENDOR_LIBS) $(IMGUI_LIBS)
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

%.ttf: src/fonts/%.ttf
	$(CP) $< $@

$(HEADER_SHADERS): shader_%.h: %.spv
	./hexdump_include.sh "`echo $(basename $<)ShaderBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)ShaderSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

$(HEADER_TEXTURES): texture_%.h: %.png
	./hexdump_include.sh "`echo $(basename $<)TextureBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)TextureSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

$(HEADER_MESHES): mesh_%.h: %.obj
	./hexdump_include.sh "`echo $(basename $<)MeshBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)MeshSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

$(HEADER_FONTS): font_%.h: %.ttf
	./hexdump_include.sh "`echo $(basename $<)FontBytes | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" "`echo $(basename $<)FontSize | $(SED) -r 's/(_|-|\.)(\w)/\U\2/g'`" $< > $@

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
		$(MODELER_OBJS) $(SPIRV_SHADERS) $(HEADER_SHADERS) $(PNG_TEXTURES) $(HEADER_TEXTURES) $(OBJ_MESHES) $(HEADER_MESHES) $(TTF_FONTS) $(HEADER_FONTS)

clean-vendor:
	$(RM) -rf $(VENDOR_LIBS) \
		xdg-shell-protocol.o xdg-shell-client-protocol.h  xdg-shell-protocol.c \
		imgui.a cimgui.o cimgui_impl_vulkan.o imgui.o imgui_demo.o imgui_draw.o imgui_impl_modeler.o imgui_impl_vulkan.o imgui_tables.o imgui_widgets.o
