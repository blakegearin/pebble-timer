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
 * AUTHOR :     Blake Gearin        START DATE :    2026-09-24
 *
 */

#pragma once

/*
 * Structure:   SettingId
 * ----------------------
 * one of the app's settings. The shipped default is wherever a zero-initialised
 * static lands, and it is the static that carries the default -- not option index
 * 0. For an On/Off setting the options are listed `Off, On` so the pair reads the
 * same everywhere, which puts a default of On at index 1; main.c's accessors
 * translate the bool to the right index. Every other option list still leads with
 * its default, because there index 0 and the false static agree.
 *
 * SettingColor is a PBL_COLOR member, not a platform one: aplite and diorite
 * are both black and white, so a colour they cannot render must not exist in
 * their copy of this enum. Every renderer sizes itself off SettingCount,
 * which drops the row on both automatically.
 *
 * The rest come in two groups, and the prefix on each name says which: the
 * `SettingList` three change how the timer list behaves, the `SettingTimer` three
 * how a timer itself behaves. The prefixes exist because a bare `SettingGroup` or
 * `SettingDelete` begs the question of what is being grouped or deleted, and
 * because on every platform but aplite each group is a sub-menu of its own --
 * naming them is what lets the code say which one it means. They stay adjacent in
 * this enum because that order is also the order aplite renders its inline rows
 * in. Which group has a home of its own where is main.c's business, not this
 * enum's.
 */

typedef enum {
  SettingListSortOrder = 0,
  SettingListGroup,
  SettingListWrapAround,
  SettingTimerStartMode,
  SettingTimerDeleteConfirm,
  SettingTimerSnoozeLength,
#ifdef PBL_COLOR
  SettingColor,
#endif
  SettingCount,
} SettingId;
