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

$(B)/icon.o: devises-icone (1).png | $(B)
	$(NWLINK) png-icon-o $< $@
	@echo "=== Taille de l'icone ==="
	@stat -c '%s bytes' $@

$(B)/Devises.nwa: $(B)/devises.o $(B)/icon.o
	$(ARM_CC) -Wl,--relocatable -nostartfiles --specs=nano.specs \
	  -Wl,-e,main -Wl,--gc-sections \
	  -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level \
	  $(B)/devises.o $(B)/icon.o -o $@
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
