/*******************************************************************************
 * FILENAME :        settings.h
 *
 * DESCRIPTION :
 *      The app's settings, named so both windows and menu_window can refer
 *      to one in a callback signature without depending on main.c. The copy
 *      and the values live in main.c and reach the windows through callbacks,
 *      matching how every other window in this app is fed.
 *
 * NOTES :      Header only -- there is no settings.c.
 *
 * AUTHOR :     Blake Gearin        START DATE :    24/09/26
 *
 */

#pragma once

/*
 * Structure:   SettingId
 * ----------------------
 * one of the app's settings. option index 0 is always the shipped default,
 * which is also what a zero-initialised static gives an upgrading user.
 */

typedef enum {
  SettingSortOrder = 0,
  SettingStartTimers,
  SettingDelete,
  SettingCount,
} SettingId;
