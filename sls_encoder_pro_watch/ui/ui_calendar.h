#ifndef UI_CALENDAR_H
#define UI_CALENDAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_calendar_screen;
extern lv_obj_t *ui_calendar;
extern lv_obj_t *ui_CalendarTitle;
extern lv_obj_t *ui_cal_arrow_left;
extern lv_obj_t *ui_cal_arrow_right;

void ui_calendar_screen_init(void);
void ui_calendar_update_today(int year, int month, int day);
void updateCalendarTitle(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
