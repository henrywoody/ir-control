// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(GPIO_PIN_NUM) *(gpio+((GPIO_PIN_NUM)/10)) &= ~(7<<(((GPIO_PIN_NUM)%10)*3))
#define OUT_GPIO(GPIO_PIN_NUM) *(gpio+((GPIO_PIN_NUM)/10)) |=  (1<<(((GPIO_PIN_NUM)%10)*3))
#define SET_GPIO_ALT(GPIO_PIN_NUM,a) *(gpio+(((GPIO_PIN_NUM)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((GPIO_PIN_NUM)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(GPIO_PIN_NUM) (*(gpio+13)&(1<<GPIO_PIN_NUM)) // 0 if LOW, (1<<GPIO_PIN_NUM) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

// Time & Pulse control macros
#define SET_START_TIME gettimeofday(&tv_signal_start, NULL);
#define UPDATE_CURRENT_TIME gettimeofday(&tv_current, NULL);
#define SEND_INIT_SIGNAL turn_led_on_for(9000); sleep_for(4500)
#define SEND_ONE turn_led_on_for(PULSE_WIDTH); sleep_for(ONE_WIDTH)
#define SEND_ZERO turn_led_on_for(PULSE_WIDTH); sleep_for(ZERO_WIDTH)
#define PULSE turn_led_on_for(PULSE_WIDTH)
#define TURN_LED_ON GPIO_SET = 1<<GPIO_PIN_NUM
#define TURN_LED_OFF GPIO_CLR = 1<<GPIO_PIN_NUM

void turn_led_on_for();
void sleep_for();
int carrier_sleep_for();
void sleep_until();
void setup_gpio();
void setup_io();

const int GPIO_PIN_NUM = 17;
const char ZERO_CHAR = 48;
const char ONE_CHAR = 49;
const int PULSE_WIDTH = 560; // us
const int ZERO_WIDTH = 560; // us
const int ONE_WIDTH = 1680; // us
const int REPEAT_WIDTH = 110000; // us
const int CARRIER_SIGNAL_PERIOD = 26; // (26.3) us (38kHz)
const float DUTY_CYCLE = 0.33;
const int CARRIER_SIGNAL_ON_TIME = 9; // CARRIER_SIGNAL_PERIOD * DUTY_CYCLE
const int CARRIER_SIGNAL_OFF_TIME = 17; //CARRIER_SIGNAL_PERIOD *  (1 - DUTY_CYCLE)

int time_tracker = 0;
/* 
    time_tracker is like ideal time ellapsed so that errors in real time do not propagate
    (e.g. if one pulse takes a little too long, then the next pulse would start a little later than it should, and so on...
        ideal time handles that by ignoring the actual time taking by a pulse and just tracks how long that pulse should have taken)
*/
struct timeval tv_signal_start;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Need to give a code to send\n");
        return 1;
    }
    char *code = argv[1]; // binary string

    setup_gpio();

    SET_START_TIME;
    SEND_INIT_SIGNAL;

    for (int i = 0; code[i] != '\0'; i++) {
        if (code[i] == ONE_CHAR) {
            SEND_ONE;
        } else {
            SEND_ZERO;
        }
    }

    PULSE;

    return 0;
}

void turn_led_on_for(int durr_us) {
    int carrier_time_tracker = time_tracker;
    time_tracker += durr_us;

    struct timeval tv_current;
    UPDATE_CURRENT_TIME;

    while ((tv_current.tv_sec - tv_signal_start.tv_sec) * 1000000 + tv_current.tv_usec - tv_signal_start.tv_usec < time_tracker) {
        TURN_LED_ON;
        carrier_time_tracker += carrier_sleep_for(CARRIER_SIGNAL_ON_TIME, carrier_time_tracker);

        TURN_LED_OFF;
        carrier_time_tracker += carrier_sleep_for(CARRIER_SIGNAL_OFF_TIME, carrier_time_tracker);

        UPDATE_CURRENT_TIME;
    }
}

void sleep_for(int durr_us) {
    struct timeval tv_current;
    time_tracker += durr_us;

    do {
        UPDATE_CURRENT_TIME;
    } while ((tv_current.tv_sec - tv_signal_start.tv_sec) * 1000000 + tv_current.tv_usec - tv_signal_start.tv_usec < time_tracker);
}

int carrier_sleep_for(int durr_us, int carrier_time_tracker) {
    struct timeval tv_current;
    int target_us = carrier_time_tracker + durr_us;

    do {
        UPDATE_CURRENT_TIME;
    } while ((tv_current.tv_sec - tv_signal_start.tv_sec) * 1000000 + tv_current.tv_usec - tv_signal_start.tv_usec < target_us);

    return durr_us;
}

void setup_gpio() {
    // Set up gpi pointer for direct register access
    setup_io();

    // Set GPIO pin to output
    INP_GPIO(GPIO_PIN_NUM); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(GPIO_PIN_NUM);
}

//  
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
//  From https://elinux.org/RPi_GPIO_Code_Samples#Direct_register_access

//
// Set up a memory regions to access GPIO
//
void setup_io() {
     /* open /dev/mem */
     if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
            printf("can't open /dev/mem \n");
            exit(-1);
     }

     /* mmap GPIO */
     gpio_map = mmap(
            NULL,             //Any adddress in our space will do
            BLOCK_SIZE,       //Map length
            PROT_READ|PROT_WRITE,// Enable reading & writing to mapped memory
            MAP_SHARED,       //Shared with other processes
            mem_fd,           //File to map
            GPIO_BASE         //Offset to GPIO peripheral
     );

     close(mem_fd); //No need to keep mem_fd open after mmap

     if (gpio_map == MAP_FAILED) {
            printf("mmap error %d\n", (int)gpio_map);//errno also set!
            exit(-1);
     }

     // Always use volatile pointer!
     gpio = (volatile unsigned *)gpio_map;


}