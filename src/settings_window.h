/*******************************************************************************
 * FILENAME :        settings_window.h
 *
 * DESCRIPTION :
 *      Create, destroy, and manage a SettingsWindow to list the app's
 *      settings, each drawn as a name-over-value cell.
 *
 * PUBLIC FUNCTIONS :
 *      SettingsWindow  *settings_window_create(SettingsWindowCallbacks
 *                          settings_window_callbacks);
 *      void            settings_window_destroy(SettingsWindow
 *                          *settings_window);
 *      void            settings_window_push(SettingsWindow
 *                          *settings_window, bool animated);
 *      void            settings_window_refresh(SettingsWindow
 *                          *settings_window);
 *      void            settings_window_set_highlight_color(SettingsWindow
 *                          *settings_window, GColor color);
 *
 * NOTES :      Not compiled on aplite, where the settings are inline rows in
 *              the timer list instead -- see .c for the guard.
 *
 * AUTHOR :     Blake Gearin        START DATE :    24/09/26
 *
 */

#pragma once

#include <pebble.h>



/*******************************************************************************
 * CALLBACK DECLARATIONS
 */

/*
 * Callback:    SettingsWindowGetName
 * ----------------------------------
 * gets the display name of one setting, e.g. "Sort Order"
 */

typedef const char *(*SettingsWindowGetName)(uint8_t setting, void *context);



/*
 * Callback:    SettingsWindowGetValue
 * -----------------------------------
 * gets the label of the option a setting is currently set to
 */

typedef const char *(*SettingsWindowGetValue)(uint8_t setting, void *context);



/*
 * Callback:    SettingsWindowClickCallback
 * ----------------------------------------
 * called when a setting row is clicked
 */

typedef void (*SettingsWindowClickCallback)(uint8_t setting, void *context);



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
 *
 *  returns: a pointer to a new SettingsWindow structure
 */

SettingsWindow *settings_window_create(SettingsWindowCallbacks settings_window_callbacks);



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
