#include "ui_calendar.h"
#include "ui.h"

// SCREEN: ui_calendar_screen
lv_obj_t *ui_calendar_screen;
lv_obj_t *ui_calendar;
lv_obj_t *ui_CalendarTitle;
lv_obj_t *ui_cal_arrow_left;
lv_obj_t *ui_cal_arrow_right;

// Persistent Styles to override theme defaults
static lv_style_t style_calendar_main;
static lv_style_t style_calendar_items;
static lv_style_t style_calendar_today;

// Helper to update the dynamic title
void updateCalendarTitle() {
  if (!ui_calendar || !ui_CalendarTitle)
    return;
  const lv_calendar_date_t *d = lv_calendar_get_showed_date(ui_calendar);

  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  if (d->month >= 1 && d->month <= 12) {
    lv_label_set_text_fmt(ui_CalendarTitle, "%s %d", months[d->month - 1],
                          d->year);
  }
}

static void calendar_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_calendar_date_t date;
    if (lv_calendar_get_pressed_date(lv_event_get_target(e), &date) ==
        LV_RES_OK) {
      // Unused currently
    }
  }
}

static void arrow_left_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    const lv_calendar_date_t *d = lv_calendar_get_showed_date(ui_calendar);
    lv_calendar_date_t new_date = *d;
    new_date.month--;
    if (new_date.month < 1) {
      new_date.month = 12;
      new_date.year--;
    }
    lv_calendar_set_showed_date(ui_calendar, new_date.year, new_date.month);
    updateCalendarTitle();
  }
}

static void arrow_right_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    const lv_calendar_date_t *d = lv_calendar_get_showed_date(ui_calendar);
    lv_calendar_date_t new_date = *d;
    new_date.month++;
    if (new_date.month > 12) {
      new_date.month = 1;
      new_date.year++;
    }
    lv_calendar_set_showed_date(ui_calendar, new_date.year, new_date.month);
    updateCalendarTitle();
  }
}

void ui_calendar_screen_init(void) {
  ui_calendar_screen = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_calendar_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_calendar_screen, lv_color_hex(0x000000),
                            LV_PART_MAIN);

  // --- REWRITE: Style Hierarchy ---

  // 1. Main Widget Style (The Void)
  lv_style_init(&style_calendar_main);
  lv_style_set_bg_opa(&style_calendar_main, 0);
  lv_style_set_border_width(&style_calendar_main, 0);
  lv_style_set_outline_width(&style_calendar_main, 0);
  lv_style_set_radius(&style_calendar_main, 0);
  lv_style_set_pad_all(&style_calendar_main, 0);

  // 2. Item Style (Floating Text)
  lv_style_init(&style_calendar_items);
  lv_style_set_bg_opa(&style_calendar_items, 0);
  lv_style_set_border_width(&style_calendar_items, 0);
  lv_style_set_text_color(&style_calendar_items, lv_color_white());
  lv_style_set_text_font(&style_calendar_items, &lv_font_montserrat_14);
  lv_style_set_pad_all(&style_calendar_items,
                       5); // Some breathing room for the circle

  // 3. Today Style (Solid Red Circle)
  lv_style_init(&style_calendar_today);
  lv_style_set_bg_color(&style_calendar_today, lv_color_hex(0xFF0000));
  lv_style_set_bg_opa(&style_calendar_today, LV_OPA_COVER);
  lv_style_set_radius(&style_calendar_today, LV_RADIUS_CIRCLE);
  lv_style_set_text_color(&style_calendar_today, lv_color_white());

  // --- Implementation ---

  ui_calendar = lv_calendar_create(ui_calendar_screen);
  lv_obj_set_size(ui_calendar, lv_pct(90), lv_pct(70));
  lv_obj_align(ui_calendar, LV_ALIGN_CENTER, 0, 10);

  // Apply Styles to Main Part
  lv_obj_add_style(ui_calendar, &style_calendar_main, LV_PART_MAIN);

  // Deep Dive: Apply Styles to Underlying BtnMatrix
  lv_obj_t *calendar_btnm = lv_calendar_get_btnmatrix(ui_calendar);
  lv_obj_add_style(calendar_btnm, &style_calendar_items, LV_PART_ITEMS);

  // Ensure 'Today' uses the red circle.
  // In LVGL 8, the today cell has LV_CALENDAR_CTRL_TODAY.
  // The calendar widget maps this to LV_STATE_CHECKED on the specific button.
  lv_obj_add_style(calendar_btnm, &style_calendar_today,
                   LV_PART_ITEMS | LV_STATE_CHECKED);

  // Grey Day Names & Hide Ghost Dates
  // Ghost dates are 'disabled' in the calendar's btnmatrix logic.
  lv_obj_set_style_text_color(calendar_btnm, lv_color_hex(0x606060),
                              LV_PART_ITEMS | LV_STATE_DISABLED);
  lv_obj_set_style_text_opa(calendar_btnm, 0,
                            LV_PART_ITEMS | LV_STATE_DISABLED);
  // Wait, day names are also often disabled or in a different part?
  // Actually day names are usually the first row.
  // To be precise, let's use a drawing callback if this fails, but try simple
  // first: Some themes style disabled items with a specific color.

  lv_obj_add_event_cb(ui_calendar, calendar_event_handler, LV_EVENT_ALL, NULL);

  // 4. Custom Title
  ui_CalendarTitle = lv_label_create(ui_calendar_screen);
  lv_label_set_text(ui_CalendarTitle, "");
  lv_obj_set_style_text_font(ui_CalendarTitle, &ui_font_Title, 0);
  lv_obj_set_style_text_color(ui_CalendarTitle, lv_color_white(), 0);
  lv_obj_align(ui_CalendarTitle, LV_ALIGN_TOP_MID, 0, 20);

  // 5. Custom Button Layer (Bottom)
  ui_cal_arrow_left = lv_btn_create(ui_calendar_screen);
  lv_obj_set_size(ui_cal_arrow_left, 60, 60);
  lv_obj_align(ui_cal_arrow_left, LV_ALIGN_BOTTOM_MID, -45, -20);
  lv_obj_set_style_bg_opa(ui_cal_arrow_left, 0, 0);
  lv_obj_set_style_shadow_width(ui_cal_arrow_left, 0, 0);

  lv_obj_t *label_left = lv_label_create(ui_cal_arrow_left);
  lv_label_set_text(label_left, LV_SYMBOL_LEFT);
  lv_obj_center(label_left);
  lv_obj_set_style_text_color(label_left, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_left, &lv_font_montserrat_24, 0);
  lv_obj_add_event_cb(ui_cal_arrow_left, arrow_left_event_cb, LV_EVENT_CLICKED,
                      NULL);

  ui_cal_arrow_right = lv_btn_create(ui_calendar_screen);
  lv_obj_set_size(ui_cal_arrow_right, 60, 60);
  lv_obj_align(ui_cal_arrow_right, LV_ALIGN_BOTTOM_MID, 45, -20);
  lv_obj_set_style_bg_opa(ui_cal_arrow_right, 0, 0);
  lv_obj_set_style_shadow_width(ui_cal_arrow_right, 0, 0);

  lv_obj_t *label_right = lv_label_create(ui_cal_arrow_right);
  lv_label_set_text(label_right, LV_SYMBOL_RIGHT);
  lv_obj_center(label_right);
  lv_obj_set_style_text_color(label_right, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_right, &lv_font_montserrat_24, 0);
  lv_obj_add_event_cb(ui_cal_arrow_right, arrow_right_event_cb,
                      LV_EVENT_CLICKED, NULL);

  updateCalendarTitle();
}

void ui_calendar_update_today(int year, int month, int day) {
  if (ui_calendar) {
    lv_calendar_set_today_date(ui_calendar, year, month, day);
  }
}
