# xiao-rp2040-audio-player

![Circuit Diagram](xiao-rp2040-audioplayer.png)

- blue components handle motion detection
- red components handle audio

## Parts

### Seeed [XIAO RP2040](https://www.seeedstudio.com/XIAO-RP2040-v1-0-p-5026.html) $5.40

### PIR: Panasonic Electric Works [EKMC1601111](https://akizukidenshi.com/catalog/g/gM-09750/) 500 Yen

| Type                  | Description            |
|-----------------------|------------------------|
| Sensor Type           | PIR (Passive Infrared) |
| Sensing Distance      | 197" (5m) 16.4'        |
| Output Type           | Digital                |
| Voltage - Supply      | 3V ~ 6V                |
| Trigger Type          | Internal               |
| Features              | Standard Lens, White   |
| Operating Temperature | -20°C ~ 60°C (TA)      |
| Detection Pattern     | Standard               |

### Speaker 8Ω 8W [P-03285](https://akizukidenshi.com/catalog/g/gP-03285/) 100 Yen
| Type                 | Description    |
|----------------------|----------------|
| Size                 | 50mm / 2inch   |
| Rated Impedance      | 8Ω             |
| Nominal Power        | 8W w/NETWORK   |
| Frequency Range      | 2k ~ 20kHx     |
| Sound Pressure Level | 87dB           |
| Voice Coil Diameter  | 13mm           |
| Magnet Mass          | 25.8g / 0.91oz |
| Total Mass           | 100g           |

### Transistor 2SC4408

### Variable Resistor 10kΩ

## Convert audio file to samples.h
The *rate* parameter `-r` below can changed, but don't forget to set `SAMPLE_RATE` in main.h accordingly
```
sox file.wav -c1 -r44100 -tdat - \
| tail -n+3 \
| awk '
  BEGIN { printf "const uint8_t __in_flash() audio_buffer[] = {\n" }
  { printf "  %.0f,\n", ($2+1)*128}
  END { printf "};\n"}' \
> samples.h
```
