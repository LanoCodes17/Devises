NWLINK ?= npx --yes -- nwlink@1.0.0
ARM_CC = arm-none-eabi-gcc
EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-device)

B = build

all: $(B)/Devises.nwa

$(B)/Devises.nwa: src/devises.c | $(B)
	$(ARM_CC) -std=gnu11 $(EADK_CFLAGS) -Os -Wall \
	  -ffunction-sections -fdata-sections \
	  -nostartfiles --specs=nano.specs -Wl,--relocatable -Wl,--gc-sections \
	  -Wl,-e,main -Wl,-u,main \
	  -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level \
	  src/devises.c -lm -lgcc -o $@
	arm-none-eabi-strip --strip-unneeded $@

$(B):
	mkdir -p $(B)

clean:
	rm -rf $(B)

.PHONY: all clean
