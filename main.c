#include "pico/stdlib.h"

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

int main() {
  init_leds();
  detect_motion_loop(1, USER_LED_B);
}

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
