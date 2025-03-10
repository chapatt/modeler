# modeler

A cross-platform (macOS, Windows, Linux, iOS, Android) application implementing native, borderless windows and a Vulkan rendering context.

* All window management (creation/resizing/etc.) is handled via native system APIs (SwiftUI (with AppKit/UIKit), win32, wayland, GameActivity) rather than a layer like GLFW or SDL
* Legitimate application windows are used, so all behaviors like snapping, tiling, etc. work as expected
* Multi-monitor HiDPI awareness is implemented on all platforms

Functionality is nearly feature-complete. The following major tasks remain:
- [x] Handle wayland tiling/maximize hints (`stages` argument from `xdg_toplevel::configure` event)
- [ ] Fullscreen mode
	- [ ] Attempt to acquire exclusive fullscreen access if possible (`VK_EXT_full_screen_exclusive` extension)
	- [ ] Disable window border/shadow renderpass on wayland when transitioning to fullscreen
		- `DRAW_WINDOW_BORDER` conditionals
		- Move from subpass to genuine renderpass

Nice-to-haves:
- [ ] Get system cursor theme on wayland
- [ ] Refactor functions with long parameter lists to take structs
- [x] Integrate dear imgui with application renderpass
- [ ] Add test suite and run via CI pipeline
- [ ] Apply this framework via a game/application beyond drawing a triangle :)

macOS:<br />
<img src="docs/images/window_triangle_macos.png" alt="macOS" width=400 /><br />
Windows:<br />
<img src="docs/images/window_triangle_windows.png" alt="Windows" width=400 /><br />
Linux:<br />
<img src="docs/images/window_triangle_linux.png" alt="Linux" width=400 /><br />

## Building

```shell
make
```
builds the binary on Linux (`modeler`) and Windows (`modeler.exe`), a static library (`modeler.a`) on macOS and iOS, and a static library (`modeler_android.a`) on Android. The macOS and iOS applications can be built via the Xcode project under `xcode/`. The Android app can be built via the Android Studio project under `android/`.

To build for iOS, set environment variable `IOS` to true. Similarly, for Android, set `ANDROID` to true. E.g.
```shell
make IOS=true
```

To enable debugging symbols, external SPIR-V files, and dynamic libraries, set the `DEBUG` environment variable
```shell
make DEBUG=true
```

To enable imgui, set the `ENABLE_IMGUI` environment variable
```shell
make ENABLE_IMGUI=true
```

To enable FIFO frame presentation, set the `ENABLE_VSYNC` environment variable. Otherwise mailbox is used. (This variable is misnamed and should be changed.)
```shell
make ENABLE_VSYNC=true
```

## Linux

### Build Dependencies
wayland-protocols
libwayland-dev
libvulkan-dev
glslc
bsdextrautils (hexdump)

#### Debug only
vulkan-validationlayers

## macOS and iOS

### Vulkan SDK
Set up locations of Vulkan libraries to which to link: Xcode > Settings > Locations > Custom Paths

| Name				| Display Name		| Path													|
|--					|--					|--														|
| VULKAN_SDK		| Vulkan SDK		| <path_to_vulkan_sdk_including_version>				|
| VULKAN_HEADERS	| Vulkan Headers	| <path_to_vulkan_sdk_including_version>/macOS/include	|
| VULKAN_IOS_HEADERS	| Vulkan iOS Headers	| <path_to_vulkan_sdk_including_version>/iOS/include	|
| VULKAN_LIB		| Vulkan Libraries	| <path_to_vulkan_sdk_including_version>/macOS/lib		|

## Windows
Install mingw_w64 (preferrably via msys64).

Install Vulkan SDK. Paths in `Makefile` may need to be updated for install location and version.

## Dependencies
| Name				| License		| Website																		| Notes								|
|--					|--				|--																				|--									|
| Dear ImGui		| MIT			| [GitHub](https://github.com/ocornut/imgui)									| 									|
| lodepng			| zlib			| [GitHub](https://github.com/lvandeve/lodepng)									| 									|
| MoltenVK			| Apache-2.0	| [GitHub](https://github.com/KhronosGroup/MoltenVK)							| macOS and iOS only				|
| tinyobjloader		| MIT			| [GitHub](https://github.com/syoyo/tinyobjloader-c)							| 									|
| VMA				| MIT			| [GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)	| 									|
| Vulkan Loader		| Apache-2.0	| [GitHub](https://github.com/KhronosGroup/Vulkan-Loader)						| Dynamic linking on some platforms	|