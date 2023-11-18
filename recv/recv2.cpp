#include "nfio.h"
#include "../c64/c64_kernal.h"
#include "../c64/c64_cursor.h"
#include "../c64/c64_char_rom.h"
#include <string.h>

static volatile unsigned char * const screen_border = (unsigned char*)0xD020;
static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;
static volatile unsigned char * const color_mem = (unsigned char*)0xD800;

uint8_t SCREEN_WIDTH = 40;
uint8_t SCREEN_HEIGHT = 25;

#define BLACK       0
#define WHITE       1
#define RED         2
#define CYAN        3
#define MAGENTA     4
#define GREEN       5
#define BLUE        6
#define YELLOW      7
#define ORANGE      8
#define BROWN       9
#define LTRED      10
#define BLACK_GRAY 11
#define DKGRAY     12
#define LTGREEN    13
#define LTBLUE     14
#define LTGRAY     15

static unsigned char petscii_to_screen_code(unsigned char petscii)
{
  if (petscii == 0x0D) petscii = 0xFF; // Route newline to 0xFF
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
}

static void put_screen(int screen_pos, char ch, unsigned char color, uint8_t highlight)
{
  screen_mem[screen_pos] = petscii_to_screen_code(ch) ^ highlight;
  color_mem[screen_pos] = color;
}

uint8_t get_key()
{
  uint8_t key = 0;
  while(!key) key = _GETIN();
  return key;
}

void clear_line(uint16_t screenPos, uint8_t ch, uint8_t color)
{
  for(uint8_t i = 0; i < SCREEN_WIDTH; ++i)
    put_screen(screenPos + i, ch, color, 0);
}

void put_string_on_screen(const char *str, uint8_t x, uint8_t y, uint8_t color)
{
  uint16_t pos = (uint16_t)y*SCREEN_WIDTH + x;
  for(uint8_t i = 0; str[i] != 0; ++i, ++pos)
    put_screen(pos, str[i], color, 0);
}

int ask_string_prompt(const char *prompt, char *dst, int maxLen)
{
  clear_line((uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH, ' ', LTGRAY); // Clear last line to start with

  put_string_on_screen(prompt, 0, SCREEN_HEIGHT-1, WHITE);
  uint8_t prompt_len = (uint8_t)strlen(prompt);
  uint8_t x = (uint8_t)strlen(dst);
  put_string_on_screen(dst, prompt_len, SCREEN_HEIGHT-1, LTGRAY);
  const uint16_t pos = (uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH+prompt_len;
  int8_t result = 0;
  while(result == 0)
  {
    int key = get_key();
    switch(key)
    {
    case 0x5F: result = -1; break;
    case 0x0D: if (x > 0) result = 1; break;
    case 0x14:
      if (x > 0)
      {
        dst[--x] = 0;
        put_screen(pos+x, ' ', LTGRAY, 0);
        c64_set_cursor_pos(prompt_len+x, SCREEN_HEIGHT-1);
      }
      break;
    default:
      if (x < maxLen && key >= 0x20 && key < 127)
      {
        if (maxLen > 1 || key == 'y' || key == 'Y' || key == 'n' || key == 'N')
        {
          c64_set_cursor_pos(prompt_len+x+1, SCREEN_HEIGHT-1);
          put_screen(pos+x, key, LTGRAY, 0);
          dst[x++] = key;
        }
      }
      break;
    }
  }
  dst[x] = 0;
  if (result < 0) return -1;
  return x;
}

// If run_prg == true, run the PRG. If false, save the PRG to Datasette.
extern "C" void receive_and_run_prg(bool run_prg);

int ask_yes_no_prompt(const char *prompt)
{
  char dst[2] = {0};
  int ret = ask_string_prompt(prompt, dst, 1);
  return ret > 0 && (dst[0] == 'y' || dst[0] == 'Y');
}

int main()
{
  deploy_restore_overlay_memory();
  for(;;)
  {
    if (_KBHIT() && _GETIN() == 0x5F/*ESC*/) break; // Check quit

    _CLRSCR();

    char device[8] = {};
    char filename[MAX_C64_FILENAME_LEN+1] = {};
    ask_string_prompt("SAVE TO DEVICE #: ", device, 2);
    uint8_t device_num = atoi(device);

    if (device_num == 1)
    {
      bool save_prg = ask_yes_no_prompt("SAVE TO PRG? ");
      if (save_prg)
      {
        printf("\n"); // Add a newline so that the next question appears on a new line
        ask_string_prompt("ENTER FILENAME: ", filename, MAX_C64_FILENAME_LEN);
        _SETNAM(filename, strlen(filename));
        _SETLFS(1, 1, 1);
        printf("\n");
        printf("WAITING TO RECEIVE FILE (ESC QUITS)...\n");
        rrnet_init(false);
        receive_and_run_prg(false);
        put_string_on_screen("WRITE COMPLETE.", 0, SCREEN_HEIGHT-1, WHITE);
        if (get_key() == 0x5F/*ESC*/) break; // Check quit
        continue;
      }
    }
    printf("\n");
    printf("WAITING TO RECEIVE FILE (ESC QUITS)...\n");
    NFILE *f = nfopen("", "r");
    if (!f) continue;

    strcat(device, "w");
    bool prg_file = (f->header.filetype == 'P');
    if (prg_file) strcat(device, "p");
    strncpy(filename, f->header.filename, MAX_C64_FILENAME_LEN);
    printf("\n"); // Add a newline so that the next question appears on a new line
    ask_string_prompt("ENTER FILENAME: ", filename, MAX_C64_FILENAME_LEN);
    printf("\n"); // Add a newline so that the potential text "PRESS RECORD & PLAY ON TAPE" will appear on a line of its own
/*
    if (device_num == 1)
    {
      char *buf = (char*)malloc(f->header.file_size);
      nfread(buf, f->header.file_size, 1, f);
      for(int i = 0; i < f->header.file_size; ++i)
        printf("%c", buf[i]);
      _SETNAM(filename, strlen(filename));
      _SETLFS(1, 1, 1);
      _SAVE(buf, buf + f->header.file_size);
      free(buf);
    }
    else
    {
*/
      FILE *dst = fopen(filename, device);
      _CLRSCR();
      printf("START ADDRESS: $%X\n", f->header.file_start_address);
      if (prg_file)
      {
        if (device_num >= 8)
        {
          fputc((uint8_t)f->header.file_start_address, dst);
          fputc((uint8_t)(f->header.file_start_address>>8), dst);
        }
        /*
        struct
        {
          uint8_t header_type;
          uint16_t start_address;
          uint16_t end_address;
  //        uint8_t filename[12];
          //uint8_t filename_hidden[171]; // filename is 12+171 = 183 bytes total
        } __attribute__((packed)) tape_prg_header;
        tape_prg_header.header_type = 3;
        tape_prg_header.start_address = f->header.file_start_address;
        tape_prg_header.end_address = tape_prg_header.start_address + f->header.file_size;
        fwrite(&tape_prg_header, sizeof(tape_prg_header), 1, dst);
        fwrite(f->header.filename, 20, 1, dst);
        for(int i = 20; i < 183; ++i)
          fputc(' ', dst);
          */
      }
      for(;;)
      {
        int ch = nfgetc(f);
        if (ch < 0) break;
        *screen_border = ch;
        fputc(ch, dst);
        printf("%c", (unsigned char)ch);
      }
      fclose(dst);
//    }
    nfclose(f);
    put_string_on_screen("WRITE COMPLETE.", 0, SCREEN_HEIGHT-1, WHITE);
    if (get_key() == 0x5F/*ESC*/) break; // Check quit
  }
  _RESET();
}