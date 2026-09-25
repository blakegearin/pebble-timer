/*******************************************************************************
 * FILENAME :        option_window.h
 *
 * DESCRIPTION :
 *      Create, destroy, and manage a reusable radio option window: a list of
 *      option labels reproducing the look of the firmware's own option menus
 *      with public API -- selection circles on rect, centred labels with the
 *      committed option set bold on round.
 *
 * PUBLIC FUNCTIONS :
 *      OptionWindow  *option_window_create(OptionWindowSelectCallback
 *                          selected, void *context);
 *      void          option_window_destroy(OptionWindow *option_window);
 *      void          option_window_push(OptionWindow *option_window,
 *                          const char *title, const char *const *labels,
 *                          uint8_t count, uint8_t selected,
 *                          const GColor *swatches, bool animated);
 *      void          option_window_set_highlight_color(OptionWindow
 *                          *option_window, GColor color);
 *
 * NOTES :      One window, re-pointed at one setting per push -- not one
 *              window per setting. Not compiled on aplite, which flips its
 *              settings inline; see .c for the guard.
 *
 * AUTHOR :     Blake Gearin        START DATE :    2026-09-24
 *
 */

#pragma once

#include <pebble.h>



/*******************************************************************************
 * CALLBACK DECLARATIONS
 */

/*
 * Callback:    OptionWindowSelectCallback
 * ---------------------------------------
 * called when an option is selected. the window has already popped itself by
 * the time this fires, so the caller can refresh whatever it just changed.
 */

typedef void (*OptionWindowSelectCallback)(uint8_t option, void *context);



/*******************************************************************************
 * STRUCTURE DECLARATION
 */

/*
 * Structure:   OptionWindow
 * -------------------------
 * main structure containing all data for an OptionWindow
 */

typedef struct OptionWindow OptionWindow;



/*******************************************************************************
 * API FUNCTIONS
 */

/*
 * Function:    option_window_create
 * ---------------------------------
 * creates a new OptionWindow in memory but does not push it into view
 *
 *  selected: called with the option the user picked
 *  context: passed back to selected
 *
 *  returns: a pointer to a new OptionWindow structure
 */

OptionWindow *option_window_create(OptionWindowSelectCallback selected, void *context);



/*
 * Function:    option_window_destroy
 * ----------------------------------
 * destroys an existing OptionWindow
 *
 *  option_window: a pointer to the OptionWindow being destroyed
 */

void option_window_destroy(OptionWindow *option_window);



/*
 * Function:    option_window_push
 * -------------------------------
 * re-points the window at one setting and pushes it onto the stack
 *
 *  option_window: a pointer to the OptionWindow being pushed
 *  title: the window title -- on rect a section header, on round nothing
 *  labels: the option labels, one per row
 *  count: the number of labels
 *  selected: the option the list opens on; the committed one carries the
 *      filled circle on rect and the bold label on round
 *  swatches: one colour per row, or NULL. when given, every row's circle
 *      carries that colour as a chip so the user can see each option before
 *      picking it -- this is what turns the radio list into a colour picker
 *  animated: whether to animate the push or not
 */

void option_window_push(OptionWindow *option_window, const char *title,
                        const char *const *labels, uint8_t count, uint8_t selected,
                        const GColor *swatches, bool animated);



/*
 * Function:    option_window_set_highlight_color
 * ----------------------------------------------
 * sets the over-all color scheme of the window
 *
 *  option_window: a pointer to the OptionWindow to configure
 *  color: the GColor to set the highlight to
 */

void option_window_set_highlight_color(OptionWindow *option_window, GColor color);
