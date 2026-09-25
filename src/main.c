/*******************************************************************************
 * FILENAME :        main.c
 *
 * DESCRIPTION :
 *      Entry point and main logic for program. Controls everything
 *      of importance that happens.
 *
 * AUTHOR :     Eric Phillips        START DATE :    07/10/15
 *
 */

#include <pebble.h>
#include "countdown_timer.h"
#include "menu_window.h"
#include "detail_window.h"
#include "duration_window.h"
#include "popup_window.h"
#include "phone.h"
#include "settings.h"
#include "settings_window.h"
#include "option_window.h"

// constants
#define COUNTDOWN_TIMER_PERSIST_KEY 72445846
#define COUNTDOWN_TIMER_ID_PERSIST_KEY 3568356
// this key's *value* is part of the on-flash contract: 1.2.6 users already have
// their sort preference stored under 9938472, so renaming the #define is fine
// but changing the integer would silently drop their choice on upgrade.
// the stored int keeps meaning "1 = sort by duration" even though this fork
// ships duration as the default; the inversion lives in initialize/deinitialize
#define TIMER_SORT_BY_DURATION_PERSIST_KEY 9938472
#define TIMER_GROUPING_DISABLED_PERSIST_KEY 73849201
#define TIMER_START_MANUALLY_PERSIST_KEY 51827394
#define TIMER_DELETE_IMMEDIATELY_PERSIST_KEY 68013925
#define TIMER_HIGHLIGHT_COLOR_PERSIST_KEY 19283746
#define TIMER_SNOOZE_PERSIST_KEY 37492058
#define PERSIST_VERSION 1
#define PERSIST_VERSION_KEY 46134672
#define COUNTDOWN_TIMERS_MAX 8
#define TIMER_MIN_LENGTH 1000 // milliseconds
#define TIMELINE_MIN_LENGTH 900000 // milliseconds
#define INACTIVITY_THRESHOLD 900000 // length of time before refresh throttling in milliseconds
#define INACTIVE_REFRESH_DELAY 1000 // ms between frames after throttling
#define REFRESH_DELAY 1000 // ms between periodic redraws
#define POPUP_REFRESH_DELAY 35 // ms between popup animation frames
#define REFRESH_ALIGNMENT_DELAY 5 // ms after a second boundary to refresh
#define MIN_REFRESH_DELAY 25 // minimum delay when correcting near a boundary
#define PIN_ACTION_CODE_TRUNCATION_LEVEL 100 // both the pin id and action code have to be stored
                                             // in the pins action code
#define PIN_LAUNCH_ARGS_OPEN 10 // when opened from pin, action code to open timer in detail view


/*******************************************************************************
 * MAIN LOCAL VARIABLES
 */

static MenuWindow *s_menu_window = NULL;
static DetailWindow *s_detail_window = NULL;
static DurationWindow *s_duration_window = NULL;
static PopupWindow *s_popup_window = NULL;
#ifndef PBL_PLATFORM_APLITE
// aplite renders the settings as inline rows in the timer list instead of
// these two windows, and cannot afford either one's .text
static SettingsWindow *s_settings_window = NULL;
static OptionWindow *s_option_window = NULL;
// which setting s_option_window is currently pointed at, so the select
// callback knows what it just changed
static SettingId s_option_window_setting = SettingSortOrder;
#endif
static uint8_t s_countdown_timers_count = 0;
static CountdownTimer *s_countdown_timers[COUNTDOWN_TIMERS_MAX] = {};
static uint8_t s_timer_view_indices[COUNTDOWN_TIMERS_MAX] = {};
static bool s_timer_sort_by_last_used = false;
static bool s_grouping_disabled = false;
// naming rule: every setting's bool is named so that false is the shipped
// default. statics zero-initialise and an absent persist key leaves them
// untouched, so "no key yet" means "the shipped default" with no default table
// to keep in sync -- and a fresh install lands there automatically. The rule
// names the variables, not the on-flash ints: a stored value still means what
// 1.2.6 stored, so the two settings this fork re-defaults are inverted at the
// load/store boundary.
static bool s_start_timers_automatically = false;
static bool s_delete_immediately = false;
// the Snooze setting is not a bool, so the rule above cannot name its default
// either; index 0 is the shipped one ("1 Minute") and an absent persist key
// leaves this initialiser untouched, exactly like the accent colour's
static uint8_t s_snooze_option = 0;
#ifdef PBL_COLOR
// the app's accent colour. not a bool, so the false-is-default rule above
// cannot name it; the job is done here instead -- this initialiser is the
// shipped default, and an absent persist key leaves it alone
static GColor s_highlight_color = GColorMalachite;
#endif
static int32_t s_countdown_timer_id_max = 0;
static AppTimer *s_app_timer = NULL;
static int64_t s_last_activity = 0;

/*
 * the copy, one home for all the settings strings
 *
 * option index 0 is always the shipped default, which is what a zero value
 * means. this is an enum and a string table, not the data-driven descriptor
 * table the spec rejected -- it generates no UI. it exists because aplite
 * renders the same settings as rows in the timer list while the other
 * platforms render them in the settings window: two renderers, one copy.
 */
static const char *const s_setting_names[SettingCount] = {
  "Sort Order", "Group", "Start Timers", "Delete", "Snooze",
#ifdef PBL_COLOR
  "Accent Color",
#endif
};
static const char *const s_setting_options[SettingCount][2] = {
  { "Duration",      "Recency"       },
  { "Running First", "Off"           },
  { "Manually",      "Automatically" },
  { "Confirm First", "Immediately"   },
  { NULL, NULL },  // Snooze's options are the delay list below, not this table
#ifdef PBL_COLOR
  { NULL, NULL },  // Accent Color's options are the palette below, not this table
#endif
};

/*
 * the Snooze setting's options: how long the alarm waits before going off
 * again when the snooze button is pressed. index 0 is the shipped default
 * per the rule in settings.h, and Off sits last as its own sentinel: delay 0
 * means the popup shows no snooze icon at all.
 */
#define SNOOZE_OPTIONS 7
#define SNOOZE_OPTION_OFF (SNOOZE_OPTIONS - 1)
static const char *const s_snooze_options[SNOOZE_OPTIONS] = {
  "1 Minute", "5 Minutes", "10 Minutes", "15 Minutes", "30 Minutes", "1 Hour", "Off",
};
static const int64_t s_snooze_delays[SNOOZE_OPTIONS] = {
  60000, 300000, 600000, 900000, 1800000, 3600000, 0,
};

#ifdef PBL_COLOR
// the Accent Color setting's options: all sixty-four colours a colour platform can
// render, in rainbow order -- red, orange, yellow, green, blue, indigo,
// violet, dark to light within each band -- with the four true greys last.
// The live preview on the picker (option_window.c) is what makes neighbours
// like "Icterine" and "Pastel Yellow" tellable apart; the names are the
// SDK's, spaced for reading.
// index 0 is the shipped default, per the rule in settings.h, and Picton
// Blue follows it: the two colours the app has ever shipped with, kept at
// the top where the cursor lands, before the sweep.
#define COLOR_OPTIONS 64
static const char *const s_color_names[COLOR_OPTIONS] = {
  "Malachite",                 "Picton Blue",               "Bulgarian Rose",            "Dark Candy Apple Red",
  "Jazzberry Jam",             "Red",                       "Folly",                     "Rose Vale",
  "Sunset Orange",             "Brilliant Rose",            "Melon",                     "Windsor Tan",
  "Orange",                    "Chrome Yellow",             "Rajah",                     "Army Green",
  "Kelly Green",               "Limerick",                  "Brass",                     "Spring Bud",
  "Inchworm",                  "Yellow",                    "Icterine",                  "Pastel Yellow",
  "Dark Green",                "Midnight Green",            "Islamic Green",             "Jaeger Green",
  "Tiffany Blue",              "May Green",                 "Cadet Blue",                "Green",
  "Medium Spring Green",       "Bright Green",              "Cyan",                      "Screamin Green",
  "Medium Aquamarine",         "Electric Blue",             "Mint Green",                "Celeste",
  "Oxford Blue",               "Duke Blue",                 "Blue",                      "Cobalt Blue",
  "Blue Moon",                 "Liberty",                   "Very Light Blue",           "Vivid Cerulean",
  "Baby Blue Eyes",            "Indigo",                    "Electric Ultramarine",      "Vivid Violet",
  "Lavender Indigo",           "Imperial Purple",           "Purple",                    "Fashion Magenta",
  "Magenta",                   "Purpureus",                 "Shocking Pink",             "Rich Brilliant Lavender",
  "Black",                     "Dark Gray",                 "Light Gray",                "White",
};
static const GColor s_color_values[COLOR_OPTIONS] = {
  GColorMalachite,             GColorPictonBlue,            GColorBulgarianRose,         GColorDarkCandyAppleRed,
  GColorJazzberryJam,          GColorRed,                   GColorFolly,                 GColorRoseVale,
  GColorSunsetOrange,          GColorBrilliantRose,         GColorMelon,                 GColorWindsorTan,
  GColorOrange,                GColorChromeYellow,          GColorRajah,                 GColorArmyGreen,
  GColorKellyGreen,            GColorLimerick,              GColorBrass,                 GColorSpringBud,
  GColorInchworm,              GColorYellow,                GColorIcterine,              GColorPastelYellow,
  GColorDarkGreen,             GColorMidnightGreen,         GColorIslamicGreen,          GColorJaegerGreen,
  GColorTiffanyBlue,           GColorMayGreen,              GColorCadetBlue,             GColorGreen,
  GColorMediumSpringGreen,     GColorBrightGreen,           GColorCyan,                  GColorScreaminGreen,
  GColorMediumAquamarine,      GColorElectricBlue,          GColorMintGreen,             GColorCeleste,
  GColorOxfordBlue,            GColorDukeBlue,              GColorBlue,                  GColorCobaltBlue,
  GColorBlueMoon,              GColorLiberty,               GColorVeryLightBlue,         GColorVividCerulean,
  GColorBabyBlueEyes,          GColorIndigo,                GColorElectricUltramarine,   GColorVividViolet,
  GColorLavenderIndigo,        GColorImperialPurple,        GColorPurple,                GColorFashionMagenta,
  GColorMagenta,               GColorPurpureus,             GColorShockingPink,          GColorRichBrilliantLavender,
  GColorBlack,                 GColorDarkGray,              GColorLightGray,             GColorWhite,
};
#endif

static uint16_t prv_get_next_refresh_delay(void) {
  if (popup_window_get_topmost_window(s_popup_window)) {
    return POPUP_REFRESH_DELAY;
  }

  CountdownTimer *next_timer = countdown_timer_list_get_closest_timer(s_countdown_timers,
    s_countdown_timers_count);
  if (next_timer == NULL) {
    return REFRESH_DELAY;
  }

  int64_t remaining = countdown_timer_get_current_time(next_timer);
  if (remaining <= 0) {
    return MIN_REFRESH_DELAY;
  }

  int64_t delay = remaining % REFRESH_DELAY;
  if (delay == 0) {
    delay = REFRESH_DELAY;
  }
  delay += REFRESH_ALIGNMENT_DELAY;
  if (delay < MIN_REFRESH_DELAY) {
    delay = MIN_REFRESH_DELAY;
  }
  return (uint16_t)delay;
}



/*
 * decides whether timer "a" should be listed above timer "b"
 *
 * running timers come before paused ones -- unless Group is Off -- and within
 * each group the most recently used timer (largest last_update) comes first.
 * "Used" means started, paused, or edited -- anything that touches a timer's
 * last_update.
 */

static bool prv_timer_precedes(CountdownTimer *a, CountdownTimer *b) {
  bool a_running = !countdown_timer_get_paused(a);
  bool b_running = !countdown_timer_get_paused(b);
  if (!s_grouping_disabled && a_running != b_running) {
    return a_running;
  }
  return countdown_timer_get_last_update(a) > countdown_timer_get_last_update(b);
}



/*
 * decides whether timer "a" should be listed above timer "b" when sorting by
 * length
 *
 * strictly shorter wins, so equal-length timers keep the order they arrived in
 * -- which is recency order, since the storage array is always recency-sorted
 */

static bool prv_timer_is_shorter(CountdownTimer *a, CountdownTimer *b) {
  return countdown_timer_get_duration(a) < countdown_timer_get_duration(b);
}



/*
 * an ordering over two timers, as used by prv_sort_timers
 */

typedef bool (*TimerPrecedes)(CountdownTimer *a, CountdownTimer *b);



/*
 * order "timers" in place, placing "a" before "b" whenever precedes(a, b)
 *
 * insertion sort is fine here: the list holds at most COUNTDOWN_TIMERS_MAX
 * entries. it is also stable, which both orderings rely on for tiebreaking.
 */

static void prv_sort_timers(CountdownTimer **timers, uint8_t count, TimerPrecedes precedes) {
  for (uint8_t i = 1; i < count; i++) {
    CountdownTimer *key = timers[i];
    int16_t j = (int16_t)i - 1;
    while (j >= 0 && precedes(key, timers[j])) {
      timers[j + 1] = timers[j];
      j--;
    }
    timers[j + 1] = key;
  }
}



/*
 * rebuild the view slot -> storage index mapping the MenuWindow reads through
 *
 * the storage array is always kept in recency order, so the recency view is
 * just the identity mapping and the duration view is a reordering laid over the
 * top. keeping the two separate means toggling the sort never destroys recency.
 */

static void prv_rebuild_timer_view_indices(void) {
  if (s_timer_sort_by_last_used) {
    for (uint8_t i = 0; i < s_countdown_timers_count; i++) {
      s_timer_view_indices[i] = i;
    }
    return;
  }

  CountdownTimer *by_duration[COUNTDOWN_TIMERS_MAX];
  memcpy(by_duration, s_countdown_timers, sizeof(CountdownTimer*) * s_countdown_timers_count);
  prv_sort_timers(by_duration, s_countdown_timers_count, prv_timer_is_shorter);
  for (uint8_t i = 0; i < s_countdown_timers_count; i++) {
    s_timer_view_indices[i] = (uint8_t)countdown_timer_list_get_timer_index(s_countdown_timers,
      s_countdown_timers_count, by_duration[i]);
  }
}



/*
 * re-establish the list invariants after any change to the timers
 *
 * every mutation of the timer list funnels through here: the storage array goes
 * back into recency order (running timers on top, most recently used first) and
 * the view mapping is rebuilt to match the selected sort.
 */

static void prv_timers_changed(void) {
  prv_sort_timers(s_countdown_timers, s_countdown_timers_count, prv_timer_precedes);
  prv_rebuild_timer_view_indices();
}



/*
 * promote a just-used timer to the top of its group
 *
 * last_update only has one-second resolution, so several timers touched in the
 * same second compare equal. Moving the touched timer to the front of the array
 * first means the stable sort keeps it ahead of those same-second peers, so the
 * timer the user actually just used ends up on top of its running/paused group.
 */

static void prv_promote_timer(CountdownTimer *countdown_timer) {
  int16_t index = countdown_timer_list_get_timer_index(s_countdown_timers,
    s_countdown_timers_count, countdown_timer);
  if (index > 0) {
    memmove(&s_countdown_timers[1], &s_countdown_timers[0],
      sizeof(CountdownTimer*) * index);
    s_countdown_timers[0] = countdown_timer;
  }
  prv_timers_changed();

  /*
   * the list opens with the cursor on the timer you last used
   *
    * the order is untouched -- a timer created paused still lands below every
    * running one when Group is Running First -- but selection follows use. all four acting
    * paths (create, edit, play/pause, snooze) already funnel through here, so
    * this one place covers them all. delete and timer-expiry never promote,
    * and so never steal the cursor: a deleted timer has no row to land on, and
    * an expired one gets the stronger signal of a PopupWindow.
   */
  for (uint8_t view = 0; view < s_countdown_timers_count; view++) {
    if (s_countdown_timers[s_timer_view_indices[view]] == countdown_timer) {
      menu_window_select_timer_index(s_menu_window, view);
      break;
    }
  }
}



/*
 * move a timer between running and paused, and keep its Timeline pin in step
 *
 * the one rule this enforces: a pin exists for a timer if and only if that
 * timer is running and its duration is at least TIMELINE_MIN_LENGTH. Every
 * running/paused transition in the app goes through here, because the five
 * hand-copied versions this replaces had already drifted apart.
 *
 * two orderings are load-bearing:
 *   - delete the pin *before* stopping. countdown_timer_stop rolls the timer's
 *     id, and the pin is identified by that id, so deleting afterwards would
 *     delete nothing.
 *   - start *before* sending the pin, so a pin is only ever sent for a timer
 *     that is definitively running.
 *
 * the pause half is guarded against a call that is not a real transition --
 * pausing an already-paused timer, as "Manually" does on create, neither
 * deletes a pin nor stops anything.
 */

static void prv_set_timer_running(CountdownTimer *countdown_timer, bool running) {
  if (running) {
    if (countdown_timer_get_current_time(countdown_timer) <= 0) {
      countdown_timer_update(countdown_timer,
        countdown_timer_get_duration(countdown_timer), false);
    }
    countdown_timer_start(countdown_timer);
    if (countdown_timer_get_duration(countdown_timer) >= TIMELINE_MIN_LENGTH) {
      phone_send_pin(countdown_timer);
    }
  } else {
    // The pin is deleted on the running->paused *transition*, per the rule.
    // When the timer is already paused -- creating under "Manually", or
    // editing a timer that was already paused -- there is no transition and
    // no pin, so delete nothing. countdown_timer_stop carries the same
    // idempotence guard for the state itself.
    if (!countdown_timer_get_paused(countdown_timer)) {
      if (countdown_timer_get_duration(countdown_timer) >= TIMELINE_MIN_LENGTH) {
        phone_delete_pin(countdown_timer);
      }
      countdown_timer_stop(countdown_timer, &s_countdown_timer_id_max);
    }
  }
}



/*
 * the only place the SettingId enum meets the bools behind it
 */

#ifdef PBL_COLOR
/*
 * push the accent colour into every window that owns a highlight. the setters
 * stay per-window -- that is how the windows were built -- and this is the
 * one place that knows all six. PBL_COLOR platforms are exactly the ones
 * that own the settings and option windows.
 */
static void prv_apply_highlight_color(void) {
  menu_window_set_highlight_color(s_menu_window, s_highlight_color);
  detail_window_set_highlight_color(s_detail_window, s_highlight_color);
  duration_window_set_highlight_color(s_duration_window, s_highlight_color);
  popup_window_set_highlight_color(s_popup_window, s_highlight_color);
  settings_window_set_highlight_color(s_settings_window, s_highlight_color);
  option_window_set_highlight_color(s_option_window, s_highlight_color);
}
#endif

static uint8_t prv_get_setting(SettingId setting) {
  switch (setting) {
    case SettingSortOrder:
      return s_timer_sort_by_last_used ? 1 : 0;
    case SettingGroup:
      return s_grouping_disabled ? 1 : 0;
    case SettingStartTimers:
      return s_start_timers_automatically ? 1 : 0;
    case SettingDelete:
      return s_delete_immediately ? 1 : 0;
    case SettingSnooze:
      return s_snooze_option;
#ifdef PBL_COLOR
    case SettingColor:
      for (uint8_t i = 0; i < COLOR_OPTIONS; i++) {
        if (gcolor_equal(s_color_values[i], s_highlight_color)) {
          return i;
        }
      }
      return 0;  // a colour outside the palette reads as the default
#endif
    default:
      return 0;
  }
}

static void prv_set_setting(SettingId setting, uint8_t option) {
  switch (setting) {
    case SettingSortOrder:
      s_timer_sort_by_last_used = (option != 0);
      prv_rebuild_timer_view_indices();
      break;
    case SettingGroup:
      s_grouping_disabled = (option != 0);
      // the storage array itself is grouped, so re-establish both invariants
      prv_timers_changed();
      break;
    case SettingStartTimers:
      s_start_timers_automatically = (option != 0);
      break;
    case SettingDelete:
      s_delete_immediately = (option != 0);
      // the detail window owns the arming, so hand it the new value now
      detail_window_set_delete_immediately(s_detail_window, s_delete_immediately);
      break;
    case SettingSnooze:
      s_snooze_option = (option < SNOOZE_OPTIONS) ? option : 0;
      break;
#ifdef PBL_COLOR
    case SettingColor:
      s_highlight_color = s_color_values[option];
      prv_apply_highlight_color();
      break;
#endif
    default:
      break;
  }
}


/*
 * how many options a setting offers
 *
 * every renderer that has to walk a setting's options -- the aplite row cycle
 * and the option window push -- asks here instead of assuming two
 */

static uint8_t prv_get_option_count(SettingId setting) {
  switch (setting) {
    case SettingSnooze:
      return SNOOZE_OPTIONS;
#ifdef PBL_COLOR
    case SettingColor:
      return COLOR_OPTIONS;
#endif
    default:
      return 2;
  }
}



/*******************************************************************************
 * CALLBACKS
 */

/*
 * AppTimer callback
 *
 * update callback which determines refresh rate
 */

static void app_timer_callback(void *data) {
  s_app_timer = NULL;

  // check for expired timers
  CountdownTimer *countdown_timer = countdown_timer_check_ended(s_countdown_timers,
    s_countdown_timers_count);

  if (countdown_timer != NULL) {
    // a timer just expired and is now paused; re-sort so, when Group is
    // Running First, it drops below any still-running timers
    prv_timers_changed();
    // deep refresh the DetailWindow in case it was that timer
    detail_window_deep_refresh(s_detail_window);
    // show timer confirmation window
    popup_window_set_countdown_timer(s_popup_window, countdown_timer);
    popup_window_set_title(s_popup_window, "Time's Up!");
    popup_window_set_highlight_color(s_popup_window, PBL_IF_COLOR_ELSE(s_highlight_color, GColorWhite));
#ifdef PBL_PLATFORM_APLITE
    popup_window_set_image(s_popup_window, RESOURCE_ID_IMAGE_ALARM);
#else
    popup_window_set_pdc(s_popup_window, RESOURCE_ID_ICON_ALARM_CLOCK, true);
#endif
    popup_window_set_auto_close_duration(s_popup_window, 15000);
    popup_window_set_snooze_enabled(s_popup_window, s_snooze_option != SNOOZE_OPTION_OFF);
    popup_window_add_action_bar(s_popup_window);
    popup_window_push(s_popup_window, true);
    popup_window_set_vibes();

    // we want the alarm going off to count as activity
    s_last_activity = countdown_timer_get_epoch_ms();
  }

  // refresh
  bool menu_top = menu_window_get_topmost_window(s_menu_window);
  bool detail_top = detail_window_get_topmost_window(s_detail_window);
  bool popup_top = popup_window_get_topmost_window(s_popup_window);
  if (menu_top) menu_window_refresh(s_menu_window);
  if (detail_top) detail_window_refresh(s_detail_window);
  if (popup_top) popup_window_refresh(s_popup_window);

  // check activity
  int64_t inactivity_duration = countdown_timer_get_epoch_ms() - s_last_activity;

  // schedule next refresh
  uint16_t refresh_rate = prv_get_next_refresh_delay();
  if (popup_top) {
    inactivity_duration = 0;
  }
  if (refresh_rate == 0) {
    return;
  }
  // cap refresh rate if inactive
  if (inactivity_duration > INACTIVITY_THRESHOLD) {
    refresh_rate = (refresh_rate > INACTIVE_REFRESH_DELAY) ? refresh_rate : INACTIVE_REFRESH_DELAY;
  }
  s_app_timer = app_timer_register(refresh_rate, app_timer_callback, NULL);
}



/*
 * PopupWindow snooze timer callback
 * snoozes the vibrating timer for the duration the Snooze setting picks.
 * the popup hides the snooze icon when the setting is Off, so the delay
 * check is just a guard against a click config that predates the change
 */

static void popup_window_snooze_timer_callback(CountdownTimer *countdown_timer, void *context) {
  int64_t snooze_delay = s_snooze_delays[s_snooze_option];
  if (snooze_delay <= 0) {
    return;
  }
  countdown_timer_update(countdown_timer, snooze_delay, false);
  countdown_timer_start(countdown_timer);
  prv_promote_timer(countdown_timer);
  popup_window_pop(s_popup_window, true);
  // show detail if not on top
  if (!detail_window_get_topmost_window(s_detail_window)) {
    detail_window_set_countdown_timer(s_detail_window, countdown_timer);
    detail_window_set_delete_immediately(s_detail_window, s_delete_immediately);
    detail_window_push(s_detail_window, false);
  }
  detail_window_deep_refresh(s_detail_window);
  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * PopupWindow stop timer callback
 * cancels the current timer vibration sequence
 */

static void popup_window_stop_timer_callback(void *context) {
  // pop the window
  popup_window_pop(s_popup_window, true);

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * DurationWindow complete callback
 * simple window to create or edit timer durations
 */

static void duration_window_complete_callback(int64_t duration, void *context) {
  DurationWindow *duration_window = (DurationWindow*)context;
  CountdownTimer *countdown_timer = duration_window_get_timer(duration_window);
  // check if long enough
  if (duration < TIMER_MIN_LENGTH) {
    duration_window_pop(duration_window, true);
    if (s_app_timer != NULL) {
      app_timer_reschedule(s_app_timer, MIN_REFRESH_DELAY);
    }
    return;
  }

  // check if new timer or editing
  if (countdown_timer == NULL) {
    countdown_timer = countdown_timer_create(duration, &s_countdown_timer_id_max);
    // list_add destroys the tail timer when full, and it knows nothing about
    // Timeline pins -- per the pin rule, that is this layer's job. The victim
    // is the storage tail, and only a running one of at least TIMELINE_MIN_LENGTH
    // can own a pin.
    if (s_countdown_timers_count == COUNTDOWN_TIMERS_MAX) {
      CountdownTimer *evicted = s_countdown_timers[COUNTDOWN_TIMERS_MAX - 1];
      if (!countdown_timer_get_paused(evicted) &&
          countdown_timer_get_duration(evicted) >= TIMELINE_MIN_LENGTH) {
        phone_delete_pin(evicted);
      }
    }
    countdown_timer_list_add(s_countdown_timers, COUNTDOWN_TIMERS_MAX,
      &s_countdown_timers_count, countdown_timer);
    // Start Timers is absolute: under Manually the create path lands the
    // timer paused. The chokepoint skips a stop that is not a real transition,
    // so this is a true no-op -- a fresh timer is already paused and owns no
    // pin.
    prv_set_timer_running(countdown_timer, s_start_timers_automatically);
    // update visuals
    menu_window_reload_data(s_menu_window);
    menu_window_refresh(s_menu_window);
    detail_window_set_countdown_timer(s_detail_window, countdown_timer);
    detail_window_set_delete_immediately(s_detail_window, s_delete_immediately);
    duration_window_pop(duration_window, false);
    detail_window_push(s_detail_window, true);
    detail_window_deep_refresh(s_detail_window);
  } else {
    // stop first, while the *old* duration is still in place: a timer edited
    // from above TIMELINE_MIN_LENGTH down to below it still has a stale pin,
    // and only the old duration passes the guard that deletes it
    prv_set_timer_running(countdown_timer, false);
    countdown_timer_update(countdown_timer, duration, true);
    // the same setting governs edit as create: the timer lands in the state
    // the setting names either way
    prv_set_timer_running(countdown_timer, s_start_timers_automatically);
    detail_window_deep_refresh(s_detail_window);
    duration_window_pop(duration_window, true);
  }

  // keep the list in order and put the cursor on the timer just used
  prv_promote_timer(countdown_timer);

  // refresh now
  if (s_app_timer != NULL) {
    app_timer_reschedule(s_app_timer, MIN_REFRESH_DELAY);
  }

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * DetailWindow edit timer callback
 * edit the timer currently in the detail view
 */

static void detail_window_edit_timer_callback(CountdownTimer *countdown_timer, void *context) {
  duration_window_set_timer(s_duration_window, countdown_timer);
  duration_window_push(s_duration_window, true);

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * DetailWindow play pause timer callback
 * plays or pauses the timer currently in the detail view
 */

static void detail_window_playpause_timer_callback(CountdownTimer *countdown_timer, void *context) {
  prv_set_timer_running(countdown_timer, countdown_timer_get_paused(countdown_timer));
  // keep the list in order and put the cursor on the timer just used
  prv_promote_timer(countdown_timer);
  // refresh DetailWindow
  detail_window_deep_refresh(s_detail_window);

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * DetailWindow delete timer callback
 * delete the timer currently in the detail view
 */

static void detail_window_delete_timer_callback(CountdownTimer *countdown_timer, void *context) {
  // delete the Timeline pin
  if (countdown_timer_get_duration(countdown_timer) >= TIMELINE_MIN_LENGTH) {
    phone_delete_pin(countdown_timer);
  }

  // delete the timer
  int16_t timer_index = countdown_timer_list_get_timer_index(s_countdown_timers,
    s_countdown_timers_count, countdown_timer);
  countdown_timer_destroy(countdown_timer);
  countdown_timer_list_remove(s_countdown_timers, &s_countdown_timers_count, timer_index);
  prv_timers_changed();
  // reload MenuWindow data (no idea why, but this must be called twice or when the last timer
  // is deleted, the "+" cell is stuck at the short cell height)
  menu_window_reload_data(s_menu_window);
  menu_window_reload_data(s_menu_window);
  // pop detail off stack
  detail_window_pop(s_detail_window, true);

  // show timer confirmation window
  popup_window_set_title(s_popup_window, "Timer Deleted");
  popup_window_set_highlight_color(s_popup_window, PBL_IF_COLOR_ELSE(s_highlight_color, GColorWhite));
#ifdef PBL_PLATFORM_APLITE
  popup_window_set_image(s_popup_window, RESOURCE_ID_IMAGE_SHREADER);
  popup_window_set_auto_close_duration(s_popup_window, 1000);
#else
  popup_window_set_pdc(s_popup_window, RESOURCE_ID_ICON_DELETED, false);
  int64_t pdc_duration = popup_window_get_pdc_duration(s_popup_window);
  popup_window_set_auto_close_duration(s_popup_window, pdc_duration);
#endif
  popup_window_remove_action_bar(s_popup_window);
  popup_window_push(s_popup_window, true);
  popup_window_refresh(s_popup_window);

  // refresh immediately
  if (s_app_timer) {
    app_timer_reschedule(s_app_timer, POPUP_REFRESH_DELAY);
  } else {
    s_app_timer = app_timer_register(POPUP_REFRESH_DELAY, app_timer_callback, NULL);
  }

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*
 * MenuWindow get timer callback
 * gets a pointer to a timer at a specific index
 */

static CountdownTimer *menu_window_get_timer_callback(uint8_t index, void *context) {
  if (index < s_countdown_timers_count) {
    return s_countdown_timers[s_timer_view_indices[index]];
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to access timer outside array bounds");
  return NULL;
}



/*
 * MenuWindow get timer count callback
 * get the total number of timers
 */

static uint8_t menu_window_get_timer_count_callback(void *context) {
  return s_countdown_timers_count;
}



/*
 * Settings copy callbacks
 *
 * one pair feeds both renderers of the settings: the inline rows in
 * menu_window (only ever drawn on aplite) and, on every other platform,
 * the settings window.
 */

static const char *settings_name_callback(uint8_t setting, void *context) {
  if (setting < SettingCount) {
    return s_setting_names[setting];
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to access setting outside the enum");
  return "";
}

static const char *settings_value_callback(uint8_t setting, void *context) {
  if (setting < SettingCount) {
#ifdef PBL_COLOR
    if ((SettingId)setting == SettingColor) {
      return s_color_names[prv_get_setting(SettingColor)];
    }
#endif
    if ((SettingId)setting == SettingSnooze) {
      return s_snooze_options[prv_get_setting(SettingSnooze)];
    }
    return s_setting_options[setting][prv_get_setting((SettingId)setting)];
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to access setting outside the enum");
  return "";
}



#ifndef PBL_PLATFORM_APLITE

/*
 * SettingsWindow clicked callback
 * re-point the one option window at the clicked setting and open it
 */

static void settings_window_clicked_callback(uint8_t setting, void *context) {
  s_option_window_setting = (SettingId)setting;
#ifdef PBL_COLOR
  if ((SettingId)setting == SettingColor) {
    option_window_push(s_option_window, s_setting_names[setting], s_color_names,
      COLOR_OPTIONS, prv_get_setting(SettingColor), s_color_values, true);
    return;
  }
#endif
  if ((SettingId)setting == SettingSnooze) {
    option_window_push(s_option_window, s_setting_names[setting], s_snooze_options,
      SNOOZE_OPTIONS, prv_get_setting(SettingSnooze), NULL, true);
    return;
  }
  option_window_push(s_option_window, s_setting_names[setting], s_setting_options[setting],
    2, prv_get_setting((SettingId)setting), NULL, true);
}



/*
 * OptionWindow selected callback
 *
 * the option window has already popped itself by the time this runs, so the
 * settings window is the topmost again and shows the new value immediately.
 * the timer list underneath needs no touch: the MenuLayer redraws the rows
 * through the view mapping as soon as it is topmost again.
 */

static void option_window_selected_callback(uint8_t option, void *context) {
  prv_set_setting(s_option_window_setting, option);
  settings_window_refresh(s_settings_window);

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}

#endif  // PBL_PLATFORM_APLITE



/*
 * MenuWindow click callback
 */

static void menu_window_click_callback(MenuRowKind kind, uint8_t row, void *context) {
  switch (kind) {
    case MenuRowAdd: {
      // add a timer: open the duration picker with no timer to edit
      duration_window_set_timer(s_duration_window, NULL);
      duration_window_push(s_duration_window, true);
      break;
    }
    case MenuRowTimer: {
      // show timer in detail window
      const int16_t view_index = menu_window_row_to_timer_index(s_menu_window, row);
      if (view_index < 0) {
        break;
      }
      CountdownTimer *countdown_timer = menu_window_get_timer_callback((uint8_t)view_index,
        context);
      if (countdown_timer == NULL) {
        break;
      }
      detail_window_set_countdown_timer(s_detail_window, countdown_timer);
      detail_window_set_delete_immediately(s_detail_window, s_delete_immediately);
      detail_window_push(s_detail_window, true);
      detail_window_deep_refresh(s_detail_window);
      if (s_app_timer != NULL) {
        app_timer_reschedule(s_app_timer, MIN_REFRESH_DELAY);
      }
      break;
    }
    case MenuRowSettings: {
      // the cog row only exists off aplite, but the switch arms are compiled
      // everywhere so the row model stays out of #ifdefs
#ifndef PBL_PLATFORM_APLITE
      settings_window_push(s_settings_window, true);
#endif
      break;
    }
    case MenuRowSetting: {
      // on aplite a settings row cycles its options in place, in one press
      const int16_t setting = menu_window_row_to_setting_index(s_menu_window, row);
      if (setting < 0) {
        break;
      }
      const uint8_t next = (prv_get_setting((SettingId)setting) + 1) %
        prv_get_option_count((SettingId)setting);
      prv_set_setting((SettingId)setting, next);
      if ((SettingId)setting == SettingSortOrder || (SettingId)setting == SettingGroup) {
        // the timer order just changed, so reload -- which drops the
        // selection back to the "+" row. put the user back on the row they
        // just pressed.
        menu_window_reload_data(s_menu_window);
        menu_window_select_row(s_menu_window, row);
      }
      // the remaining settings changed only row content, not row count
      menu_window_refresh(s_menu_window);
      break;
    }
  }

  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}



/*******************************************************************************
 * INITIALIZE AND DEINITIALIZE
 */

/*
 * initialize the program
 */

static void initialize(void) {
  // connect to phone
  phone_connect();
  // load the CountdownTimer data
  if (persist_exists(COUNTDOWN_TIMER_PERSIST_KEY)) {
    countdown_timer_list_load(s_countdown_timers, COUNTDOWN_TIMERS_MAX,
      &s_countdown_timers_count, COUNTDOWN_TIMER_PERSIST_KEY);
  }
  if (persist_exists(COUNTDOWN_TIMER_ID_PERSIST_KEY)) {
    s_countdown_timer_id_max = persist_read_int(COUNTDOWN_TIMER_ID_PERSIST_KEY);
  }
  if (persist_exists(TIMER_SORT_BY_DURATION_PERSIST_KEY)) {
    // stored int keeps 1.2.6's meaning: 1 = sort by duration. this fork
    // defaults to duration, so the variable is its inverse
    s_timer_sort_by_last_used = (persist_read_int(TIMER_SORT_BY_DURATION_PERSIST_KEY) == 0);
  }
  if (persist_exists(TIMER_GROUPING_DISABLED_PERSIST_KEY)) {
    // a key this fork introduced, so it stores the bool's own meaning: no
    // 1.2.6 contract to honour and no inversion at the load boundary
    s_grouping_disabled = (persist_read_int(TIMER_GROUPING_DISABLED_PERSIST_KEY) != 0);
  }
  if (persist_exists(TIMER_START_MANUALLY_PERSIST_KEY)) {
    // same contract: 1 = start manually, and the fork's default inverts it
    s_start_timers_automatically = (persist_read_int(TIMER_START_MANUALLY_PERSIST_KEY) == 0);
  }
  if (persist_exists(TIMER_DELETE_IMMEDIATELY_PERSIST_KEY)) {
    s_delete_immediately = (persist_read_int(TIMER_DELETE_IMMEDIATELY_PERSIST_KEY) != 0);
  }
  if (persist_exists(TIMER_SNOOZE_PERSIST_KEY)) {
    int saved = persist_read_int(TIMER_SNOOZE_PERSIST_KEY);
    s_snooze_option = (saved >= 0 && saved < SNOOZE_OPTIONS) ? (uint8_t)saved : 0;
  }
#ifdef PBL_COLOR
  if (persist_exists(TIMER_HIGHLIGHT_COLOR_PERSIST_KEY)) {
    s_highlight_color = (GColor) {
      .argb = (uint8_t)persist_read_int(TIMER_HIGHLIGHT_COLOR_PERSIST_KEY)
    };
  }
#endif
  // cancel wakeup
  wakeup_cancel_all();

  // open the restored list with the most recently used timer on top
  prv_timers_changed();

  // create menu window
  MenuWindowCallbacks menu_callbacks = {
    .get_timer = menu_window_get_timer_callback,
    .get_timer_count = menu_window_get_timer_count_callback,
    .get_setting_name = settings_name_callback,
    .get_setting_value = settings_value_callback,
    .clicked = menu_window_click_callback,
  };
  s_menu_window = menu_window_create(menu_callbacks, true);
  menu_window_set_highlight_color(s_menu_window, PBL_IF_COLOR_ELSE(s_highlight_color, GColorBlack));
  menu_window_refresh(s_menu_window);

  // create detail window
  DetailWindowCallbacks detail_callbacks = {
    .edit_timer = detail_window_edit_timer_callback,
    .playpause_timer = detail_window_playpause_timer_callback,
    .delete_timer = detail_window_delete_timer_callback,
  };
  s_detail_window = detail_window_create(detail_callbacks);
  detail_window_set_highlight_color(s_detail_window,PBL_IF_COLOR_ELSE(s_highlight_color, GColorWhite));

  // create duration window
  DurationWindowCallbacks duration_callbacks = {
    .duration_complete = duration_window_complete_callback,
  };
  s_duration_window = duration_window_create(duration_callbacks);
  duration_window_set_highlight_color(s_duration_window, PBL_IF_COLOR_ELSE(s_highlight_color, GColorBlack));

  // create pop-up window
  PopupWindowCallbacks popup_callbacks = {
    .up_click = popup_window_snooze_timer_callback,
    .down_click = popup_window_stop_timer_callback,
  };
  s_popup_window = popup_window_create();
  popup_window_set_action_bar_callbacks(s_popup_window, popup_callbacks);

  // create settings and option windows
  // aplite has neither: its settings are inline rows in the timer list, and
  // the two windows' .text does not fit in its 24 KB alongside the data
#ifndef PBL_PLATFORM_APLITE
  SettingsWindowCallbacks settings_callbacks = {
    .get_name = settings_name_callback,
    .get_value = settings_value_callback,
    .clicked = settings_window_clicked_callback,
  };
  s_settings_window = settings_window_create(settings_callbacks);
  settings_window_set_highlight_color(s_settings_window,
    PBL_IF_COLOR_ELSE(s_highlight_color, GColorBlack));
  s_option_window = option_window_create(option_window_selected_callback, NULL);
  option_window_set_highlight_color(s_option_window,
    PBL_IF_COLOR_ELSE(s_highlight_color, GColorBlack));
#endif

  // check wakeup in case launched by pin
  if (launch_reason() == APP_LAUNCH_TIMELINE_ACTION) {
    uint32_t args = launch_get_args() % PIN_ACTION_CODE_TRUNCATION_LEVEL;
    if (args == PIN_LAUNCH_ARGS_OPEN) {
      CountdownTimer *countdown_timer = countdown_timer_list_get_timer_by_id(s_countdown_timers,
        s_countdown_timers_count, launch_get_args() / PIN_ACTION_CODE_TRUNCATION_LEVEL);
      if (countdown_timer != NULL) {
        // show timer in detail window
        detail_window_set_countdown_timer(s_detail_window, countdown_timer);
        detail_window_set_delete_immediately(s_detail_window, s_delete_immediately);
        detail_window_push(s_detail_window, true);
        detail_window_deep_refresh(s_detail_window);
      }
    }
  }

  // open the duration picker if no timers
  if (s_countdown_timers_count == 0) {
    duration_window_set_timer(s_duration_window, NULL);
    duration_window_push(s_duration_window, true);
  }

  // start the main update timer
  s_app_timer = app_timer_register(prv_get_next_refresh_delay(), app_timer_callback, NULL);


  // log activity
  s_last_activity = countdown_timer_get_epoch_ms();
}

static void prv_add_slice(AppGlanceReloadSession *session, const char *str, time_t expiration_time) {
  const AppGlanceSlice slice = {
    .layout.subtitle_template_string = str,
    .expiration_time = expiration_time
  };
  AppGlanceResult result = app_glance_add_slice(session, slice);
  if (result != APP_GLANCE_RESULT_SUCCESS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error : %d", result);
  }
}

// update app glance
static void prv_update_app_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
  // Ensure we have sufficient slices
  if (limit < 1) {
    return;
  }

  time_t expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION;

  CountdownTimer *countdown_timer = countdown_timer_list_get_closest_timer(s_countdown_timers,
    s_countdown_timers_count);

  if (countdown_timer != NULL) {
    expiration_time = time(NULL) + countdown_timer_get_current_time(countdown_timer) / 1000; // expires when the timer is ended
    char buff_glance[40] = {0}; // {time_until(4294967295)|format('%fT')}
    snprintf(buff_glance, sizeof(buff_glance), "{time_until(%ld)|format('%%fT')}", expiration_time);
    prv_add_slice(session, buff_glance, expiration_time);
  } else {
    countdown_timer = countdown_timer_list_get_last_updated_timer(s_countdown_timers, s_countdown_timers_count);
    if (countdown_timer != NULL) {
      expiration_time = countdown_timer_get_last_update(countdown_timer) + SECONDS_PER_HOUR;
      if (expiration_time > time(NULL)) { // don't display the appglance if the timer is paused for > 1 hour
        prv_add_slice(session, countdown_timer_format_own_buff(countdown_timer), expiration_time);
      }
    }
  }
}


/*
 * deinitialize the program
 */

static void deinitialize(void) {
  // cancel the timer if it is still registered
  if (s_app_timer != NULL) {
    app_timer_cancel(s_app_timer);
  }
  // disconnect from phone
  phone_disconnect();

  // persist state
  persist_write_int(PERSIST_VERSION_KEY, PERSIST_VERSION);
  persist_write_int(COUNTDOWN_TIMER_ID_PERSIST_KEY, s_countdown_timer_id_max);
  persist_write_int(TIMER_SORT_BY_DURATION_PERSIST_KEY, s_timer_sort_by_last_used ? 0 : 1);
  persist_write_int(TIMER_GROUPING_DISABLED_PERSIST_KEY, s_grouping_disabled ? 1 : 0);
  persist_write_int(TIMER_START_MANUALLY_PERSIST_KEY, s_start_timers_automatically ? 0 : 1);
  persist_write_int(TIMER_DELETE_IMMEDIATELY_PERSIST_KEY, s_delete_immediately ? 1 : 0);
  persist_write_int(TIMER_SNOOZE_PERSIST_KEY, s_snooze_option);
#ifdef PBL_COLOR
  persist_write_int(TIMER_HIGHLIGHT_COLOR_PERSIST_KEY, s_highlight_color.argb);
#endif
  countdown_timer_list_save(s_countdown_timers, s_countdown_timers_count,
    COUNTDOWN_TIMER_PERSIST_KEY);
  // schedule the wakeup
  CountdownTimer *countdown_timer = countdown_timer_list_get_closest_timer(s_countdown_timers,
    s_countdown_timers_count);
  if (countdown_timer != NULL) {
    time_t timestamp = time(NULL) + countdown_timer_get_current_time(countdown_timer) / 1000;
    // add one second to ensure it opens straight to the PopupWindow
    wakeup_schedule(timestamp + 1, 0, true);
  }

  // update appglance
  app_glance_reload(prv_update_app_glance, NULL);

  // destroy classes
  popup_window_destroy(s_popup_window);
#ifndef PBL_PLATFORM_APLITE
  option_window_destroy(s_option_window);
  settings_window_destroy(s_settings_window);
#endif
  duration_window_destroy(s_duration_window);
  detail_window_destroy(s_detail_window);
  menu_window_destroy(s_menu_window);
  countdown_timer_list_destroy_all(s_countdown_timers, &s_countdown_timers_count);
}



/*
 * main entry point
 */

int main(void) {
  initialize();
  app_event_loop();
  deinitialize();
}
