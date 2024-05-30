vpath %.c $(dir ./)
vpath %.c $(dir ../wolfboot/lib/wolfssl/wolfcrypt/src)

./src/wcs/%.o: ./wolfboot/lib/wolfssl/wolfcrypt/src/%.c
	@echo "\t[CC-$(ARCH)] $@"
	$(Q)arm-none-eabi-gcc $(CFLAGS) -c $(OUTPUT_FLAG) -o $@ $<

