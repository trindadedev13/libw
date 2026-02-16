PROJECT = example
SRC     = main.c
INCLUDE = ../include
BUILD   = build

ANDROID_SDK = $(HOME)/android-sdk
ANDROID_NDK = $(HOME)/android-ndk
ANDROID_API = 29
ANDROID_ABI = arm64-v8a

ANDROID_SDK_JAR = $(ANDROID_SDK)/platforms/android-$(ANDROID_API)/android.jar

ANDROID_DIR      = android
ANDROID_MANIFEST = $(ANDROID_DIR)/AndroidManifest.xml
ANDROID_RES_DIR  = $(ANDROID_DIR)/res
ANDROID_JAVA_SRC = $(ANDROID_DIR)/src/java

ANDROID_KEYSTORE      = $(ANDROID_DIR)/libw.keystore
ANDROID_KEY_ALIAS     = libwKey
ANDROID_KEYSTORE_PASS = 12345678

NATIVE_APP_GLUE = $(ANDROID_NDK)/sources/android/native_app_glue/android_native_app_glue.c

ANDROID_SO = $(BUILD)/lib/$(ANDROID_ABI)/libexample.so

ifeq ($(ANDROID), 1)
	CC = $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$(ANDROID_API)-clang
	CFLAGS = -I$(INCLUDE) -I$(ANDROID_NDK)/sources/android/native_app_glue -fPIC
	LIBS   = -shared -landroid -llog -lEGL -lGLESv3 ../build/arm64-v8a/libw.a
	TARGET = $(BUILD)/signed.apk
else
	CC = gcc
	CFLAGS = -I$(INCLUDE)
	LIBS   = ../build/libw.a -lEGL -lX11 -g -fsanitize=address
	TARGET = $(BUILD)/example
endif

.PHONY: all clean add-in-zip prepare

all: prepare $(TARGET)

prepare:
	mkdir -p build

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/example: | $(BUILD)
	$(CC) $(SRC) $(CFLAGS) -o $@ $(LIBS)

$(ANDROID_SO): | $(BUILD)
	mkdir -p $(BUILD)/lib/$(ANDROID_ABI)
	$(CC) $(SRC) $(NATIVE_APP_GLUE) $(CFLAGS) -o $@ $(LIBS)

$(BUILD)/res.zip:
	aapt2 compile --dir $(ANDROID_RES_DIR) -o $@

$(BUILD)/unaligned.apk: $(BUILD)/res.zip
	aapt2 link --auto-add-overlay \
		--manifest $(ANDROID_MANIFEST) \
		-I $(ANDROID_SDK_JAR) \
		--min-sdk-version 24 \
		--target-sdk-version $(ANDROID_API) \
		--java $(ANDROID_JAVA_SRC) \
		-R $^ \
		-o $@

$(BUILD)/classes.jar: $(BUILD)/unaligned.apk
	javac --release 8 -classpath $(ANDROID_SDK_JAR) -d $(BUILD)/classes \
		$(ANDROID_JAVA_SRC)/dev/trindadedev/wexample/MainActivity.java \
		$(ANDROID_JAVA_SRC)/dev/trindadedev/wexample/R.java
	jar cf $@ -C $(BUILD)/classes .

$(BUILD)/classes.dex: $(BUILD)/classes.jar
	d8 --min-api $(ANDROID_API) --output $(BUILD) $^

add-in-zip: $(BUILD)/classes.dex $(ANDROID_SO)
	(cd $(BUILD) && zip -ur unaligned.apk classes.dex)
	(cd $(BUILD) && zip -ur unaligned.apk lib/$(ANDROID_ABI)/libexample.so)

$(BUILD)/signed-unaligned.apk: $(BUILD)/unaligned.apk add-in-zip
	jarsigner -keystore $(ANDROID_KEYSTORE) \
		-storepass $(ANDROID_KEYSTORE_PASS) \
		-keypass $(ANDROID_KEYSTORE_PASS) \
		-signedjar $@ \
		$(BUILD)/unaligned.apk \
		$(ANDROID_KEY_ALIAS)

$(BUILD)/signed.apk: $(BUILD)/signed-unaligned.apk
	zipalign -f 4 $^ $@