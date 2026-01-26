#ifndef PET_ENGINE_H
#define PET_ENGINE_H

#include "lvgl.h"
#include <Arduino.h>

enum Mood { NEUTRAL, HAPPY, ANGRY, SLEEPY };

class PetEngine {
public:
    PetEngine() : currentMood(NEUTRAL), isActive(false) {}

    void init(lv_obj_t* parent) {
        petLayer = lv_obj_create(parent);
        lv_obj_set_size(petLayer, 390, 390);
        lv_obj_set_style_bg_color(petLayer, lv_color_black(), 0);
        lv_obj_set_style_border_width(petLayer, 0, 0);
        lv_obj_set_style_radius(petLayer, 0, 0);
        lv_obj_clear_flag(petLayer, LV_OBJ_FLAG_SCROLLABLE);

        // Container for eyes to move them together (Gaze)
        eyesContainer = lv_obj_create(petLayer);
        lv_obj_set_size(eyesContainer, 300, 200);
        lv_obj_set_style_bg_opa(eyesContainer, 0, 0); // Transparent
        lv_obj_set_style_border_width(eyesContainer, 0, 0);
        lv_obj_center(eyesContainer);
        lv_obj_clear_flag(eyesContainer, LV_OBJ_FLAG_SCROLLABLE);

        // Left Eye
        eyeLeft = lv_obj_create(eyesContainer);
        lv_obj_set_size(eyeLeft, 80, 120); // Big Pill Shape
        lv_obj_set_style_bg_color(eyeLeft, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_radius(eyeLeft, 40, 0); // Pill radius
        lv_obj_set_style_border_width(eyeLeft, 0, 0);
        lv_obj_align(eyeLeft, LV_ALIGN_CENTER, -60, 0);

        // Right Eye
        eyeRight = lv_obj_create(eyesContainer);
        lv_obj_set_size(eyeRight, 80, 120);
        lv_obj_set_style_bg_color(eyeRight, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_radius(eyeRight, 40, 0);
        lv_obj_set_style_border_width(eyeRight, 0, 0);
        lv_obj_align(eyeRight, LV_ALIGN_CENTER, 60, 0);

        // Init Anim vars
        blinkState = 0; // Open
        gazeX = 0;
        gazeY = 0;
    }

    void onAppEnter() {
        isActive = true;
        setMood(NEUTRAL);
        resetEyes();
    }

    void onAppLeave() {
        isActive = false;
        lv_anim_del_all(); // Stop all animations
    }

    void setMood(Mood m) {
        currentMood = m;
        lastMoodChange = millis();
        
        // Mood specific setup
        switch(m) {
            case HAPPY:
                startJumpAnim();
                lv_obj_set_style_bg_color(eyeLeft, lv_color_hex(0x00FF00), 0);
                lv_obj_set_style_bg_color(eyeRight, lv_color_hex(0x00FF00), 0);
                break;
            case SLEEPY:
                closeEyesAnim();
                lv_obj_set_style_bg_color(eyeLeft, lv_color_hex(0x5555FF), 0); // Dim Blue
                lv_obj_set_style_bg_color(eyeRight, lv_color_hex(0x5555FF), 0);
                break;
            case ANGRY:
                lv_obj_set_style_bg_color(eyeLeft, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_bg_color(eyeRight, lv_color_hex(0xFF0000), 0);
                break;
            case NEUTRAL:
            default:
                lv_obj_set_style_bg_color(eyeLeft, lv_color_hex(0x00FFFF), 0); // Cyan
                lv_obj_set_style_bg_color(eyeRight, lv_color_hex(0x00FFFF), 0);
                // Ensure eyes are open if returning from sleep
                if(lv_obj_get_height(eyeLeft) < 10) openEyesAnim();
                break;
        }
    }
    
    Mood getMood() { return currentMood; }

    void update() {
        if (!isActive) return;
        uint32_t now = millis();

        // 1. Auto-Revert Mood
        if ((currentMood == HAPPY || currentMood == ANGRY) && (now - lastMoodChange > 2500)) {
            setMood(NEUTRAL);
        }

        // 2. Random Blink (Only if not sleeping)
        if (currentMood != SLEEPY) {
            if (now - lastBlink > nextBlinkInterval) {
                startBlinkAnim();
                lastBlink = now;
                nextBlinkInterval = random(2000, 6000);
            }
        }

        // 3. Random Gaze Saccades (Only if Neutral or Happy)
        if (currentMood == NEUTRAL || currentMood == HAPPY) {
            if (now - lastSaccade > nextSaccadeInterval) {
                doSaccade();
                lastSaccade = now;
                nextSaccadeInterval = random(1000, 4000);
            }
        }
    }

    // --- Animation Wrappers ---
    // Helpers required because LVGL callbacks are C-style

    static void set_eye_height(void* var, int32_t v) {
        lv_obj_set_height((lv_obj_t*)var, v);
    }
    
    static void set_container_y(void* var, int32_t v) {
        lv_obj_set_y((lv_obj_t*)var, v);
    }

    static void set_container_x(void* var, int32_t v) {
        lv_obj_set_x((lv_obj_t*)var, v);
    }

private:
    Mood currentMood;
    bool isActive;
    
    lv_obj_t* petLayer;
    lv_obj_t* eyesContainer;
    lv_obj_t* eyeLeft;
    lv_obj_t* eyeRight;

    uint32_t lastMoodChange;
    uint32_t lastBlink = 0;
    uint32_t nextBlinkInterval = 3000;
    uint32_t lastSaccade = 0;
    uint32_t nextSaccadeInterval = 2000;
    
    int blinkState; 
    int gazeX, gazeY;

    void resetEyes() {
        lv_obj_set_size(eyeLeft, 80, 120);
        lv_obj_set_size(eyeRight, 80, 120);
        lv_obj_center(eyesContainer);
    }

    void startBlinkAnim() {
        // Left Eye
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, eyeLeft);
        lv_anim_set_values(&a, 120, 5); // Height: Open -> Closed
        lv_anim_set_time(&a, 150);
        lv_anim_set_playback_time(&a, 150); // Auto Reopen
        lv_anim_set_exec_cb(&a, set_eye_height);
        lv_anim_start(&a);

        // Right Eye (Clone config)
        lv_anim_set_var(&a, eyeRight);
        lv_anim_start(&a);
    }

    void closeEyesAnim() {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, eyeLeft);
        lv_anim_set_values(&a, lv_obj_get_height(eyeLeft), 5);
        lv_anim_set_time(&a, 300);
        lv_anim_set_exec_cb(&a, set_eye_height);
        lv_anim_start(&a);
        
        lv_anim_set_var(&a, eyeRight);
        lv_anim_set_values(&a, lv_obj_get_height(eyeRight), 5);
        lv_anim_start(&a);
    }

    void openEyesAnim() {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, eyeLeft);
        lv_anim_set_values(&a, lv_obj_get_height(eyeLeft), 120);
        lv_anim_set_time(&a, 300);
        lv_anim_set_exec_cb(&a, set_eye_height);
        lv_anim_start(&a);

        lv_anim_set_var(&a, eyeRight);
        lv_anim_set_values(&a, lv_obj_get_height(eyeRight), 120);
        lv_anim_start(&a);
    }

    void startJumpAnim() {
        // Jump Container Up/Down
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, eyesContainer);
        lv_anim_set_values(&a, 0, -30); // Move Up
        lv_anim_set_time(&a, 200);
        lv_anim_set_playback_time(&a, 300); // Bounce Back
        lv_anim_set_repeat_count(&a, 2);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&a, set_container_y);
        lv_anim_start(&a);
    }

    void doSaccade() {
        int newX = random(-30, 31);
        int newY = random(-20, 21);
        
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, eyesContainer);
        lv_anim_set_exec_cb(&a, set_container_x);
        lv_anim_set_values(&a, lv_obj_get_x_aligned(eyesContainer), newX);
        lv_anim_set_time(&a, 300);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);

        lv_anim_set_exec_cb(&a, set_container_y);
        lv_anim_set_values(&a, lv_obj_get_y_aligned(eyesContainer), newY);
        lv_anim_start(&a);
    }
};

#endif
