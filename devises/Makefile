NWLINK ?= npx --yes -- nwlink@1.0.0
ARM_CC = arm-none-eabi-gcc
EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-device)

B = build

all: $(B)/Devises.nwa

$(B)/devises.o: src/devises.c | $(B)
	$(ARM_CC) -std=gnu11 $(EADK_CFLAGS) -Os -Wall \
	  -fdata-sections -ffunction-sections \
	  -c src/devises.c -o $@
	@echo "=== Taille du .o ==="
	@stat -c '%s bytes' $@
	@echo "=== Symboles ==="
	@arm-none-eabi-nm $@

$(B)/Devises.nwa: $(B)/devises.o
	$(ARM_CC) -Wl,--relocatable -nostartfiles --specs=nano.specs \
	  -Wl,-e,main \
	  -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level \
	  -Wl,--gc-sections \
	  $< -o $@
	@echo "=== Taille avant strip ==="
	@stat -c '%s bytes' $@
	arm-none-eabi-strip --strip-unneeded $@
	@echo "=== Taille finale ==="
	@stat -c '%s bytes' $@

$(B):
	mkdir -p $(B)

clean:
	rm -rf $(B)

.PHONY: all clean
