#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#include "samples.h"
#define SAMPLE_RATE 44100

#define AUDIO_PIN 0
#define PIR_PIN 1

const uint USER_LED_B = 25;
const uint USER_LED_G = 16;
const uint USER_LED_R = 17;

// PIR reading often flip-flop, which is why we will
// do some averaging here over the last PIR_AVERAGING_COUNT
// PIR readings, taken at intervals of PIR_READ_INTERVAL milliseconds
const uint PIR_READ_INTERVAL = 100;
#define PIR_AVERAGING_COUNT 10

void detect_motion_loop(uint pir_pin, uint led_pin);
void init_leds();
void setup_audio_dma();

int main() {
  init_leds();
  setup_audio_dma();
  detect_motion_loop(PIR_PIN, USER_LED_B);
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
int readings_min_sum = (PIR_AVERAGING_COUNT / 2) + 1;

int get_and_set_reading(int reading) {
    int prev_reading = last_readings[last_readings_idx];
    last_readings[last_readings_idx] = reading;
    if (++last_readings_idx >= PIR_AVERAGING_COUNT)
      last_readings_idx = 0;
    return prev_reading;
}

void detect_motion_loop(uint pir_pin, uint led_pin) {
  gpio_init(pir_pin);
  gpio_set_dir(pir_pin, GPIO_IN);

  while (!true) {
    if (gpio_get(pir_pin)) {
      turn_on_led(led_pin);
    } else {
      turn_off_led(led_pin);
    }
  }
  
  bool in_motion = false;
  while (true) {
    int cur_reading = gpio_get(pir_pin);
    int prev_reading = get_and_set_reading(cur_reading);
    
    if (cur_reading) {
      if (!prev_reading) {
        last_readings_sum++;
        if (last_readings_sum >= readings_min_sum && !in_motion) {
          in_motion = true;
          turn_on_led(led_pin);
        }
      }
    } else {
      if (prev_reading) {
        last_readings_sum--;
        if (last_readings_sum < readings_min_sum && in_motion) {
          in_motion = false;
          turn_off_led(led_pin);
        }
      }
    }
    sleep_ms(PIR_READ_INTERVAL);
  }
}


//////////////////////////////////
// audio
// code taken from
// https://github.com/GregAC/pico-stuff/blob/main/pwm_audio/pwm_audio_dma.c
// as described on his blog
// https://gregchadwick.co.uk/blog/playing-with-the-pico-pt3/
//////////////////////////////////

uint32_t single_sample = 0;
uint32_t* single_sample_ptr = &single_sample;

int pwm_dma_chan, trigger_dma_chan, sample_dma_chan;

#define REPETITION_RATE 4

void dma_irh() {
    dma_hw->ch[sample_dma_chan].al1_read_addr = audio_buffer;
    dma_hw->ch[trigger_dma_chan].al3_read_addr_trig = &single_sample_ptr;

    dma_hw->ints0 = (1u << trigger_dma_chan);
}

void setup_audio_dma() {
    stdio_init_all();

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, (480000.f / SAMPLE_RATE) / REPETITION_RATE);
    pwm_config_set_wrap(&config, 254);
    pwm_init(audio_pin_slice, &config, true);

    pwm_dma_chan = dma_claim_unused_channel(true);
    trigger_dma_chan = dma_claim_unused_channel(true);
    sample_dma_chan = dma_claim_unused_channel(true);

    // Setup PWM DMA channel
    dma_channel_config pwm_dma_chan_config = dma_channel_get_default_config(pwm_dma_chan);
    // Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&pwm_dma_chan_config, DMA_SIZE_32);
    // Read from a fixed location, always writes to the same address
    channel_config_set_read_increment(&pwm_dma_chan_config, false);
    channel_config_set_write_increment(&pwm_dma_chan_config, false);
    // Chain to sample DMA channel when done
    channel_config_set_chain_to(&pwm_dma_chan_config, sample_dma_chan);
    // Transfer on PWM cycle end
    channel_config_set_dreq(&pwm_dma_chan_config, DREQ_PWM_WRAP0 + audio_pin_slice);

    dma_channel_configure(
        pwm_dma_chan,
        &pwm_dma_chan_config,
        // Write to PWM slice CC register
        &pwm_hw->slice[audio_pin_slice].cc,
        // Read from single_sample
        &single_sample,
        // Transfer once per desired sample repetition
        REPETITION_RATE,
        // Don't start yet
        false
    );

    // Setup trigger DMA channel
    dma_channel_config trigger_dma_chan_config = dma_channel_get_default_config(trigger_dma_chan);
    // Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&trigger_dma_chan_config, DMA_SIZE_32);
    // Always read and write from and to the same address
    channel_config_set_read_increment(&trigger_dma_chan_config, false);
    channel_config_set_write_increment(&trigger_dma_chan_config, false);
    // Transfer on PWM cycle end
    channel_config_set_dreq(&trigger_dma_chan_config, DREQ_PWM_WRAP0 + audio_pin_slice);

    dma_channel_configure(
        trigger_dma_chan,
        &trigger_dma_chan_config,
        // Write to PWM DMA channel read address trigger
        &dma_hw->ch[pwm_dma_chan].al3_read_addr_trig,
        // Read from location containing the address of single_sample
        &single_sample_ptr,
        // Need to trigger once for each audio sample but as the PWM DREQ is
        // used need to multiply by repetition rate
        REPETITION_RATE * sizeof(audio_buffer) / sizeof(audio_buffer[0]),
        false
    );

    // Fire interrupt when trigger DMA channel is done
    dma_channel_set_irq0_enabled(trigger_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irh);
    irq_set_enabled(DMA_IRQ_0, true);

    // Setup sample DMA channel
    dma_channel_config sample_dma_chan_config = dma_channel_get_default_config(sample_dma_chan);
    // Transfer 8-bits at a time
    channel_config_set_transfer_data_size(&sample_dma_chan_config, DMA_SIZE_8);
    // Increment read address to go through audio buffer
    channel_config_set_read_increment(&sample_dma_chan_config, true);
    // Always write to the same address
    channel_config_set_write_increment(&sample_dma_chan_config, false);

    dma_channel_configure(
        sample_dma_chan,
        &sample_dma_chan_config,
        // Write to single_sample
        &single_sample,
        // Read from audio buffer
        audio_buffer,
        // Only do one transfer (once per PWM DMA completion due to chaining)
        1,
        // Don't start yet
        false
    );

    // Kick things off with the trigger DMA channel
    dma_channel_start(trigger_dma_chan);
}
