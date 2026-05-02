
.PHONY: all

ifeq ($(DEBUG),1)
    BUILD_FOLDER = Debug
else
    BUILD_FOLDER = Release
endif

all: $(BUILD_FOLDER)/ggxrd_hitbox_injector.exe

# CHOST should be i686-w64-mingw32 or similar (a build of GCC or g++ with mingw, there will be a whole suite of tools with that prefix)

ifeq ($(CHOST),)
  CROSS_PREFIX = 
else
  CROSS_PREFIX = $(CHOST)-
endif

OBJS := $(shell <ggxrd_hitbox_injector.vcxproj sed -n -E -e 's/\s*<ClCompile\s+Include\s*=\s*"([^"]+)"\s*\/?>\s*/\1/p' | tr '\\' '/')

HEADERS := $(shell <ggxrd_hitbox_injector.vcxproj sed -n -E -e 's/\s*<ClInclude\s+Include\s*=\s*"([^"]+)"\s*\/?>\s*/\1/p' | tr '\\' '/' | sed 's/Resource.h/resource.h/')

CXX = $(CROSS_PREFIX)g++
STRIP = $(CROSS_PREFIX)strip
WINDRES = $(CROSS_PREFIX)windres

LIBS = -lcomctl32
INCLUDES = -I. -I../common

CORE_DEFAULT_DEFINES = -DUNICODE \
    -DANY_KEY='"any key"' \
    -DWIN32 \
    -D_WINDOWS

ifeq ($(DEBUG),1)
    DEFINES = $(CORE_DEFAULT_DEFINES) \
        -D_DEBUG
else
    DEFINES = $(CORE_DEFAULT_DEFINES) \
        -DNDEBUG
endif

# -municode needed to accept wWinMain instead of WinMain
# -static-libgcc because I get GetLastError() 126 "The specified module could not be found." after a LoadLibrary call to load the produced DLL fails. The DLL depends on libgcc_s_dw2-1.dll for __deregister_frame_info and __register_frame_info (possibly more) functions. Link statically to avoid having to ship this dependency? 
# -static-libstdc++ same as above
CXX_CORE_DEFAULT_FLAGS = \
    -m32 \
    -mwindows \
    -municode \
    -masm=intel \
    -march=pentium3 \
    -mavx \
    -mmmx \
    -msse \
    -mhard-float \
    -mms-bitfields \
    -fpermissive \
    -Wno-conversion-null \
    -Wno-enum-compare \
    -Wno-trigraphs \
    -Wno-invalid-constexpr \
    -Wno-invalid-offsetof \
    -static-libgcc \
    -static-libstdc++
    
ifeq ($(DEBUG),1)
    CXXFLAGS = $(CXX_CORE_DEFAULT_FLAGS)
else
    CXXFLAGS = $(CXX_CORE_DEFAULT_FLAGS) \
        --optimize \
        -g0 \
        -flto \
        -fuse-linker-plugin \
        -fno-fat-lto-objects
endif

$(BUILD_FOLDER)/ggxrd_hitbox_injector.exe: $(OBJS) $(HEADERS)
	if [ ! -d $(BUILD_FOLDER) ]; then mkdir $(BUILD_FOLDER); fi
	@# The resource file is in utf-16 little-endian, but MinGW preprocessor can't read that, it needs utf-8.
	@# If you specify -c utf-16le to WINDRES it will just ignore that, and if you specify --preprocessor-arg='-finput-charset=utf-16le' to WINDRES, it will pass -finput-charset=utf-16le to the preprocessor, but then the preprocessor won't understand the files that the .rc includes, because they're in utf-8, and you can't specify more than one encoding, or a separate encoding for each file, and it doesn't seem to autodetect encodings despite the large obvious BOM (FF FE) at the start.
	@# Use iconv --from-code=utf-16le --to-code=utf-8 -o thing_utf8.rc thing_utf16.rc to convert the encoding or, the thing we'll be using right now, a python script.
	@# By the way, windres won't be able to find files if they're specified with backslashes, so backslashes must be replaced with forward slashes, too.
	python3 prebuild.py  # updates version in ggxrd_hitbox_injector.rc, converts it to utf8 with forward slashes instead of backslashes
	$(WINDRES) $(DEFINES) $(INCLUDES) ggxrd_hitbox_injector_utf8.rc $(BUILD_FOLDER)/ggxrd_hitbox_injector.rc.o
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS) $(BUILD_FOLDER)/ggxrd_hitbox_injector.rc.o $(LIBS) -o $@
ifneq ($(DEBUG),1)
	$(STRIP) --strip-debug $@
endif
