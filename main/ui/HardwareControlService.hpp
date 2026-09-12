#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "lvgl.h"
#include "RadarTypes.hpp"

class RadarApp;

/*
 * Handles the current physical controls: confirm/back buttons, rotary encoder,
 * encoder push button, and the small on-device menu driven by those inputs.
 *
 * RadarApp still owns the actual radar state. This class owns the GPIO/knob
 * state and the LVGL menu widgets, then calls back into RadarApp for actions.
 */
class HardwareControlService {
public:
    void bind(RadarApp *app);
    void initInputs();
    void createMenu(lv_obj_t *screen);
    void refreshFonts();
    void refreshColors();
    void refreshMenu();
    void showMenu();
    void hideMenu();
    void moveSelection(int delta);
    void selectItem();
    void applyButtonAction(int action);
    void timerTick();

    static void timerEntry(lv_timer_t *timer);

private:
    static void knobLeftEntry(void *knob, void *user_data);
    static void knobRightEntry(void *knob, void *user_data);

    bool consumeButtonPress(gpio_num_t gpio, int *last_level, int64_t *last_change_us);
    bool menuVisible() const;
    gpio_num_t gpioFromSetting(int value) const;

    /* Read a possibly-unassigned input pin, reporting an unwired one as released. */
    static int readLevel(gpio_num_t gpio);

    RadarApp *owner = nullptr;
    void *rotary_knob_handle = nullptr;
    volatile int encoder_pending_delta = 0;
    int encoder_event_accum = 0;
    int confirm_last_level = 1;
    int back_last_level = 1;
    int push_last_level = 1;
    int64_t confirm_last_change_us = 0;
    int64_t back_last_change_us = 0;
    int64_t push_last_change_us = 0;
    int selected = 0;
    int64_t last_use_us = 0;
    /*
     * Unassigned until initInputs() reads the persisted pin choices. This board wires no
     * encoder or buttons of its own, and its only spare-looking pins (47/48) are the shared
     * I2C bus, so there is no safe pin to default to.
     */
    gpio_num_t confirm_gpio = GPIO_NUM_NC;
    gpio_num_t back_gpio = GPIO_NUM_NC;
    gpio_num_t rotary_a_gpio = GPIO_NUM_NC;
    gpio_num_t rotary_b_gpio = GPIO_NUM_NC;
    gpio_num_t rotary_push_gpio = GPIO_NUM_NC;

    lv_obj_t *menu = nullptr;
    lv_obj_t *title = nullptr;
    lv_obj_t *rows[10] = {};
    lv_obj_t *labels[10] = {};
    lv_obj_t *values[10] = {};
    lv_obj_t *help = nullptr;
};
