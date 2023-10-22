#pragma once
/*
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

#define LEFT_CURLY_BRACE  0xAB
#define RIGHT_CURLY_BRACE 0xB3
#define TEXT_BACKSLASH    0xBF

#define KEY_ESC        0x5F // Left Arrow key is ESC on C64
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
#define KEY_HOME       0x3313
#define KEY_END        0x3393
#define KEY_PGDN       0x5E
#define KEY_DELETE     0x153

#define KEY_INSERT     0x152
#define KEY_SHIFT_INSERT (KEY_SHIFT_MODIFIER | KEY_INSERT)
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
*/
#define KEY_SHIFT_MODIFIER    0x100
#define KEY_CMDR_MODIFIER     0x200
#define KEY_CTRL_MODIFIER     0x400
#define KEY_SCANCODE_MODIFIER 0x800
#define SCAN(x) ((x) | KEY_SCANCODE_MODIFIER)

//#define KEY_CMDR_DELETE  (SCAN(0x94) | MATRIX(0x00) | KEY_CMDR_MODIFIER)
//#define KEY_SHIFT_DELETE (SCAN(0x94) | MATRIX(0x00) | KEY_SHIFT_MODIFIER)

#define KEY_ESC             SCAN(0x39)
#define KEY_1               SCAN(0x38)
#define KEY_2               SCAN(0x3B)
#define KEY_3               SCAN(0x08)
#define KEY_4               SCAN(0x0B)
#define KEY_5               SCAN(0x10)
#define KEY_6               SCAN(0x13)
#define KEY_7               SCAN(0x18)
#define KEY_8               SCAN(0x1B)
#define KEY_9               SCAN(0x20)
#define KEY_0               SCAN(0x23)
#define KEY_PLUS            SCAN(0x28)
#define KEY_MINUS           SCAN(0x2B)
#define KEY_GBP             SCAN(0x30)
#define KEY_HOME_CLR        SCAN(0x33)
#define KEY_DEL_INST        SCAN(0x00)
#define KEY_Q               SCAN(0x3E)
#define KEY_W               SCAN(0x09)
#define KEY_E               SCAN(0x0E)
#define KEY_R               SCAN(0x11)
#define KEY_T               SCAN(0x16)
#define KEY_Y               SCAN(0x19)
#define KEY_U               SCAN(0x1E)
#define KEY_I               SCAN(0x21)
#define KEY_O               SCAN(0x26)
#define KEY_P               SCAN(0x29)
#define KEY_AT              SCAN(0x2E)
#define KEY_ASTERISK        SCAN(0x2A)
#define KEY_UP_ARROW        SCAN(0x36)
#define KEY_RUN_STOP        SCAN(0x3F)
#define KEY_A               SCAN(0x0A)
#define KEY_S               SCAN(0x0D)
#define KEY_D               SCAN(0x12)
#define KEY_F               SCAN(0x15)
#define KEY_G               SCAN(0x1A)
#define KEY_H               SCAN(0x1D)
#define KEY_J               SCAN(0x22)
#define KEY_K               SCAN(0x25)
#define KEY_L               SCAN(0x2A)
#define KEY_COLON           SCAN(0x2D)
#define KEY_SEMICOLON       SCAN(0x32)
#define KEY_EQUALS          SCAN(0x35)
#define KEY_RETURN          SCAN(0x01)
#define KEY_Z               SCAN(0x0C)
#define KEY_X               SCAN(0x17)
#define KEY_C               SCAN(0x14)
#define KEY_V               SCAN(0x1F)
#define KEY_B               SCAN(0x1C)
#define KEY_N               SCAN(0x27)
#define KEY_M               SCAN(0x24)
#define KEY_COMMA           SCAN(0x2F)
#define KEY_PERIOD          SCAN(0x2C)
#define KEY_FORWARD_SLASH   SCAN(0x37)
#define KEY_CRSR_UP_DOWN    SCAN(0x07)
#define KEY_CRSR_LEFT_RIGHT SCAN(0x02)
#define KEY_SPACE           SCAN(0x3C)
#define KEY_F1_F2           SCAN(0x04)
#define KEY_F3_F4           SCAN(0x05)
#define KEY_F5_F6           SCAN(0x06)
#define KEY_F7_F8           SCAN(0x03)

// Use some proxy characters in shifted PETSCII charmap
// for some characters needed in C/C++ programming.
#define PETSCII_GREAT_BRITAIN_POUND 0x5C
#define PETSCII_LEFT_CURLY_BRACE    0xAB
#define PETSCII_RIGHT_CURLY_BRACE   0xB3
#define PETSCII_OR_PIPE             0xDD
#define PETSCII_THICK_UNDERSCORE    0xAF
#define PETSCII_THIN_UNDERSCORE     0xA4
#define PETSCII_BACKSLASH           0xAE

// General character mappings:
#define LEFT_CURLY_BRACE  PETSCII_LEFT_CURLY_BRACE
#define RIGHT_CURLY_BRACE PETSCII_RIGHT_CURLY_BRACE
































