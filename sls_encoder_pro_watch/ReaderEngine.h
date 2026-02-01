#ifndef READER_ENGINE_H
#define READER_ENGINE_H

#include "lvgl.h"
#include <Arduino.h>
#include <BleMouse.h>

extern BleMouse bleMouse;
extern BleMouse bleMouse;
extern void playTone(int freq, int duration); // Reuse buzzer
extern bool encoder_has_focus;                // Task 1: Reference Global

class ReaderEngine {
public:
  ReaderEngine()
      : isActive(false), isScrollMode(false), scrollSpeed(1), invertDir(false),
        accumulatedTicks(0), lastScrollSend(0), hasBleStarted(false),
        kickPending(false), kickTimer(0), isCursorOffset(false) {}

  void init(lv_obj_t *parent) {
    // Main Container
    readerLayer = lv_obj_create(parent);
    lv_obj_set_size(readerLayer, 390, 390);
    lv_obj_set_style_bg_color(readerLayer, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(readerLayer, 0, 0);
    lv_obj_clear_flag(readerLayer, LV_OBJ_FLAG_SCROLLABLE);

    // Header (Status)
    headerLabel = lv_label_create(readerLayer);
    lv_label_set_text(headerLabel, "Not Connected");
    lv_obj_set_style_text_font(headerLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(headerLabel, LV_ALIGN_TOP_MID, 0, 30);

    // Mode Indicator (Big Center Text)
    modeLabel = lv_label_create(readerLayer);
    lv_label_set_text(modeLabel, "NAV MODE");
    lv_obj_set_style_text_font(modeLabel, &ui_font_H1, 0); // Big font
    lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x888888), 0);
    lv_obj_align(modeLabel, LV_ALIGN_CENTER, 0, -20);

    modeSubLabel = lv_label_create(readerLayer);
    lv_label_set_text(modeSubLabel, "Tap Knob to Lock");
    lv_obj_set_style_text_font(modeSubLabel, &ui_font_Subtitle, 0);
    lv_obj_align_to(modeSubLabel, modeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Settings Container (Bottom)
    lv_obj_t *settingsCont = lv_obj_create(readerLayer);
    lv_obj_set_size(settingsCont, 300, 80);
    lv_obj_align(settingsCont, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_opa(settingsCont, 0, 0);
    lv_obj_set_style_border_width(settingsCont, 0, 0);
    lv_obj_clear_flag(settingsCont, LV_OBJ_FLAG_SCROLLABLE);

    // Speed Button
    speedBtn = lv_btn_create(settingsCont);
    lv_obj_set_size(speedBtn, 80, 50);
    lv_obj_align(speedBtn, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_add_event_cb(speedBtn, speed_event_wrapper, LV_EVENT_CLICKED, this);
    speedLabel = lv_label_create(speedBtn);
    lv_label_set_text(speedLabel, "1x");
    lv_obj_center(speedLabel);

    // Invert Button
    invertBtn = lv_btn_create(settingsCont);
    lv_obj_set_size(invertBtn, 80, 50);
    lv_obj_align(invertBtn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(invertBtn, invert_event_wrapper, LV_EVENT_CLICKED,
                        this);
    lv_obj_t *l2 = lv_label_create(invertBtn);
    lv_label_set_text(l2, "Inv");
    lv_obj_center(l2);
  }

  void onAppEnter() {
    isActive = true;
    isScrollMode = false;
    lastActivityTime = millis(); // Reset timer on entry
    updateVisuals();
  }

  void onAppLeave() {
    if (isScrollMode) {
      encoder_has_focus = false; // Release safety
    }
    isActive = false;
    isScrollMode = false; // Always revert to Nav when leaving
  }

  void update() {
    if (!isActive)
      return;

    uint32_t currentMillis = millis();

    // 0. Delayed Cursor Kick Logic (Task Fix)
    if (kickPending && currentMillis > kickTimer) {
      if (bleMouse.isConnected() && isScrollMode) {
        performCursorKick();
      }
      kickPending = false;
    }

    // 1. Smooth Scroll Logic (15ms Interval)

    if (currentMillis - lastScrollSend > 15) {
      if (accumulatedTicks != 0 && bleMouse.isConnected() && isScrollMode) {
        int velocity = abs(accumulatedTicks);
        int base_amount = 0;

        // Gentle Ramp Formula per Task 1
        int multiplier = 3 + (velocity * 2);
        if (multiplier > 10)
          multiplier = 10; // Cap at 10

        base_amount = accumulatedTicks * multiplier;

        // Apply User Speed Setting (Scalar)
        // Note: User prompt implies the curve is the main logic, but let's keep
        // the multiplier functional If scrollSpeed is 1 (Low), we use the base.
        // If 3 (Med), we triple it.
        int final_amount = base_amount * scrollSpeed;

        // Apply Direction Invert (Logic from previous steps: Default inverted,
        // User Invert toggles back) Default: -delta. User Invert: +delta.
        final_amount = -final_amount;
        if (invertDir)
          final_amount = -final_amount;

        // Task 4: Scroll Booster (Min +/- 2)
        if (final_amount != 0) {
          if (abs(final_amount) < 2) {
            final_amount = (final_amount > 0) ? 2 : -2;
          }
          bleMouse.move(0, 0, final_amount);
        }

        accumulatedTicks = 0;
      }
      lastScrollSend = currentMillis;
    }

    // Battery Saver / Inactivity Timeout (5 Minutes)
    if (bleMouse.isConnected() && isScrollMode) {
      if (currentMillis - lastActivityTime > 300000) { // 5 minutes
        isScrollMode = false;                          // Auto-unlock
        encoder_has_focus = false;                     // Release Focus
        resetCursorPosition(); // Reset cursor on timeout
        playTone(200, 500);    // Long low buzz to indicate timeout
        updateVisuals();
      }
    }

    // Connection Monitor (Polled)
    static uint32_t lastCheck = 0;
    if (currentMillis - lastCheck > 500) { // Check every 500ms
      bool connected = bleMouse.isConnected();
      static bool lastConnected = false;

      // Update Header
      if (connected) {
        lv_label_set_text(headerLabel, LV_SYMBOL_BLUETOOTH " Connected");
        lv_obj_set_style_text_color(headerLabel, lv_color_hex(0x00FF00),
                                    0); // Green
      } else {
        lv_label_set_text(headerLabel, "Waiting for Host...");
        lv_obj_set_style_text_color(headerLabel, lv_color_hex(0xFF5555),
                                    0); // Redish
      }

      // Detect State Change to force UI refresh (Fixes "Connecting..." stuck
      // issue)
      if (connected != lastConnected) {
        lastConnected = connected;
        updateVisuals();

        // Fix: Trigger Delayed Cursor Kick
        if (connected && isScrollMode) {
          kickPending = true;
          kickTimer = currentMillis + 1000; // 1000ms delay for host readiness
        }
      }

      lastCheck = currentMillis;
    }
  }

  // Old onButtonPress removed

  void onScroll(int delta) {
    lastActivityTime = millis();
    if (!isScrollMode || !bleMouse.isConnected())
      return;

    // Momentum Guard: Ignore reverse blips if moving fast
    if (abs(accumulatedTicks) > 2) {
      if ((accumulatedTicks > 0 && delta < 0) ||
          (accumulatedTicks < 0 && delta > 0)) {
        return; // Ignore noise
      }
    }

    // Accumulate ticks (Raw direction)
    accumulatedTicks += delta;
  }

  bool getIsActive() { return isActive; }
  bool getIsScrollMode() { return isScrollMode; }

private:
  bool isActive;
  bool isScrollMode;
  int scrollSpeed; // 1, 3, 5
  bool invertDir;
  uint32_t lastActivityTime;

  // Smooth Scroll Variables
  int accumulatedTicks;
  uint32_t lastScrollSend;
  bool hasBleStarted; // Task 2

  // Delayed Kick
  bool kickPending;
  uint32_t kickTimer;
  bool isCursorOffset;

  lv_obj_t *readerLayer;
  lv_obj_t *headerLabel;
  lv_obj_t *modeLabel;
  lv_obj_t *modeSubLabel;
  lv_obj_t *speedBtn;
  lv_obj_t *speedLabel;
  lv_obj_t *invertBtn;

  void updateVisuals() {
    if (isScrollMode) {
      if (bleMouse.isConnected()) {
        lv_label_set_text(modeLabel, "SCROLL");
        lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x00FFFF),
                                    0); // Cyan
        lv_label_set_text(modeSubLabel, "Locked: Scroll Active");
      } else {
        // Task 4: Visuals for Delay
        lv_label_set_text(modeLabel, "Connecting...");
        lv_obj_set_style_text_color(modeLabel, lv_color_hex(0xFFA500),
                                    0); // Orange
        lv_label_set_text(modeSubLabel, "Please Wait...");
      }

      // Dim navigation hints?
      lv_obj_set_style_opa(speedBtn, LV_OPA_COVER, 0);
      lv_obj_set_style_opa(invertBtn, LV_OPA_COVER, 0);
    } else {
      lv_label_set_text(modeLabel, "NAV MODE");
      lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x888888), 0); // Grey
      lv_label_set_text(modeSubLabel, "Tap Knob to Lock");
      // Fade out settings to imply they are for the active mode?
      // Actually keep them visible so user can config before locking
      lv_obj_set_style_opa(speedBtn, LV_OPA_80, 0);
      lv_obj_set_style_opa(invertBtn, LV_OPA_80, 0);
    }
  }

public:
  void startBluetooth() {
    if (hasBleStarted)
      return;
    bleMouse.begin();
    hasBleStarted = true;
  }

  // Input Handling (Updated)
  void onButtonPress() {
    lastActivityTime = millis();

    // Task 3: Trigger Start
    startBluetooth();

    // Toggle Lock
    isScrollMode = !isScrollMode;

    // UX Feedback
    if (isScrollMode) {
      encoder_has_focus = true; // Task 1: Lock Focus

      // Buzz: High-High for active
      playTone(1000, 50);
      delay(50);
      playTone(1200, 50);

      // CURSOR KICK (Only if connected, otherwise we wait for delayed kick)
      if (bleMouse.isConnected()) {
        performCursorKick();
      }
    } else {
      encoder_has_focus = false; // Task 1: Unlock Focus

      playTone(500, 100);
      // Return cursor to prevent drift
      resetCursorPosition();
    }
    updateVisuals();
  }

  void performCursorKick() {
    if (isCursorOffset)
      return; // Prevent drift by only kicking once per session

    // Strong Kick (Task 2: Deeper move)
    bleMouse.move(100, 100, 0);
    delay(50);
    bleMouse.move(100, 100, 0); // Total 200, 200
    delay(50);

    // Scroll Shake (Task 2: Vigorously wake)
    bleMouse.move(0, 0, -2);
    delay(50);
    bleMouse.move(0, 0, 2);

    isCursorOffset = true;
  }

  void resetCursorPosition() {
    if (!isCursorOffset || !bleMouse.isConnected()) {
      isCursorOffset = false; // Safety reset
      return;
    }

    // Return to origin (Fixes drift)
    bleMouse.move(-100, -100, 0);
    delay(50);
    bleMouse.move(-100, -100, 0);

    isCursorOffset = false;
  }

  void toggleSpeed() {
    lastActivityTime = millis();
    if (scrollSpeed == 1)
      scrollSpeed = 3;
    else if (scrollSpeed == 3)
      scrollSpeed = 5;
    else
      scrollSpeed = 1;

    lv_label_set_text_fmt(speedLabel, "%dx", scrollSpeed);
  }

  void toggleInvert() {
    lastActivityTime = millis();
    invertDir = !invertDir;
    lv_obj_set_style_bg_color(
        invertBtn, invertDir ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444),
        0);
  }

  // Static Wrappers
  static void speed_event_wrapper(lv_event_t *e) {
    ReaderEngine *re = (ReaderEngine *)lv_event_get_user_data(e);
    re->toggleSpeed();
  }
  static void invert_event_wrapper(lv_event_t *e) {
    ReaderEngine *re = (ReaderEngine *)lv_event_get_user_data(e);
    re->toggleInvert();
  }
};

#endif
