NWLINK ?= nwlink
ARM_CC = arm-none-eabi-gcc
EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-device)

B = build

all: $(B)/Devises.nwa

$(B)/devises.o: src/devises.c | $(B)
	$(ARM_CC) -std=gnu11 $(EADK_CFLAGS) -Os -Wall -ffunction-sections -fdata-sections \
	  -c src/devises.c -o $@
	@echo "=== Taille du .o ==="
	@stat -c '%s bytes' $@

$(B)/Devises.nwa: $(B)/devises.o
	$(ARM_CC) -nostartfiles --specs=nano.specs \
	  -Wl,--relocatable -Wl,--gc-sections -Wl,-e,main \
	  -Wl,--unresolved-symbols=ignore-all \
	  -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level \
	  $< -lm -lgcc -o $@
	arm-none-eabi-strip --strip-unneeded $@
	@echo "=== Taille finale ==="
	@stat -c '%s bytes' $@

$(B):
	mkdir -p $(B)

clean:
	rm -rf $(B)

.PHONY: all clean
