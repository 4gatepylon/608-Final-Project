#include "Arduino.h"

int counter0 = 0;
int counter1 = 0;

void loop0(void *parameter)
{
    for (;;)
    {
        // Every second
        Serial.printf("Task 0: %d\n", counter0);
        counter0++;
        delay(1000);
    }
}

void loop1(void *parameter)
{
    for (;;)
    {
        // Every 5 seconds
        Serial.printf("Task 1: %d\n", counter1);
        counter1++;
        delay(5000);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup started.");

    // Use pinned to core so that it'll be scheduled on the same core
    // and that we we can run in parallel.
    xTaskCreatePinnedToCore(
        loop0,   /* Function to implement the task */
        "Task0", /* Name of the task */
        1000,    /* Stack size in words */
        NULL,    /* Task input parameter */
        0,       /* Priority of the task */
        NULL,    /* Task handle: not used since tasks are independent. */
        0);      /* Core where the task should run */

    xTaskCreatePinnedToCore(
        loop1,   /* Function to implement the task */
        "Task1", /* Name of the task */
        1000,    /* Stack size in words */
        NULL,    /* Task input parameter */
        1,       /* Priority of the task */
        NULL,    /* Task handle: not used since tasks are independent. */
        1);      /* Core where the task should run */
    Serial.println("Setup completed.");
}

void loop()
{
    delay(1);
}