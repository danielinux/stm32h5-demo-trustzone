file wolfboot/wolfboot.elf
tar rem:3333
add-symbol-file application.elf 0x08042000
hb main
mon reset
focus cmd


