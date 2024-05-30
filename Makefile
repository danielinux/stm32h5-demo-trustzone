include ./src/wcs/pkcs11.mk

OBJS:=src/main.o
CFLAGS=-g -ggdb -O0
CFLAGS+=-mcpu=cortex-m33 -ffunction-sections -fdata-sections -fno-common -mcmse
LDFLAGS+=-mcpu=cortex-m33 
LDFLAGS+=-Wl,-gc-sections -Wl,-Map=image.map
CFLAGS+=-Isrc/wcs
C3FLAGS+=-Istm32h5xx_hal_driver/Inc
CFLAGS+=-Iwolfboot/include
CFLAGS+=-Iwolfboot/lib/wolfssl
CFLAGS+=-I.
CFLAGS+=-Istm32h5xx_hal_driver/Inc
CFLAGS+=-ICMSIS/Include
CFLAGS+=-ICMSIS/Device/ST/STM32H5xx/Include

CFLAGS+=-DSTM32H563xx

CFLAGS+=-DWOLFSSL_USER_SETTINGS -DWOLFBOOT_PKCS11_APP -DSECURE_PKCS11
CFLAGS+=-DWOLFBOOT_SECURE_CALLS -Wstack-usage=12944 -DWOLFCRYPT_SECURE_MODE
CFLAGS+=-Iwolfboot/lib/wolfPKCS11
OBJS+=./src/wcs/pkcs11_test_ecc.o
OBJS+=./src/wcs/pkcs11_stub.o
OBJS+=./src/wcs/ecc.o
OBJS+=./src/wcs/rsa.o
OBJS+=./src/wcs/asn.o
OBJS+=./src/wcs/aes.o
OBJS+=./src/wcs/hmac.o
OBJS+=./src/wcs/pwdbased.o
OBJS+=./src/wcs/hash.o
OBJS+=./src/wcs/sha256.o
OBJS+=./src/wcs/sha512.o
OBJS+=./src/wcs/sha3.o
OBJS+=./src/wcs/integer.o
OBJS+=./src/wcs/tfm.o
OBJS+=./src/wcs/sp_c32.o
OBJS+=./src/wcs/sp_int.o
OBJS+=./src/wcs/cryptocb.o
OBJS+=./src/wcs/wc_pkcs11.o
OBJS+=./src/wcs/memory.o
OBJS+=./src/wcs/wolfmath.o
OBJS+=./src/wcs/dh.o
OBJS+=./src/wcs/random.o
OBJS+=./src/wcs/coding.o
OBJS+=./src/wcs/wc_encrypt.o
OBJS+=./src/wcs/wc_port.o

# NSC API objects
OBJS+=./wolfboot/src/wc_secure_calls.o

# Ethernet drivers
OBJS+=./stm32h5xx_hal_driver/Src/stm32h5xx_hal_eth.o

all: wolfboot/wolfboot.bin application_v1_signed.bin wolfboot/tools/keytools/otp/otp-keystore-primer.bin
	mkdir -p build
	cp wolfboot/wolfboot.bin build/
	mv application_v1_signed.bin build/
	cp wolfboot/tools/keytools/otp/otp-keystore-primer.bin build/

wolfboot/tools/keytools/otp/otp-keystore-primer.bin: wolfboot/wolfboot.bin
	make -C wolfboot otp

wolfboot/wolfboot.bin: wolfboot/.config
	make -C wolfboot wolfboot.bin

%.o:%.c
	arm-none-eabi-gcc -c -o $(@) $(CFLAGS) $(^)

wolfboot/.config:
	cp wolfboot.config wolfboot/.config

application_v1_signed.bin: application.bin
	wolfboot/tools/keytools/sign --sha256 --lms $^ wolfboot/wolfboot_signing_private_key.der 1

application.bin: application.elf
	arm-none-eabi-objcopy -O binary $^ $@

application.elf: $(OBJS)
	arm-none-eabi-gcc $(CFLAGS) $(OBJS) $(LDFLAGS) -T target.ld -o $@

clean:
	rm -rf build
	rm -f src/*.o application.elf application.bin application_v1_signed.bin
	rm -f *.map
	make -C wolfboot clean
