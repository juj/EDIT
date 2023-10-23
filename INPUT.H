#ifndef _INPUT_H
#define _INPUT_H

#ifdef __C64__
#include "c64/c64_keycodes.h"
#include "c64/c64_shortcuts.h"
#elif defined(_MSC_VER) || defined(__TURBOC__)
#include "vs/KEYCODES.H"
#include "vs/SHRTCUTS.H"
#endif

bool is_shift_key_currently_down();
bool is_ctrl_key_currently_down();
bool is_alt_key_currently_down();

bool is_get_key_pending();
int get_key(); // block to wait for a received key
int get_key_and_poll_network(); // block to wait for a key, but pump network transfers while at it

bool is_shift_key_held(int key);
int is_navigation_action_key(int key);
bool is_printable_key(int key);

void init_keyboard();
void deinit_keyboard();

// Returns -1 if cancelled (ESC), or the length of entered string.
int ask_string_prompt(const char *prompt, char *dst, int maxLen);

// Returns 1 if yes, 0 if no
int ask_yes_no_prompt(const char *prompt);

#endif
