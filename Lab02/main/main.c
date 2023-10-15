#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Print ”ESP32” when the button is pressed
static void button_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("ESP32\n");
        }
    }
}
// Print group members identifier every second
static void print_task(){
    while(true) {
        printf("1914237 1914961\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //interrupt of falling edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO5 here
    //Button is connected to Pin 5
    io_conf.pin_bit_mask = 1ULL<<GPIO_NUM_5;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //start gpio tasks
    xTaskCreate(button_task, "print ESP", 2048, NULL, 2, NULL);
    xTaskCreate(print_task, "print MSSV", 4096, NULL, 1, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void*) GPIO_NUM_5);

}
