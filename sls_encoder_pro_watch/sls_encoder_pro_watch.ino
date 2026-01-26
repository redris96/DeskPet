/*
nikthefix 18.06.2024

This is the Squareline Studio 1.4.1 Smart Watch Demo implemented for the Lilygo T-Encoder-Pro
At present the encoder dial and buzzer have not been assigned a function.
Tested with Arduino IDE V2.3.2


Dependencies:

ESP32_Arduino boards package version 3.0.1
LVGL 8.3.11

Notes:

This build uses a lean SH8601 display driver rather than the Arduino_GFX framework used in the Lilygo supplied examples.
The driver files are supplied in the sketch directory so no installation is required.
The touch driver is also included in the sketch directory so no installation is required.
I recommend a clean LVGL install via the Arduino library manager.


Please set in lv_conf.h:  --->   #if 1                                  (line 15)
                          --->   #define LV_COLOR_16_SWAP 1             (line 30)
                          --->   #define LV_MEM_CUSTOM 1                (line 49)
                          --->   #define LV_TICK_CUSTOM 1               (line 88)
                          --->   #define LV_FONT_MONTSERRAT_14 1        (line 367)

Set display brightness @ line 123


Build options:

Select board ESP32S3 Dev Module
Select USB CDC On Boot "Enabled"
Select Flash Size 16M
Select Partition Scheme "Huge App"
Select PSRAM "OPI PSRAM"

Todo:

Refactor to make compatible with LVGL 9.1 (proving problematic at last attempt)

*/



#include <Arduino.h>
#include "sh8601.h"
#include "TouchDrvCHSC5816.hpp"
#include "lvgl.h"
#include "SPI.h"
#include "ui.h"
#include "pins_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "include/secrets.h"

const char* weather_url = "http://api.openweathermap.org/data/2.5/weather?lat=12.97&lon=77.59&units=metric&appid=" OPEN_WEATHER_API_KEY;

#include "PetEngine.h"
#include "ReaderEngine.h"
#include <BleMouse.h>

// --- Global App Engines ---
PetEngine pet;
ReaderEngine reader;
BleMouse bleMouse("DeskPet Knob", "Antigravity", 100);

// --- Global State ---
int current_app_index = 0; // 0=Clock, 1=Analog, 2=Pet, 3=Weather, 4=Reader
const int MAX_APPS = 5;

// Shared UI pointer for Reader Screen
lv_obj_t* ui_reader_screen = NULL;

// --- Encoder Global ---
volatile int encoderPos = 0;

// Forward declarations
void perform_screen_switch(int index);

// Shared UI pointer for Reader Screen
// Shared UI pointer for Reader Screen
// lv_obj_t* ui_reader_screen = NULL; // Removed duplicate

void playTone(int freq, int duration) {
    ledcWriteTone(0, freq); // Use channel 0
    delay(duration);
    ledcWriteTone(0, 0);
}

void squeak() { playTone(2000, 50); playTone(2500, 50); }
void growl() { for(int f=200; f>100; f-=10) playTone(f, 20); }
void snore() { playTone(300, 100); }

void updateWeather() {
    Serial.println("Update Weather...");
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(weather_url);
        http.setTimeout(2000); // 2-second timeout to prevent loop freeze
        int httpCode = http.GET();
        
        if (httpCode > 0) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                float temp = doc["main"]["temp"];
                const char* description = doc["weather"][0]["main"];
                
                if (ui_weather_title_group_1) {
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_1, UI_COMP_TITLEGROUP_TITLE), "%s", description);
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_1, UI_COMP_TITLEGROUP_SUBTITLE), "Temp: %d C", (int)temp);
                }
                if (ui_weather_title_group_2) {
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_2, UI_COMP_TITLEGROUP_TITLE), "%s", description);
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_2, UI_COMP_TITLEGROUP_SUBTITLE), "Temp: %d C", (int)temp);
                }
                if (ui_weather_title_group_3) {
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_3, UI_COMP_TITLEGROUP_TITLE), "%s", description);
                    lv_label_set_text_fmt(ui_comp_get_child(ui_weather_title_group_3, UI_COMP_TITLEGROUP_SUBTITLE), "Temp: %d C", (int)temp);
                }
                if (ui_degree_7) {
                    lv_obj_set_style_text_font(ui_degree_7, &ui_font_Number_extra, 0); 
                    lv_label_set_text_fmt(ui_degree_7, "%d", (int)temp);
                }
                if (ui_label_degree) {
                    lv_obj_set_style_text_font(ui_label_degree, &ui_font_Number_extra, 0);
                    lv_label_set_text_fmt(ui_label_degree, "%d", (int)temp);
                }
                Serial.println("Weather Updated Successfully");
            } else {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.printf("Error on HTTP request: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("WiFi not connected - skipped weather update");
    }
}

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
lv_obj_t* wifi_ind;

void setupWiFi() {
    Serial.printf("Connecting to %s\n", ssid);
    WiFi.begin(ssid, password);
    
    int timeout_counter = 0;
    while (WiFi.status() != WL_CONNECTED && timeout_counter < 10) { // Timeout after ~5 seconds
        delay(500);
        Serial.print(".");
        timeout_counter++;
        lv_timer_handler(); 
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi CONNECTED");
        configTime(0, 0, "pool.ntp.org");
        setenv("TZ", "IST-5:30", 1); 
        tzset();
    } else {
        Serial.println("\nWiFi Connection Failed - Continuing Offline");
    }
}

void update_time_ui() {
    static unsigned long last_update = 0;
    if (millis() - last_update < 1000) return; // Update every 1s
    last_update = millis();

    // 1. Update WiFi Indicator
    if (wifi_ind) {
        if (WiFi.status() == WL_CONNECTED) {
            lv_obj_set_style_bg_color(wifi_ind, lv_color_white(), 0);
        } else {
            lv_obj_set_style_bg_color(wifi_ind, lv_color_hex(0x555555), 0); // Dim Grey
        }
    }

    // 2. Get System Time (Defensive Fix)
    struct tm timeinfo = {0}; // Initialize to zero
    if(!getLocalTime(&timeinfo)){
        Serial.println("getLocalTime() Failed - skipping update");
        return; 
    }
    
    // Garbage Year Check (NTP sometimes returns 1970)
    if(timeinfo.tm_year < 120) { // < Year 2020
        Serial.println("System time not sync'd (pre-2020) - skipping UI update");
        return; 
    }

    char buf[32];

    // 3. Update Digital Clock (12-Hour Format & Layout Fixes)
    static int last_h12 = -1;
    static int last_min = -1;
    static int last_sec = -1;

    int h12 = timeinfo.tm_hour % 12;
    if (h12 == 0) h12 = 12;

    if (h12 != last_h12 || timeinfo.tm_min != last_min) {
        if (ui_label_hour_1) {
            lv_obj_clear_flag(ui_label_hour_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_x(ui_label_hour_1, 20); // Moved left to avoid Date overlap
            lv_obj_set_y(ui_label_hour_1, 55); 
            lv_label_set_text_fmt(ui_label_hour_1, "%d", h12 / 10);
        }
        if (ui_label_hour_2) {
            lv_obj_clear_flag(ui_label_hour_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_x(ui_label_hour_2, 110); 
            lv_obj_set_y(ui_label_hour_2, 55);
            lv_label_set_text_fmt(ui_label_hour_2, "%d", h12 % 10);
        }
        if (ui_label_min) {
            lv_obj_clear_flag(ui_label_min, LV_OBJ_FLAG_HIDDEN);
            // Switch to smaller font to prevent spilling off screen
            lv_obj_set_style_text_font(ui_label_min, &ui_font_Number_big, 0); 
            lv_obj_set_x(ui_label_min, 205); 
            lv_obj_set_y(ui_label_min, 165); 
            lv_label_set_text_fmt(ui_label_min, "%02d", timeinfo.tm_min);
        }
        if (ui_clock) lv_label_set_text_fmt(ui_clock, "%02d   %02d", h12, timeinfo.tm_min);
    }

    // 4. Update Digital Date Group (Update daily)
    static int last_mday = -1;
    if (timeinfo.tm_mday != last_mday) {
        if (ui_date_group) {
            strftime(buf, sizeof(buf), "%a", &timeinfo);
            lv_label_set_text(ui_comp_get_child(ui_date_group, UI_COMP_DATEGROUP_DAY), buf);
            strftime(buf, sizeof(buf), "%d %b", &timeinfo);
            lv_label_set_text(ui_comp_get_child(ui_date_group, UI_COMP_DATEGROUP_MONTH), buf);
            strftime(buf, sizeof(buf), "%Y", &timeinfo);
            lv_label_set_text(ui_comp_get_child(ui_date_group, UI_COMP_DATEGROUP_YEAR), buf);
        }
        if (ui_day2) {
            strftime(buf, sizeof(buf), "%a", &timeinfo);
            lv_label_set_text(ui_day2, buf);
        }
        if (ui_month2) {
            strftime(buf, sizeof(buf), "%d %b", &timeinfo);
            lv_label_set_text(ui_month2, buf);
        }
        if (ui_year2) {
            lv_label_set_text_fmt(ui_year2, "%d", 1900 + timeinfo.tm_year);
        }
        
        // Update City/Date labels on weather screens
        if (ui_city_gruop_1) {
            lv_label_set_text(ui_comp_get_child(ui_city_gruop_1, UI_COMP_TITLEGROUP_TITLE), "Bengaluru");
            strftime(buf, sizeof(buf), "%d. %m. %A", &timeinfo);
            lv_label_set_text(ui_comp_get_child(ui_city_gruop_1, UI_COMP_TITLEGROUP_SUBTITLE), buf);
        }
        if (ui_city_gruop_2) {
            lv_label_set_text(ui_comp_get_child(ui_city_gruop_2, UI_COMP_TITLEGROUP_TITLE), "Bengaluru");
            strftime(buf, sizeof(buf), "%d. %m. %A", &timeinfo);
            lv_label_set_text(ui_comp_get_child(ui_city_gruop_2, UI_COMP_TITLEGROUP_SUBTITLE), buf);
        }
        last_mday = timeinfo.tm_mday;
    }

    // 5. Update Analog Clock hands
    // ... (No change here, just anchoring to find a good spot if needed, but I'll skip this chunk if not needed)
    // Actually, I need to insert the Encoder Handler. I'll find the END of `encoder_handler` logic or `loop` logic.
    // The previous tool call failed on the `perform_screen_switch` area.
    // Let's replace the loop logic which contained the switch. 
    // Wait, the file doesn't have `perform_screen_switch` yet? 
    // The previous search showed lines 620-745 handling encoder.
    // I need to REPLACE the encoder logic.

    if (timeinfo.tm_sec != last_sec || timeinfo.tm_min != last_min) {
        if (ui_hour) {
            int hour_angle = ((timeinfo.tm_hour % 12) * 300) + (timeinfo.tm_min * 5);
            lv_img_set_angle(ui_hour, hour_angle); 
        }
        if (ui_min) {
            int min_angle = timeinfo.tm_min * 60; 
            lv_img_set_angle(ui_min, min_angle);
        }
        if (ui_sec) {
            int sec_angle = timeinfo.tm_sec * 60;
            lv_img_set_angle(ui_sec, sec_angle);
        }
    }

    // Update battery only periodically (or if it were real data, on change)
    static uint32_t last_batt_update = 0;
    if (millis() - last_batt_update > 60000) {
        if (ui_battery_group) {
            lv_label_set_text(ui_comp_get_child(ui_battery_group, UI_COMP_BATTERYGROUP_BATTERY_PERCENT), "100%");
        }
        if (ui_battery_group1) {
            lv_label_set_text(ui_comp_get_child(ui_battery_group1, UI_COMP_BATTERYGROUP_BATTERY_PERCENT), "100%");
        }
        last_batt_update = millis();
    }

    last_h12 = h12;
    last_min = timeinfo.tm_min;
    last_sec = timeinfo.tm_sec;
}


TouchDrvCHSC5816 touch;
TouchDrvInterface *pTouch;

void CHSC5816_Initialization(void)
{
    TouchDrvCHSC5816 *pd1 = static_cast<TouchDrvCHSC5816 *>(pTouch);

    touch.setPins(TOUCH_RST, TOUCH_INT);
    if (!touch.begin(Wire, CHSC5816_SLAVE_ADDRESS, IIC_SDA, IIC_SCL))
    {
        Serial.println("Failed to find CHSC5816 - Hardware mismatch or wiring issue!");
        // DO NOT HANG HERE - allow display to continue
    } else {
        Serial.println("Touch Sensor Initialized.");
    }
}



void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd_PushColors(area->x1, area->y1, w, h, (uint16_t *)&color_p->full); 
    lv_disp_flush_ready(disp);
}


void my_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
{

   area->x1 = area->x1 & 0xFFFE;     // round down the refresh area x-axis start point to next even number - required for this display
   area->x2 = (area->x2 & 0xFFFE)+1; // round down the refresh area x-axis end point to next even number - required for this display

   area->y1 = area->y1 & 0xFFFE;     // round down the refresh area y-axis start point to next even number - required for this display
   area->y2 = (area->y2 & 0xFFFE)+1; // round down the refresh area y-axis end point to next even number - required for this display
}

lv_obj_t *ui_pet_screen;
uint8_t global_brightness = 200;
bool is_settings_mode = false;

// Forward declarations
static void openSettings();

// --- Brightness Logic ---

static void brightness_arc_event_cb(lv_event_t * e) {
    lv_obj_t * arc = lv_event_get_target(e);
    int value = (int)lv_arc_get_value(arc);
    // Convert 0-100% to 10-255
    global_brightness = map(value, 0, 100, 10, 255);
    lcd_brightness(global_brightness);
    if (ui_volume_percent) {
        lv_label_set_text_fmt(ui_volume_percent, "%d%%", value);
    }
}

static void exit_settings_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        is_settings_mode = false;
        // The original callback in ui.c will handle the screen change to ui_watch_digital
    }
}

static void openSettings() {
    is_settings_mode = true;
    
    // Configure ui_call screen components for Brightness Settings
    if (ui_call) {
        if (ui_avatar_label) {
            lv_label_set_text(ui_avatar_label, "Brightness");
            lv_obj_clear_flag(ui_avatar_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(ui_avatar_label, -140); // Move title up
        }
        if (ui_avatar) lv_obj_add_flag(ui_avatar, LV_OBJ_FLAG_HIDDEN);
        if (ui_call_time) lv_obj_add_flag(ui_call_time, LV_OBJ_FLAG_HIDDEN);
        if (ui_mute) lv_obj_add_flag(ui_mute, LV_OBJ_FLAG_HIDDEN);
        
        // Hide/Disable other buttons to prevent accidental clicks while sliding arc
        if (ui_button_top2) lv_obj_add_flag(ui_button_top2, LV_OBJ_FLAG_HIDDEN);
        
        if (ui_volume_group) {
            lv_obj_clear_flag(ui_volume_group, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_align(ui_volume_group, LV_ALIGN_CENTER); // Center the group
            lv_obj_set_x(ui_volume_group, 5);
            lv_obj_set_y(ui_volume_group, 45);
        }
        
        if (ui_volume_arc) {
            int perc = map(global_brightness, 10, 255, 0, 100);
            lv_arc_set_value(ui_volume_arc, perc);
            if (ui_volume_percent) {
                lv_label_set_text_fmt(ui_volume_percent, "%d%%", perc);
                lv_obj_set_align(ui_volume_percent, LV_ALIGN_CENTER);
                lv_obj_set_x(ui_volume_percent, 0);
                lv_obj_set_y(ui_volume_percent, 0);
            }
            if (ui_volume_image) lv_obj_add_flag(ui_volume_image, LV_OBJ_FLAG_HIDDEN); // Hide volume icon
            
            // Clean old events and add our brightness handler
            lv_obj_remove_event_cb(ui_volume_arc, NULL); // Remove all
            lv_obj_add_event_cb(ui_volume_arc, brightness_arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        }

        if (ui_button_down2) {
            lv_obj_add_event_cb(ui_button_down2, exit_settings_cb, LV_EVENT_CLICKED, NULL);
        }

        // Switch to the screen
        _ui_screen_change(&ui_call, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_call_screen_init);
    }
}

static void ui_event_settings_redirect(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        openSettings();
    }
}

static void lv_indev_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int16_t Touch_x[2], Touch_y[2];
    uint8_t touchpad = touch.getPoint(Touch_x, Touch_y);

    static uint32_t press_start = 0;
    static bool was_pressed = false;

    if (touchpad > 0)
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = Touch_x[0];
        data->point.y = Touch_y[0];

        if (!was_pressed) {
            press_start = millis();
            was_pressed = true;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
        if (was_pressed) {
            uint32_t duration = millis() - press_start;
            if (lv_scr_act() == ui_pet_screen) {
                if (duration < 500) { // Short Tap -> Happy
                    if (pet.getMood() != SLEEPY) {
                        pet.setMood(HAPPY);
                        squeak();
                    } else {
                        // Wake up if tapped while sleeping? Or ignore?
                        // Let's wake up on long press, maybe just ignore short tap or grunt
                        snore();
                    }
                } 
                else if (duration > 800) { // Long Press -> Sleep/Wake
                   if (pet.getMood() == SLEEPY) {
                       pet.setMood(NEUTRAL); // Wake up
                       squeak();
                   } else {
                       pet.setMood(SLEEPY); // Go to sleep
                       snore();
                   }
                }
            }
            was_pressed = false;
        }
    }
}

// -------------------------------------------------------------------------
// DESK PET LOGIC
// -------------------------------------------------------------------------
lv_obj_t *pet_layer;
lv_obj_t *eye_left;
lv_obj_t *eye_right;

void create_pet_layer() {
    pet_layer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(pet_layer, LV_PCT(100), LV_PCT(100)); // Full screen
    lv_obj_set_style_bg_color(pet_layer, lv_color_black(), 0);
    lv_obj_set_style_border_width(pet_layer, 0, 0);
    lv_obj_set_style_radius(pet_layer, 0, 0);
    lv_obj_clear_flag(pet_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pet_layer, LV_OBJ_FLAG_HIDDEN); // Hidden by default
    
    // Eyes: Cyan rectangles, radius 20
    eye_left = lv_obj_create(pet_layer);
    lv_obj_set_size(eye_left, 40, 60); 
    lv_obj_set_style_bg_color(eye_left, lv_color_hex(0x00FFFF), 0); // Cyan
    lv_obj_set_style_radius(eye_left, 20, 0);
    lv_obj_set_style_border_width(eye_left, 0, 0);
    lv_obj_align(eye_left, LV_ALIGN_CENTER, -35, 0);
    
    eye_right = lv_obj_create(pet_layer);
    lv_obj_set_size(eye_right, 40, 60);
    lv_obj_set_style_bg_color(eye_right, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_radius(eye_right, 20, 0);
    lv_obj_set_style_border_width(eye_right, 0, 0);
    lv_obj_align(eye_right, LV_ALIGN_CENTER, 35, 0);
}

void pet_loop_logic() {
    // Button Handler
    static bool last_btn_state = HIGH;
    bool btn_state = digitalRead(KNOB_KEY);
    
    // Reader Mode Toggle (Debounced in logic or here)
    if (last_btn_state == HIGH && btn_state == LOW) { // Press
        // If Reader App is active, toggle mode
        if (reader.getIsActive()) {
            reader.onButtonPress();
            delay(50); // Simple debounce
        }
    }
    last_btn_state = btn_state;

    // Update Engines
    pet.update();
    reader.update();
}

void setup()
{
    // Buzzer setup (Core 2.x API)
    ledcSetup(0, 2000, 8); // Channel 0, 2000Hz, 8-bit
    ledcAttachPin(BUZZER_DATA, 0); 

    pinMode(LCD_VCI_EN, OUTPUT);
    digitalWrite(LCD_VCI_EN, HIGH);//enable display hardware

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *buf;

    Serial.begin(115200);
    delay(500);
    Serial.println("Booting DeskPet...");
    
    // Init BLE Mouse
    // Init BLE Mouse
    Serial.println("Init BLE Mouse...");
    // bleMouse.begin(); // LAZY INIT: Started by ReaderEngine on demand

    Serial.println("Init Touch...");
    CHSC5816_Initialization();
    
    Serial.println("Init Display...");
    sh8601_init();
    lcd_brightness(200); 
    
    Serial.println("Init LVGL...");
    lv_init();
    
    Serial.println("Allocating Framebuffer...");
    size_t buf_size = sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE;
    // Prefer SPIRAM (PSRAM) for this large buffer
    buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!buf) {
        Serial.println("PSRAM allocation failed! Trying internal RAM...");
        buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL);
    }
    
    if (!buf) {
        Serial.println("FATAL: Memory allocation failed for display buffer!");
        while(1) delay(1000);
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);
    
    Serial.println("Registering Display Driver...");
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    Serial.println("Registering Input Device...");
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_indev_read;
    lv_indev_drv_register(&indev_drv);
        
    Serial.println("Initializing UI Components...");
    ui_init();
    
    if (ui_watch_digital) {
        wifi_ind = lv_obj_create(ui_watch_digital);
        lv_obj_set_size(wifi_ind, 10, 10);
        lv_obj_set_style_radius(wifi_ind, 5, 0); 
        lv_obj_set_style_bg_color(wifi_ind, lv_color_hex(0x555555), 0);
        lv_obj_set_style_border_width(wifi_ind, 0, 0);
        lv_obj_align(wifi_ind, LV_ALIGN_TOP_RIGHT, -25, 25); 
        lv_obj_clear_flag(wifi_ind, LV_OBJ_FLAG_SCROLLABLE);
    }
    
    Serial.println("Creating Pet App Screen...");
    ui_pet_screen = lv_obj_create(NULL); // Create as a top-level screen
    pet.init(ui_pet_screen);
    
    Serial.println("Creating Reader App Screen...");
    reader.init(NULL); // It creates its own internal screen if parent is NULL? 
    // Wait, ReaderEngine::init uses parent directly.
    // We need a screen container for it like pet_screen
    // Let's modify ReaderEngine::init briefly or just create a screen here
    // ReaderEngine above takes a parent.
    ui_reader_screen = lv_obj_create(NULL);
    reader.init(ui_reader_screen);

    // Repurpose Call Button to Settings on Digital Watch
    if (ui_button_top) {
        lv_obj_t * icon = ui_comp_get_child(ui_button_top, UI_COMP_BUTTONTOP_BUTTON_TOP_ICON);
        if (icon) {
            lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_t * gear = lv_label_create(ui_button_top);
            lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
            lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
            // Revert to Montserrat 14 (safe) without zoom (user requested reliability)
            lv_obj_set_style_text_font(gear, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(gear, lv_color_white(), 0);
            lv_obj_center(gear);
        }
        lv_obj_remove_event_cb(ui_button_top, ui_event_button_top_buttontop);
        lv_obj_add_event_cb(ui_button_top, ui_event_settings_redirect, LV_EVENT_CLICKED, NULL);
    }

    // Repurpose Call Button on Analog Watch Face (ui_button_top1)
    if (ui_button_top1) {
        lv_obj_t * icon = ui_comp_get_child(ui_button_top1, UI_COMP_BUTTONTOP_BUTTON_TOP_ICON);
        if (icon) {
            lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_t * gear = lv_label_create(ui_button_top1);
            lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
            lv_obj_set_style_text_font(gear, &lv_font_montserrat_14, 0);
            // Remove zoom to prevent disappearance
            lv_obj_set_style_text_color(gear, lv_color_white(), 0);
            lv_obj_center(gear);
        }
        // Remove default event (ui_event_button_top1_buttontop)
        lv_obj_remove_event_cb(ui_button_top1, NULL); 
        lv_obj_add_event_cb(ui_button_top1, ui_event_settings_redirect, LV_EVENT_CLICKED, NULL);
    }

    lv_timer_handler(); 
    delay(200);
    lv_timer_handler();

    Serial.println("Starting Network Services...");
    setupWiFi(); 
    updateWeather();
    
    Serial.println("Finishing Setup...");
    encoder_init();
    Serial.println("Setup Complete.");
}

void loop()
{
    // 1. Button Logic (Reader Toggle)
    static bool last_btn_state = HIGH;
    static unsigned long last_btn_time = 0;
    bool btn_state = digitalRead(KNOB_KEY);
    
    if (btn_state != last_btn_state) {
        if (millis() - last_btn_time > 50) { // 50ms Debounce
            if (btn_state == LOW) { // Press
                if (reader.getIsActive()) {
                    reader.onButtonPress();
                }
            }
            last_btn_time = millis();
            last_btn_state = btn_state;
        }
    }

    // 2. Encoder Logic
    static int last_handled_pos = 0;
    int current_pos = encoderPos;
    
    // -> Reader Scroll Lock
    if (reader.getIsActive() && reader.getIsScrollMode()) {
        if (current_pos != last_handled_pos) { 
             int diff = (current_pos - last_handled_pos);
             // int scroll_ticks = diff / 2; // REMOVED QUANTIZATION FOR SMOOTHNESS
             reader.onScroll(diff);
             last_handled_pos = current_pos; 
        }
    } 
    // -> Navigation & Carousel
    else if (abs(current_pos - last_handled_pos) >= 4) {
        if (current_pos > last_handled_pos) {
            current_app_index++;
            if (current_app_index >= MAX_APPS) current_app_index = 0;
        } else {
            current_app_index--;
            if (current_app_index < 0) current_app_index = MAX_APPS - 1;
        }
        
        // Switch Logic
        // Cleanup active app
        pet.onAppLeave();
        reader.onAppLeave();
        
        lv_obj_t* target = ui_watch_digital; // Default
        
        switch(current_app_index) {
            case 0: target = ui_watch_digital; break;
            case 1: target = ui_watch_analog; break;
            case 2: target = ui_pet_screen; pet.onAppEnter(); break;
            case 3: target = ui_weather_1; break;
            case 4: target = ui_reader_screen; reader.onAppEnter(); break;
        }

        if (target) _ui_screen_change(&target, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
        
        last_handled_pos = current_pos;
    }

    // 3. Update Engines & Maintenance
    lv_timer_handler();
    delay(5);
    pet.update();
    reader.update();
    
    static unsigned long last_ui_update = 0;
    if (millis() - last_ui_update > 500) { 
        update_time_ui();
        last_ui_update = millis();
    }
    
    static unsigned long last_weather_update = 0;
    if (millis() - last_weather_update > 1800000) { 
        updateWeather();
        last_weather_update = millis();
    }
}
// -------------------------------------------------------------------------
// ENCODER INT
// -------------------------------------------------------------------------

// volatile int encoderPos = 0; // Moved to top
int lastEncoderPos = 0;

void IRAM_ATTR readEncoder() {
  static uint8_t old_AB = 0;
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( (digitalRead(KNOB_DATA_A) << 1) | digitalRead(KNOB_DATA_B) );
  
  // Standard quadrature decoding
  // 0b0010 (2) -> ++, 0b1011 (11) -> ++ (CW)
  // 0b0001 (1) -> --, 0b0111 (7)  -> -- (CCW)
  // Mask 0x0F to keep only 4 bits
  switch (old_AB & 0x0F) {
    case 0x02: case 0x0B: case 0x0D: case 0x04: encoderPos++; break;
    case 0x01: case 0x07: case 0x0E: case 0x08: encoderPos--; break;
  }
}

void encoder_init() {
    pinMode(KNOB_DATA_A, INPUT_PULLUP);
    pinMode(KNOB_DATA_B, INPUT_PULLUP);
    pinMode(KNOB_KEY, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(KNOB_DATA_A), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(KNOB_DATA_B), readEncoder, CHANGE);
}

// Order of screens
lv_obj_t* screens[] = {
    NULL, // ui_watch_digital
    NULL, // ui_watch_analog
    NULL, // ui_pet_screen (added)
    NULL, // ui_weather_1
    NULL, // ui_weather_2
    NULL, // ui_blood_oxy
    NULL, // ui_ecg
    NULL, // ui_blood_pressure
    NULL, // ui_measuring
    NULL  // ui_call
};

void perform_screen_switch(int dir) {
    lv_obj_t* acts = lv_scr_act();
    
    // Call onAppLeave if exiting Pet App
    if (acts == ui_pet_screen) pet.onAppLeave();

    if (acts == ui_watch_digital) {
        if (dir > 0) _ui_screen_change(&ui_watch_analog, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_watch_analog_screen_init);
        else         _ui_screen_change(&ui_weather_1,    LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_weather_1_screen_init);
    }
    else if (acts == ui_watch_analog) {
        if (dir > 0) {
            pet.onAppEnter();
            _ui_screen_change(&ui_pet_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL); // Pet is already init
        }
        else _ui_screen_change(&ui_watch_digital, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_watch_digital_screen_init);
    }
    else if (acts == ui_pet_screen) {
        if (dir > 0) _ui_screen_change(&ui_weather_1,    LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_weather_1_screen_init);
        else         _ui_screen_change(&ui_watch_analog,  LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_watch_analog_screen_init);
    }
    else if (acts == ui_weather_1) {
        if (dir > 0) _ui_screen_change(&ui_weather_2,    LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_weather_2_screen_init);
        else         _ui_screen_change(&ui_pet_screen,    LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
    }
    else if (acts == ui_weather_2) {
        if (dir > 0) _ui_screen_change(&ui_watch_digital, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_watch_digital_screen_init);
        else         _ui_screen_change(&ui_weather_1,     LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_weather_1_screen_init);
    }
    else {
         _ui_screen_change(&ui_watch_digital, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_watch_digital_screen_init);
    }
}

void encoder_handler() {
    // Debounce / Divide steps (Encoders are sensitive)
    static int last_handled_pos = 0;
    int current_pos = encoderPos;
    
    // Brightness Control Logic if in settings mode
    if (is_settings_mode && ui_volume_arc) {
        if (abs(current_pos - last_handled_pos) >= 4) {
             int step = (current_pos > last_handled_pos) ? 5 : -5;
             int current_perc = lv_arc_get_value(ui_volume_arc);
             int next_perc = constrain(current_perc + step, 0, 100);
             
             lv_arc_set_value(ui_volume_arc, next_perc);
             // Trigger the event manually to update hardware and label
             lv_event_send(ui_volume_arc, LV_EVENT_VALUE_CHANGED, NULL);
             
             last_handled_pos = current_pos;
        }
        return;
    }

    if (abs(current_pos - last_handled_pos) >= 4) { // Every 4 ticks = 1 click usually
        lv_disp_trig_activity(NULL); // Mark activity on Knob move
        if (current_pos > last_handled_pos) {
            perform_screen_switch(1); // Next
        } else {
            perform_screen_switch(-1); // Prev
        }
        last_handled_pos = current_pos;
    }
}

