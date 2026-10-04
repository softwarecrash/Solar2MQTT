# Solar2MQTT — M5Stack ATOM Lite + ATOMIC RS232 Base

Target hardware:
- M5Stack ATOM Lite (ESP32-PICO-D4, 4 MB flash)
- M5Stack ATOMIC RS232 Base (MAX232, full duplex)

UART mapping for the ATOMIC RS232 Base:
- Inverter RX (ESP32 receives): GPIO22
- Inverter TX (ESP32 transmits): GPIO19
- DE/RE: disabled (-1), not used by RS232

Other mapping:
- DS18B20: GPIO21
- Status LED service: disabled (ATOM Lite onboard RGB is addressable SK6812)

Build:

    pio run -e m5stack_atom_lite

Upload to COM8:

    pio run -e m5stack_atom_lite -t upload --upload-port COM8

Generated application binary:

    .pio/build/m5stack_atom_lite/firmware.bin

Important: keep your original 4 MB flash dump as a backup.
