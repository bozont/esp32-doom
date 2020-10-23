#include <stdlib.h>
#include "g_game.h"
#include <driver/uart.h>


#define UARTGAMEPAD_KEY_ESC             'n'
#define UARTGAMEPAD_KEY_UP              'w'
#define UARTGAMEPAD_KEY_DOWN            's'
#define UARTGAMEPAD_KEY_LEFT            'a'
#define UARTGAMEPAD_KEY_RIGHT           'd'
#define UARTGAMEPAD_KEY_FIRE            'k'
#define UARTGAMEPAD_KEY_ENTER           'l'
#define UARTGAMEPAD_KEY_USE             'e'
#define UARTGAMEPAD_KEY_WEAPONTOGGLE    'r'
#define UARTGAMEPAD_KEY_MAP             'm'


typedef struct {
    int *key;
    char mapped_uart_char;
    char key_as_text[10];
    int64_t press_time;
} keyMapElement;

static const int64_t key_unpress_delay = 350000;
#define KEY_MAX 10

keyMapElement uart_keymap[KEY_MAX] = {
    {&key_escape,       UARTGAMEPAD_KEY_ESC,            "ESC",      -1},
    {&key_up,           UARTGAMEPAD_KEY_UP,             "UP",       -1},
    {&key_down,         UARTGAMEPAD_KEY_DOWN,           "DOWN",     -1},
    {&key_left,         UARTGAMEPAD_KEY_LEFT,           "LEFT",     -1},
    {&key_right,        UARTGAMEPAD_KEY_RIGHT,          "RIGHT",    -1},
    {&key_fire,         UARTGAMEPAD_KEY_FIRE,           "FIRE",     -1},
    {&key_enter,        UARTGAMEPAD_KEY_ENTER,          "ENTER",    -1},
    {&key_use,          UARTGAMEPAD_KEY_USE,            "USE",      -1},
    {&key_weapontoggle, UARTGAMEPAD_KEY_WEAPONTOGGLE,   "WEAPONTGL",-1},
    {&key_map,          UARTGAMEPAD_KEY_MAP,            "MAP",      -1},
};

void postDoomKeyEvent(int *key, evtype_t key_event_type) {
    event_t ev;
    ev.type = key_event_type;
    ev.data1 = *key;
    D_PostEvent(&ev);
}

void keyCheckAutoUnpress() {
    int64_t curr_time = esp_timer_get_time();

    for(int i = 0 ; i < KEY_MAX ; i++) {
        if((uart_keymap[i].press_time > 0) && (uart_keymap[i].press_time + key_unpress_delay < curr_time)) {
            uart_keymap[i].press_time = -1;
            postDoomKeyEvent(uart_keymap[i].key, ev_keyup);
        }
    }
}

void uartGamepadProcessInput(char input) {

    for(int i = 0 ; i < KEY_MAX ; i++) {
        if(input == uart_keymap[i].mapped_uart_char) {
            uart_keymap[i].press_time = esp_timer_get_time();
            postDoomKeyEvent(uart_keymap[i].key, ev_keydown);
            ets_printf("uartGamepad: %s\n", uart_keymap[i].key_as_text);
            return;
        }
    }

    ets_printf("uartGamepad: unmapped key (%c)\n", input);
}

#define UART_MODULE_NUM    0
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      512

void uartGamepadTask(void *arg) {
    ets_printf("uartGamepad: task starting\n");

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_MODULE_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_MODULE_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_MODULE_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t *uart_rx_buf = (uint8_t*)malloc(UART_BUF_SIZE);
    int uart_buf_iter = 0;
    int uart_rx_buf_len = 0;

    while(1) {
        uart_rx_buf_len = uart_read_bytes(UART_MODULE_NUM, uart_rx_buf, UART_BUF_SIZE, 20 / portTICK_RATE_MS);

        for(uart_buf_iter = 0 ; uart_buf_iter < uart_rx_buf_len ; uart_buf_iter++) {
            uartGamepadProcessInput(uart_rx_buf[uart_buf_iter]);
        }
        /* uart_write_bytes(UART_MODULE_NUM, (const char *) uart_rx_buf, uart_rx_buf_len); */
        keyCheckAutoUnpress();
    }
}

void uartGamepadInit() {
    ets_printf("uartGamepad: initializing\n");
    xTaskCreatePinnedToCore(&uartGamepadTask, "uartgamepad", 1024, NULL, 7, NULL, 0);
}
