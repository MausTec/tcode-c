#include "tcode.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define TCODE_RING_BUFFER_LEN 128

void tcode_callback(tcode_command_t cmd);
void data_callback(tcode_data_command_type_t cmd, FILE *stream);

int app_main() {
    tcode_text_buffer_t tcode_uart_buffer = {
        "", 0, 0
    };
    
    for (;;) {
        // First, check if we can append something to our buffer:
        tcode_getc(stdin, &tcode_uart_buffer);

        char response[40] = "";
        if (tcode_parse_buffer(&tcode_uart_buffer, &data_callback, response, 40) > 0) {
            puts(response);
        }

        // This next bit will execute the events, taking in account any ramping that needs to happen.
        // All the relevant tweening is calculated and stored onto the internal buffer, so we need
        // only worry about handling whatever events tcode tells us to.
        tcode_run_buffer(&tcode_callback, esp_timer_get_time() / 1000L);

        vTaskDelay(1);
    }
}

#ifdef ARDUINO
void setup() { app_main(); }
void loop() {}
#endif

void tcode_callback(tcode_command_t cmd) {
    switch (cmd.type) {
        case TCODE_COMMAND_INVALID:
        case TCODE_COMMAND_LINEAR:
        case TCODE_COMMAND_ROTATE:
        case TCODE_COMMAND_VIBRATE:
        case TCODE_COMMAND_AUXILLIARY:
            printf("Got a T-code command: %d, magnitude: %f", cmd.type, cmd.magnitude);
            break;
    }
}

void data_callback(tcode_data_command_type_t cmd, FILE *stream) {
    switch (cmd) {
        case TCODE_DATA_COMMAND_GETCAPS:
            printf("Got a Data command: %d", cmd);
            fprintf(stream, "?%d", cmd);
            break;
    }
}