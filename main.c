#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/vreg.h"

#define SYS_CLOCK_KHZ 133000
//#define VREG_VOLTAGE VREG_VOLTAGE_1_30

#include "samples.h"

#define AUDIO_ENABLED true
#define LEDS_ENABLED true
#define AUDIO_PIN 4
#define PIR_PIN 1

const uint USER_LED_B = 25;
const uint USER_LED_G = 16;
const uint USER_LED_R = 17;

// PIR readings often flip-flop, which is why we will
// do some smoothing here over the last PIR_AVERAGING_COUNT
// PIR readings, taken at intervals of PIR_READ_INTERVAL milliseconds
const uint PIR_READ_INTERVAL = 100;
#define PIR_AVERAGING_COUNT 5
const uint PIR_READINGS_MIN_SUM = 5;

// allows us to activate and place the device, without it going off already
const uint INITIAL_SLEEP_MS = 5000;

void detect_motion_loop(uint pir_pin, uint led_pin, uint led_pin_above_threshold);
void init_leds();
void setup_audio(int pin);
void play_audio(uint8_t plays, bool block_until_done);

int main() {
  //  vreg_set_voltage(VREG_VOLTAGE);
  set_sys_clock_khz(SYS_CLOCK_KHZ, true);
  init_leds();
  if (AUDIO_ENABLED)
    setup_audio(AUDIO_PIN);

  sleep_ms(INITIAL_SLEEP_MS);

  detect_motion_loop(PIR_PIN, USER_LED_B, USER_LED_R);
  return 0;
}

//////////////////////////////////
// builtin LEDs
//////////////////////////////////

void turn_off_led(uint pin) {
  gpio_put(pin, 1);
}

void turn_on_led(uint pin) {
  gpio_put(pin, 0);
}


void init_led(uint pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  turn_off_led(pin);
}

// by default, these leds are always on
// let's turn them off
void init_leds() {
  init_led(USER_LED_R);
  init_led(USER_LED_G);
  init_led(USER_LED_B);
}

//////////////////////////////////
// PIR motion detection
//////////////////////////////////

int last_readings[PIR_AVERAGING_COUNT];
int last_readings_idx = 0;
int last_readings_sum = 0;

int get_and_set_reading(int reading) {
  int prev_reading = last_readings[last_readings_idx];
  last_readings[last_readings_idx] = reading;
  if (++last_readings_idx >= PIR_AVERAGING_COUNT)
    last_readings_idx = 0;
  return prev_reading;
}

void detect_motion_loop(uint pir_pin, uint led_pin, uint led_pin_above_threshold) {
  gpio_init(pir_pin);
  gpio_set_dir(pir_pin, GPIO_IN);

  bool in_motion = false;
  while (true) {
    int cur_reading = gpio_get(pir_pin);
    int prev_reading = get_and_set_reading(cur_reading);

    if (cur_reading) {
      if (LEDS_ENABLED)
        turn_on_led(led_pin);
      if (!prev_reading && ++last_readings_sum >= PIR_READINGS_MIN_SUM && !in_motion) {
        in_motion = true;
        if (LEDS_ENABLED)
          turn_on_led(led_pin_above_threshold);
        if (AUDIO_ENABLED)
          play_audio(1, false);
      }
    } else {
      if (LEDS_ENABLED)
        turn_off_led(led_pin);
      if (prev_reading && --last_readings_sum < PIR_READINGS_MIN_SUM && in_motion) {
        in_motion = false;
        if (LEDS_ENABLED)
          turn_off_led(led_pin_above_threshold);
      }
    }
    sleep_ms(PIR_READ_INTERVAL);
  }
}


//////////////////////////////////
// audio
//////////////////////////////////
const uint16_t MAX_DMA_TRANSFER_COUNT = 65535;
volatile uint8_t remaining_plays = 0;
uint32_t remaining_transfer_count = 0;
uint pwm_chan;

void trigger_dma_isr() {
  dma_hw->ints0 = 1u << pwm_chan;
  if (remaining_transfer_count == 0) {
    if (--remaining_plays == 0)
      return;
    dma_channel_set_read_addr(pwm_chan, audio_buffer, false);
    remaining_transfer_count = count_of(audio_buffer);
  }
  uint16_t transfer_count = remaining_transfer_count > MAX_DMA_TRANSFER_COUNT ? MAX_DMA_TRANSFER_COUNT : remaining_transfer_count;
  remaining_transfer_count -= transfer_count;
  dma_channel_set_trans_count(pwm_chan, transfer_count, true);
  //dma_channel_start(pwm_chan);
}

void setup_audio(int pin) {
  enum dma_channel_transfer_size dma_size;
  switch (SAMPLE_BITS) {
  case 8: dma_size = DMA_SIZE_8; break;
  case 16: dma_size = DMA_SIZE_16; break;
  case 32: dma_size = DMA_SIZE_32; break;
  }

  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 1.00028955750348785174f);
  pwm_config_set_wrap(&config, PWM_WRAP);
  gpio_set_function(pin, GPIO_FUNC_PWM);
  int slice_num = pwm_gpio_to_slice_num(pin);
  pwm_init(slice_num, &config, true);

  pwm_chan = dma_claim_unused_channel(true);

  dma_channel_config pwm_chan_config = dma_channel_get_default_config(pwm_chan);
  channel_config_set_transfer_data_size(&pwm_chan_config, dma_size);
  channel_config_set_read_increment(&pwm_chan_config, true);
  channel_config_set_write_increment(&pwm_chan_config, false);
  channel_config_set_dreq(&pwm_chan_config, DREQ_PWM_WRAP0 + slice_num);

  uint16_t transfer_count = count_of(audio_buffer) > MAX_DMA_TRANSFER_COUNT ? MAX_DMA_TRANSFER_COUNT : count_of(audio_buffer);
  dma_channel_configure(
                        pwm_chan,
                        &pwm_chan_config,
                        &pwm_hw->slice[slice_num].cc, // write_addr
                        audio_buffer,                 // read_addr
                        transfer_count,               // transfer_count
                        false                         // trigger immediately
                        );

  dma_channel_set_irq0_enabled(pwm_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_0, trigger_dma_isr);
  irq_set_enabled(DMA_IRQ_0, true);
}

void play_audio(uint8_t plays, bool block_until_done) {
  if (remaining_plays != 0) // still running - ignore request to play
    return;

  remaining_plays = plays + 1; // gets decremented immediately once in trigger_dma_isr()
  remaining_transfer_count = 0;

  trigger_dma_isr();

  while (block_until_done && remaining_plays > 0)
    __wfi();
  //    tight_loop_contents();
}
