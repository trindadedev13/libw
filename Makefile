CMAKE = cmake
BUILD = build

CMAKE_GEN_FLAGS = -B build -S . -G Ninja

ANDROID_NDK             = $(HOME)/android-ndk
ANDROID_API_VERSION     = 29
ANDROID_CMAKE_TOOLCHAIN = $(ANDROID_NDK)/build/cmake/android.toolchain.cmake

all:
echo "Usage: make android or linux"

linux: prepare $(BUILD)/libw.a
linux-ccj:
	$(CMAKE) $(CMAKE_GEN_FLAGS) -DWANDROID=FALSE -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE
	cp $(BUILD)/compile_commands.json .

android: prepare $(BUILD)/arm64-v8a/libw.a
android-ccj:
	$(CMAKE) $(CMAKE_GEN_FLAGS) \
		-DWANDROID=TRUE \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
		-DANDROID_NDK=$(ANDROID_NDK) \
		-DCMAKE_TOOLCHAIN_FILE=$(ANDROID_CMAKE_TOOLCHAIN) \
		-DANDROID_ABI=arm64-v8a \
		-DANDROID_PLATFORM=android-$(ANDROID_API_VERSION)
		cp $(BUILD)/compile_commands.json .

prepare:
	mkdir -p build

clean:
	rm -rf build

# Linux Build

$(BUILD)/libw.a:
	$(CMAKE) $(CMAKE_GEN_FLAGS) -DWANDROID=FALSE
	$(CMAKE) --build build

# Android Build
$(BUILD)/arm64-v8a/libw.a:
	$(CMAKE) $(CMAKE_GEN_FLAGS) \
	-DWANDROID=TRUE \
	-DANDROID_NDK=$(ANDROID_NDK) \
	-DCMAKE_TOOLCHAIN_FILE=$(ANDROID_CMAKE_TOOLCHAIN) \
	-DANDROID_ABI=arm64-v8a \
	-DANDROID_PLATFORM=android-$(ANDROID_API_VERSION)
	$(CMAKE) --build build
	mkdir -p $(shell dirname $@)
	mv build/libw.a $@