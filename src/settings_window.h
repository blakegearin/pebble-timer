/*******************************************************************************
 * FILENAME :        settings_window.h
 *
 * DESCRIPTION :
 *      Create, destroy, and manage a SettingsWindow to list settings, each
 *      drawn as a name-over-value cell.
 *
 * PUBLIC FUNCTIONS :
 *      SettingsWindow  *settings_window_create(SettingsWindowCallbacks
 *                          settings_window_callbacks, const char *title,
 *                          const uint8_t *row_ids, uint8_t num_rows);
 *      void            settings_window_destroy(SettingsWindow
 *                          *settings_window);
 *      void            settings_window_push(SettingsWindow
 *                          *settings_window, bool animated);
 *      void            settings_window_refresh(SettingsWindow
 *                          *settings_window);
 *      void            settings_window_set_highlight_color(SettingsWindow
 *                          *settings_window, GColor color);
 *
 * NOTES :      One window type, three instances: the app's settings list and the
 *              List and Timer sub-menus it opens. Not compiled on aplite, where
 *              the settings are inline rows in the timer list instead -- see .c
 *              for the guard.
 *
 * AUTHOR :     Blake Gearin        START DATE :    2026-09-24
 *
 */

#pragma once

#include <pebble.h>



/*******************************************************************************
 * ROW IDS
 */

/*
 * A SettingsWindow is a list of rows, and what a row's id means is entirely the
 * caller's business: the window never interprets one, it holds the array it was
 * given and hands each row's id straight back to get_name, get_value and clicked.
 * So the id space is settings.h's: the SettingId enum, plus the group-row ids
 * `SETTINGS_ROW_GROUP` places just above `SettingCount`, which open another
 * SettingsWindow rather than an option list. main.c hands those ids over from
 * settings.c's row tables.
 */



/*******************************************************************************
 * CALLBACK DECLARATIONS
 */

/*
 * Callback:    SettingsWindowGetName
 * ----------------------------------
 * gets the display name of one row, e.g. "Sort Order"
 */

typedef const char *(*SettingsWindowGetName)(uint8_t row_id, void *context);



/*
 * Callback:    SettingsWindowGetValue
 * -----------------------------------
 * gets the label of the option a row is currently set to, or NULL when the row
 * has no value to show -- a group row is navigation, not a setting
 */

typedef const char *(*SettingsWindowGetValue)(uint8_t row_id, void *context);



/*
 * Callback:    SettingsWindowClickCallback
 * ----------------------------------------
 * called when a row is clicked
 */

typedef void (*SettingsWindowClickCallback)(uint8_t row_id, void *context);



/*
 * Structure:   SettingsWindowCallbacks
 * ------------------------------------
 * structure containing all SettingsWindow callbacks
 */

typedef struct SettingsWindowCallbacks {
  SettingsWindowGetName get_name;
  SettingsWindowGetValue get_value;
  SettingsWindowClickCallback clicked;
} SettingsWindowCallbacks;



/*******************************************************************************
 * STRUCTURE DECLARATION
 */

/*
 * Structure:   SettingsWindow
 * ---------------------------
 * main structure containing all data for a SettingsWindow
 */

typedef struct SettingsWindow SettingsWindow;



/*******************************************************************************
 * API FUNCTIONS
 */

/*
 * Function:    settings_window_create
 * -----------------------------------
 * creates a new SettingsWindow in memory but does not push it into view
 *
 *  settings_window_callbacks: callbacks for communication
 *  title: the window's own name -- drawn as a section header on rect and not
 *      at all on round, where a header has no pixels to land in. Owned by the
 *      caller and must outlive the window.
 *  row_ids: the rows to list, in order, as ids for the callbacks to read --
 *      SettingsWindow gives them no meaning of its own. Owned by the caller and
 *      must outlive the window; it is not copied.
 *  num_rows: how many row ids there are
 *
 *  returns: a pointer to a new SettingsWindow structure
 */

SettingsWindow *settings_window_create(SettingsWindowCallbacks settings_window_callbacks,
                                       const char *title, const uint8_t *row_ids,
                                       uint8_t num_rows);



/*
 * Function:    settings_window_destroy
 * ------------------------------------
 * destroys an existing SettingsWindow
 *
 *  settings_window: a pointer to the SettingsWindow being destroyed
 */

void settings_window_destroy(SettingsWindow *settings_window);



/*
 * Function:    settings_window_push
 * ---------------------------------
 * push the window onto the stack
 *
 *  settings_window: a pointer to the SettingsWindow being pushed
 *  animated: whether to animate the push or not
 */

void settings_window_push(SettingsWindow *settings_window, bool animated);



/*
 * Function:    settings_window_refresh
 * ------------------------------------
 * redraws the settings list, e.g. after a value changed on the option window
 *
 *  settings_window: a pointer to the SettingsWindow being refreshed
 */

void settings_window_refresh(SettingsWindow *settings_window);



/*
 * Function:    settings_window_set_highlight_color
 * ------------------------------------------------
 * sets the over-all color scheme of the window
 *
 *  settings_window: a pointer to the SettingsWindow to configure
 *  color: the GColor to set the highlight to
 */

void settings_window_set_highlight_color(SettingsWindow *settings_window, GColor color);
