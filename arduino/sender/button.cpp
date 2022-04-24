#include "button.h"

Button::Button(int p)
{
    flag = 0;
    state = S0;
    pin = p;
    S2_start_time = millis();      // init
    button_change_time = millis(); // init
    debounce_duration = 10;
    long_press_duration = 1000;
    button_pressed = 0;
}
void Button::read()
{
    uint8_t button_val = digitalRead(pin);
    button_pressed = !button_val; // invert button
}
int Button::update()
{
    read();
    flag = 0;
    int m = 0;
    switch (state)
    {
    case S0:
        if (button_pressed)
        {
            button_change_time = millis();
            state = S1;
        }
        break;
    case S1:
        m = millis();
        if (button_pressed && m - button_change_time > debounce_duration)
        {
            S2_start_time = m;
            state = S2;
        }
        else if (!button_pressed)
        {
            button_change_time = m;
            state = S0;
        }
        break;
    case S2:
        if (!button_pressed)
        {
            button_change_time = millis();
            state = S4;
        }
        else if (millis() - S2_start_time > long_press_duration)
        {
            state = S3;
        }
        break;
    case S3:
        if (!button_pressed)
        {
            button_change_time = millis();
            state = S4;
        }
        break;
    case S4:
        m = millis();
        if (!button_pressed && m - button_change_time > debounce_duration)
        {
            flag = 1;
            if (button_change_time - S2_start_time > long_press_duration)
            {
                flag = 2;
            }
            state = S0;
        }
        else if (button_pressed)
        {
            if (m - S2_start_time > long_press_duration)
            {
                state = S3;
            }
            else
            {
                state = S2;
            }
            button_change_time = m;
        }
        break;
    }
    return flag;
};