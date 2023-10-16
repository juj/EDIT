#ifndef _INPUT_H
#define _INPUT_H

#define KEY_SHIFT_MODIFIER 0x4000

#define KEY_CTRL_A 0x01
#define KEY_CTRL_C 0x03
#define KEY_CTRL_G 0x07
#define KEY_CTRL_K 0x0B
#define KEY_CTRL_N 0x0E
#define KEY_CTRL_O 0x0F
#define KEY_CTRL_Q 0x11
#define KEY_CTRL_S 0x13
#define KEY_CTRL_V 0x16
#define KEY_CTRL_W 0x17
#define KEY_CTRL_X 0x18

#ifdef __C64__

#define LEFT_CURLY_BRACE  0xAB
#define RIGHT_CURLY_BRACE 0xB3
#define TEXT_BACKSLASH    0xBF

//#define KEY_ESC    0x03 On C64, ESC and Ctrl-C give the same PETSCII key code
#define KEY_F1         0x85
#define KEY_F3         0x86
#define KEY_F5         0x87
#define KEY_F7         0x88
#define KEY_F2         0x89
#define KEY_F4         0x8A
#define KEY_F6         0x8B
#define KEY_F8         0x8C
#define KEY_BACKSPACE  0x14
#define KEY_LEFT       0x9D
#define KEY_UP         0x91
#define KEY_DOWN       0x11
#define KEY_RIGHT      0x1D
#define KEY_HOME       0x13
#define KEY_END        0x5F
#define KEY_PGDN       0x5E
#else

#define LEFT_CURLY_BRACE  '{'
#define RIGHT_CURLY_BRACE '}'
#define TEXT_BACKSLASH    '\\'

#define KEY_F1         0x13B
#define KEY_F2         0x13C
#define KEY_F3         0x13D
#define KEY_F4         0x13E
#define KEY_F5         0x13F
#define KEY_F6         0x140
#define KEY_F7         0x141
#define KEY_F8         0x142
#define KEY_F9         0x143
#define KEY_F10        0x144
#define KEY_ESC        0x1B
#define KEY_BACKSPACE  0x08
#define KEY_LEFT       0x14B
#define KEY_UP         0x148
#define KEY_DOWN       0x150
#define KEY_RIGHT      0x14D
#define KEY_HOME       0x147
#define KEY_END        0x14f
#define KEY_PGUP       0x149
#define KEY_PGDN       0x151
#define KEY_CTRL_HOME  0x177
#define KEY_CTRL_END   0x175
#define KEY_CTRL_LEFT  0x173
#define KEY_CTRL_RIGHT 0x174
#define KEY_CTRL_F4    0x161
#define KEY_ALT_F4     0x16B

#endif

#define KEY_INSERT     0x152
#define KEY_SHIFT_INSERT (KEY_SHIFT_MODIFIER | KEY_INSERT)
#define KEY_DELETE     0x153
#define KEY_SHIFT_DELETE (KEY_SHIFT_MODIFIER | KEY_DELETE)
#define KEY_CTRL_UP    0x18d
#define KEY_CTRL_MINUS 0x18E
#define KEY_CTRL_PLUS  0x190
#define KEY_CTRL_DOWN  0x191
#define KEY_CTRL_INSERT 0x192
#define KEY_CTRL_DELETE 0x193
#define KEY_CTRL_TAB    0x194
#define KEY_ENTER      0x0D
#define KEY_TAB        0x09
#define KEY_ALT_TAB    0x1A5
#define KEY_SHIFT_TAB  0x10F

bool is_shift_key_currently_down();
bool is_ctrl_key_currently_down();
bool is_alt_key_currently_down();

bool is_get_key_pending();
int get_key(); // block to wait for a received key
int get_key_and_poll_network(); // block to wait for a key, but pump network transfers while at it

bool is_shift_key_held(int key);
int is_shift_aware_key(int key);
int is_navigation_action_key(int key);

void init_keyboard();
void deinit_keyboard();

// Returns -1 if cancelled (ESC), or the length of entered string.
int ask_string_prompt(const char *prompt, char *dst, int maxLen);

// Returns 1 if yes, 0 if no
int ask_yes_no_prompt(const char *prompt);

#endif
