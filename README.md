# Circuit

![Circuit Diagram](xiao-rp2040-audioplayer.png)

- blue components handle motion detection
- red components handle audio

# Build instructions
```
export PICO_BOARD=seeed_xiao_rp2040
mkdir build
cd build
cmake ..
make
```

# Convert audio file to samples.h
The *rate* parameter `-r` below can changed, but don't forget to set `SAMPLE_RATE` in main.c accordingly
```
sox file.wav -c1 -r44100 -tdat - \
| tail -n+3 \
| awk '
  BEGIN { printf "const uint8_t __in_flash() audio_buffer[] = {\n" }
  { printf "  %.0f,\n", ($2+1)*128}
  END { printf "};\n"}' \
> samples.h
```

# Parts

## Seeed [XIAO RP2040](https://www.seeedstudio.com/XIAO-RP2040-v1-0-p-5026.html) [790 Yen](https://www.marutsu.co.jp/pc/i/2229736/)

- [RP2040 chip datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) (C/C++ development with RP2040-based boards)
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf) (Libraries and tools for C/C++ development on RP2040 microcontrollers)
  - the Seeed XIAO RP2040 board has been added to the [pico-sdk](https://github.com/raspberrypi/pico-sdk) in [version 1.3.1](https://github.com/raspberrypi/pico-sdk/releases/tag/1.3.1) on 2022/5/19

## PIR: Panasonic Electric Works [EKMC1601111](https://www3.panasonic.biz/ac/e/search_num/index.jsp?c=detail&part_no=EKMC1601111) [500 Yen](https://akizukidenshi.com/catalog/g/gM-09750/)

[Datasheet](https://mediap.industry.panasonic.eu/assets/download-files/import/ds_ekmc160111_v63_en.pdf)

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

## Speaker 8Ω 8W P-03285 [100 Yen](https://akizukidenshi.com/catalog/g/gP-03285/)

[Datasheet](https://akizukidenshi.com/download/p3285.pdf)

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

## Transistor 2SC4408

[Datasheet](https://cdn.datasheetspdf.com/pdf-down/C/4/4/C4408_ToshibaSemiconductor.pdf)

### Absolute Maximum Ratings (Ta = 25°C)

| Characteristics             | Symbol | Rating     | Unit |
|-----------------------------|--------|------------|------|
| Collector-base voltage      | V_CBO  | 80         | V    |
| Collector-emitter voltage   | V_CEO  | 50         | V    |
| Emitter-base voltage        | V_EBO  | 6          | V    |
| Collector current           | I_C    | 2          | A    |
| Base current                | I_B    | 0.2        | A    |
| Collector power dissipation | P_C    | 900        | mW   |
| Junction temperature        | Tj     | 150        | °C   |
| Storage temperature range   | Tstg   | −55 to 150 | °C   |

### Electrical Characteristics (Ta = 25°C)

| Characteristics                      | Symbol    | Test Condition                         | Min | Typ | Max | Unit |
|--------------------------------------|-----------|----------------------------------------|-----|-----|-----|------|
| Collector cut-off current            | I_CBO     | V_CB = 80 V, I_E = 0                   | ―   | ―   | 1.0 | μA   |
| Emitter cut-off current              | I_EBO     | V_EB = 6 V, I_C = 0                    | ―   | ―   | 1.0 | μA   |
| Collector-emitter breakdown voltage  | V_(BR)CEO | I_C = 10 mA, I_B = 0                   | 50  | ―   | ―   | V    |
| DC current gain                      | h_FE(1)   | V_CE = 2 V, I_C = 100 mA               | 120 | ―   | 400 |      |
| DC current gain                      | h_FE(2)   | V_CE = 2 V, I_C = 1.5 A                | 40  | ―   | ―   |      |
| Collector-emitter saturation voltage | V_CE(sat) | I_C = 1 A, I_B = 0.05 A                | ―   | ―   | 0.5 | V    |
| Base-emitter saturation voltage      | V_BE(sat) | I_C = 1 A, I_B = 0.05 A                | ―   | ―   | 1.2 | V    |
| Transition frequency                 | f_T       | V_CE = 2 V, I_C = 100 mA               | ―   | 100 | ―   | MHz  |
| Collector output capacitance         | C_ob      | V_CB = 10 V, I_C = 0, f = 1 MHz        | ―   | 14  | ―   | p    |
| Switching time Turn-on time          | t_on      | I_B1 = −I_B2 = 0.05 A, duty cycle ≤ 1% | ―   | 0.1 | ―   | μs   |
| Switching time Storage time          | t_stg     | I_B1 = −I_B2 = 0.05 A, duty cycle ≤ 1% | ―   | 0.5 | ―   | μs   |
| Switching time Fall time             | t_f       | I_B1 = −I_B2 = 0.05 A, duty cycle ≤ 1% | ―   | 0.1 | ―   | μs   |

## Variable Resistor 10kΩ
