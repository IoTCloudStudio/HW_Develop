esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash -fs 4MB -ff 40m \
    0x00000 boot_v1.6.bin 0x1000 user1.bin \
    0x7C000 esp_init_data_default.bin 0x7E000 blank.bin

sudo socat pty,link=/dev/ttyUSB33 tcp:192.168.100.135:23