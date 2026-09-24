/*******************************************************************************
 * FILENAME :        settings_window.c
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
 * AUTHOR :     Blake Gearin        START DATE :    24/09/26
 *
 */

#include <pebble.h>
#include "settings_window.h"
#include "settings.h"

/*
 * The whole window is excluded on aplite rather than deleted from the build:
 * wscript globs every C file under src/, so in-file exclusion is the only
 * route. The cost that matters on aplite is compiled code -- this file's .text
 * would eat exactly the heap the detail window needs -- and uncompiled code is
 * free.
 */
#ifndef PBL_PLATFORM_APLITE

// Round menu cell heights are unconditional firmware constants (issue 01):
// a focused cell holds name over value, an unfocused one only the name.
#ifdef PBL_ROUND
#define SETTINGS_CELL_HEIGHT_FOCUSED 68
#define SETTINGS_CELL_HEIGHT 32
#endif

/*******************************************************************************
 * STRUCTURE DEFINITION
 */

/*
 * the structure of a SettingsWindow
 */

struct SettingsWindow {
  Window      *window;    //< main window
  MenuLayer   *menu;      //< menu layer displaying the settings
  StatusBarLayer *status; //< status bar
  SettingsWindowCallbacks callbacks; //< settings list callbacks
  GColor      highlight_color;       //< main color for highlights
};



/*******************************************************************************
 * PRIVATE FUNCTIONS
 */

/*
 * get number of sections for menu layer
 * this is always one for this application
 */

static uint16_t settings_get_num_sections_callback(MenuLayer *menu_layer, void *context) {
  return 1;
}



/*
 * get number of rows for menu layer, one per setting
 */

static uint16_t settings_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index,
                                               void *context) {
  return SettingCount;
}



/*
 * the window title is a section header on rect and nothing on round
 *
 * status_bar_layer_set_title is not in the app SDK, so the firmware's own
 * title mechanism has no app equivalent. On round a header lands flush in the
 * top-left corner where the circular mask has no pixels; and round shows one
 * focused setting at a time anyway, so the title would answer a question
 * nobody is asking.
 */

static int16_t settings_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index,
                                                   void *context) {
  return PBL_IF_RECT_ELSE(MENU_CELL_BASIC_HEADER_HEIGHT, 0);
}

static void settings_draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                          uint16_t section_index, void *context) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Settings");
}



/*
 * draw each row: bold setting name over its current value
 *
 * menu_cell_basic_draw resolves the fonts from the theme, which is why there
 * is no font handling here -- hardcoding one would break emery and gabbro.
 */

static void settings_draw_row_callback(GContext *ctx, const Layer *cell_layer,
                                       MenuIndex *cell_index, void *context) {
  SettingsWindow *settings_window = (SettingsWindow*)context;
  const uint8_t setting = (uint8_t)cell_index->row;
  menu_cell_basic_draw(ctx, cell_layer,
    settings_window->callbacks.get_name(setting, context),
    settings_window->callbacks.get_value(setting, context), NULL);
}



/*
 * menu layer clicked callback
 */

static void settings_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                     void *context) {
  SettingsWindow *settings_window = (SettingsWindow*)context;
  settings_window->callbacks.clicked((uint8_t)cell_index->row, context);
}



#ifdef PBL_ROUND
// focused cells are taller so they can hold the value line
static int16_t settings_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                                 void *context) {
  if (menu_layer_get_selected_index(menu_layer).row == cell_index->row) {
    return SETTINGS_CELL_HEIGHT_FOCUSED;
  }
  return SETTINGS_CELL_HEIGHT;
}
#endif



/*
 * window load
 */

static void settings_window_load(Window *window) {
  SettingsWindow *settings_window = window_get_user_data(window);
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_frame(root);

#ifdef PBL_ROUND
  settings_window->menu = menu_layer_create(bounds);
#else
  settings_window->menu = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w,
                                                  bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
#endif
  MenuLayerCallbacks callbacks = {
    .get_num_sections = settings_get_num_sections_callback,
    .get_num_rows = settings_get_num_rows_callback,
    .get_header_height = settings_get_header_height_callback,
    .draw_header = settings_draw_header_callback,
    .draw_row = settings_draw_row_callback,
    .select_click = settings_select_callback,
#ifdef PBL_ROUND
    .get_cell_height = settings_get_cell_height_callback,
#endif
  };
  // no get_cell_height on rect: the MenuLayer default is the system metric on
  // every platform, and MENU_CELL_BASIC_HEIGHT does not exist to name it
  menu_layer_set_callbacks(settings_window->menu, settings_window, callbacks);
  menu_layer_set_click_config_onto_window(settings_window->menu, window);
  menu_layer_set_highlight_colors(settings_window->menu, settings_window->highlight_color,
                                  gcolor_legible_over(settings_window->highlight_color));
  layer_add_child(root, menu_layer_get_layer(settings_window->menu));

  settings_window->status = status_bar_layer_create();
  status_bar_layer_set_colors(settings_window->status, GColorClear, GColorBlack);
  layer_add_child(root, status_bar_layer_get_layer(settings_window->status));
}

/*
 * window unload
 */

static void settings_window_unload(Window *window) {
  SettingsWindow *settings_window = window_get_user_data(window);
  status_bar_layer_destroy(settings_window->status);
  settings_window->status = NULL;
  menu_layer_destroy(settings_window->menu);
  settings_window->menu = NULL;
}



/*******************************************************************************
 * API FUNCTIONS
 */

/*
 * create a new SettingsWindow and return a pointer to it
 * the window itself is created now, its layers only while it is on screen
 */

SettingsWindow *settings_window_create(SettingsWindowCallbacks settings_window_callbacks) {
  SettingsWindow *settings_window = (SettingsWindow*)malloc(sizeof(SettingsWindow));
  if (settings_window == NULL) {
    // error handling
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create SettingsWindow");
    return NULL;
  }
  settings_window->callbacks = settings_window_callbacks;
  settings_window->menu = NULL;
  settings_window->status = NULL;
  settings_window->highlight_color = GColorBlack;
  settings_window->window = window_create();
  window_set_user_data(settings_window->window, settings_window);
  window_set_window_handlers(settings_window->window, (WindowHandlers) {
    .load = settings_window_load,
    .unload = settings_window_unload,
  });
  return settings_window;
}



/*
 * destroy a previously created SettingsWindow
 */

void settings_window_destroy(SettingsWindow *settings_window) {
  if (settings_window != NULL) {
    window_destroy(settings_window->window);
    free(settings_window);
    return;
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to free NULL SettingsWindow");
}



/*
 * push the window onto the stack
 */

void settings_window_push(SettingsWindow *settings_window, bool animated) {
  window_stack_push(settings_window->window, animated);
}



/*
 * refresh the provided SettingsWindow
 */

void settings_window_refresh(SettingsWindow *settings_window) {
  if (settings_window->menu) {
    layer_mark_dirty(menu_layer_get_layer(settings_window->menu));
  }
}



/*
 * set highlight color of this window
 * this is the overall color scheme used
 */

void settings_window_set_highlight_color(SettingsWindow *settings_window, GColor color) {
  settings_window->highlight_color = color;
  if (settings_window->menu) {
    menu_layer_set_highlight_colors(settings_window->menu, color, gcolor_legible_over(color));
  }
}

#endif  // PBL_PLATFORM_APLITE
