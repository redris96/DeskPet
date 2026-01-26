/**
 * @file lv_conf.h
 * Configuration file for v8.3.11
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simple: to `lvgl` library
 * 2. or to `lv_conf_demo.h`
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH 16

/*Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP 1

/*1: Enable complex draw engine (much faster, uses more RAM)*/
#define LV_DRAW_COMPLEX 1

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*1: Use custom malloc/free, 0: Use the built-in `lv_mem_alloc` / `lv_mem_free`*/
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>               /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/*=========================
   HAL SETTINGS
 *=========================*/

/*1: Use a custom tick source that requires the user to call `lv_tick_inc()`*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current system time in ms*/
#endif

/*=========================
   FONT USAGE
 *=========================*/

#define LV_FONT_MONTSERRAT_14 1

#endif /*LV_CONF_H*/
