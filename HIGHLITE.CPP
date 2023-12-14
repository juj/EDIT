#include "iassert.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "line.h"
#include "highlite.h"
#include "screen.h"
#include "state.h"
#include "file.h"

#ifdef ENABLE_SYNTAX_HIGHLIGHTING

static bool syntax_highlight_disabled = false;

void disable_syntax_highlighting()
{
  if (syntax_highlight_disabled) return;
  syntax_highlight_disabled = true;
  // Free all memory used by color highlighting
  for(File *f = currentFile; f; f = f->nextFile)
    for(Line *l = f->firstLine; l; l = l->nextLine)
      clear_highlight(l);
}

/* Keywords recognized by Borland Turbo C++ 3.0:
auto break case catch char class const continue default delete do double else
enum extern float for friend goto if inline int long new operator private
protected public register return short signed sizeof static struct switch
template this throw try typedef union unsigned virtual void volatile while */

static int strneq(const char huge *s, const char *s2, int n)
{
  for(int i = 0; i < n; ++i)
#ifdef __C64__
    if (s[i] != (s2[i] ^ 0x20))
#else
    if (s[i] != s2[i])
#endif
      return 0;
  return 1;
}

static int is_one_of(char ch, const char *s)
{
  while(*s)
    if (ch == *s++)
      return 1;
  return 0;
}

#define E(str) (strneq(s, str, len))
#define E1(str) (strneq(s1, str, len1))

#ifdef __C64__
#define CH(x) ((x) ^ 0x20) // Convert string literal a-z from ASCII to business PETSCII
#else
#define CH(x) (x)
#endif

int is_cpp_reserved_word(const char huge *s, const char huge *end)
{
  assert(s);
  assert(end);
  const int len = (int)(end-s);
  const int len1 = len-1;
  const char huge * const s1 = s+1;
  switch(len)
  {
  case 2:
    if (*s == CH('d')) return E1("o");
    if (*s == CH('i')) return E1("8") || E1("f");
    return E("u8");
  case 3:
    if (*s == CH('i')) return E1("16") || E1("24") || E1("32") || E1("64") || E1("nt");
    if (*s == CH('u')) return E1("16") || E1("24") || E1("32") || E1("64");
//disabled for code size////    if (*s == CH('n')) return E1("ew");
    if (*s == CH('t')) return E1("ry");
    return E("for");
  case 4:
//disabled for code size////    if (*s == CH('a')) return E1("uto");
    if (*s == CH('b')) return E1("ool");
    if (*s == CH('c')) return E1("ase") || E1("har");
    if (*s == CH('e')) return E1("lse") || E1("num");
    if (*s == CH('g')) return E1("oto");
    if (*s == CH('l')) return E1("ong");
//disabled for code size////    if (*s == CH('t')) return E1("this");
    return E("void");
  case 5:
    if (*s == CH('b')) return E1("reak");
    if (*s == CH('c')) return E1("onst") || E1("lass");//disabled for code size//// || E1("atch");
//disabled for code size////    if (*s == CH('f')) return E1("loat");
    if (*s == CH('s')) return E1("hort");
//disabled for code size////    if (*s == CH('t')) return E1("hrow");
    if (*s == CH('u')) return E1("nion");
    return E("while");
  case 6:
//disabled for code size////    if (*s == CH('d')) return E1("elete") || E1("ouble");
//disabled for code size////    if (*s == CH('e')) return E1("xtern");
    //disabled for code size////if (*s == CH('f')) return E1("riend");
//disabled for code size////    if (*s == CH('i')) return E1("nline");
//disabled for code size////    if (*s == CH('p')) return E1("ublic");
    if (*s == CH('s')) return E1("witch") || E1("izeof") || E1("truct");//disabled for code size//// || E1("tatic") || E1("igned");
    return E("return");
  case 7:
    if (*s == CH('d')) return E1("efault");
//disabled for code size////    if (*s == CH('p')) return E1("rivate");
//disabled for code size////    if (*s == CH('v')) return E1("irtual");
    return E("typedef");
  case 8:
    if (*s == CH('c')) return E1("ontinue");
    if (*s == CH('o')) return E1("perator");
//disabled for code size////    if (*s == CH('r')) return E1("egister");
    if (*s == CH('t')) return E1("emplate");
    if (*s == CH('u')) return E1("nsigned");
    return E("volatile");
//disabled for code size////  case 9:
//disabled for code size////    return E("protected");
  default:
    return 0;
  }
}

void clear_highlight(Line far *line)
{
  assert(line);
  if (line->colors)
  {
    Farfree(line->colors, line->capacity);
    line->colors = 0;
  }
}

unsigned char scan_hex_number(const char huge *s, size_t len)
{
  assert(s[0] == '0');
  assert(s[1] == CH('x')); // these are known from calling this function
  for(size_t i = 2; i < len; ++i)
    if (!(s[i] >= '0' && s[i] <= '9')
     && !((s[i] | 0x20) >= 'a' && (s[i] | 0x20) <= 'f'))
     return BLACK | BGCOLOR(RED);
  return BLACK | (editorTextColor & 0xF0);
}

unsigned char scan_decimal_number(const char huge *s, size_t len)
{
  for(size_t i = 0; i < len; ++i)
//  for(; s != e; ++s)
    if (s[i] < '0' || s[i] > '9')
     return BLACK | BGCOLOR(RED);
  return LTGRAY | (editorTextColor & 0xF0);
}

unsigned char scan_octal_number(const char huge *s, size_t len)
{
  assert(s[0] == '0');
  for(size_t i = 1; i < len; ++i)
    if (s[i] < '0' || s[i] > '7')
     return BLACK | BGCOLOR(RED);
  return DKGRAY | (editorTextColor & 0xF0);
}

void syntax_highlight(Line far *line)
{
  if (syntax_highlight_disabled) return;
  assert(line);
  int prevLineFlags = line->prevLine ? line->prevLine->colorFlags : 0;
  if (!line->colors)
  {
    if (!line->capacity) return;
    line->colors = (unsigned char far *)Farmalloc(line->capacity);
    line->colorFlags = 0;
  }
  else if ((prevLineFlags >> 1) == line->colorFlags)
    return;
  line->colorFlags = prevLineFlags >> 1;

  int indentLength = find_line_indent_position(line);
  unsigned char bg = editorTextColor & 0xF0;

  size_t i = 0;

  if ((line->colorFlags & LINE_STARTS_IN_COMMENT) != 0)
  {
    int commentTerminated = 0;
    for(; i < line->length; ++i)
    {
      line->colors[i] =	CYAN | bg;
      if (line->text[i] == '*' && i+1 < line->length && line->text[i+1] == '/')
      {
	      line->colors[i+1] = CYAN | bg;
	      i += 2;
	      commentTerminated = 1;
	      break;
      }
    }
    if (!commentTerminated)
    {
      line->colorFlags |= LINE_ENDS_IN_COMMENT;
      return;
    }
  }
  assert(line->length <= MAX_CHARS_PER_LINE);
  for(; i < line->length; ++i)
  {                                      //   % 'asdfasd'
    if (is_whitespace(line->text[i]))    // !     ! () 0        :;<=>? @ |
      line->colors[i] = editorTextColor; // &  *+,-./  01 0x23 [ ~ \ ^ _ ` { }
    else if (is_alphanum(line->text[i]) || line->text[i] == '_')
    {
      size_t j = i+1;
      while(j < line->length && (is_alphanum(line->text[j]) || line->text[j] == '_'))
	      ++j;
      unsigned char color = LTGREEN | bg;
      if (line->text[i] == '0' && j-i>1)
      {
	      if (line->text[i+1] == CH('x')) color = scan_hex_number(&line->text[i], j-i);
	      else color = scan_octal_number(&line->text[i], j-i);
      }
      else if (line->text[i] >= '0' && line->text[i] <= '9')
	      color = scan_decimal_number(&line->text[i], j-i);
      else if (is_cpp_reserved_word(&line->text[i], &line->text[j]))
	      color = WHITE | bg;

      while(i != j)
	      line->colors[i++] = color;
      --i;
    }
    else if (line->text[i] == '/')
    {
      unsigned char nextCh = i + 1 < line->length ? line->text[i+1] : 0;
      if (nextCh == '/')
      {
	      for(; i < line->length; ++i)
	        line->colors[i] = CYAN | bg;
      }
      else if (nextCh == '*')
      {
	      line->colors[i++] = CYAN | bg;
	      int commentTerminated = 0;
	      for(; i < line->length; ++i)
	      {
	        line->colors[i] = CYAN | bg;
	        if (line->text[i] == '*' && i+1 < line->length && line->text[i+1] == '/')
	        {
	          line->colors[i+1] = CYAN | bg;
	          ++i;
	          commentTerminated = 1;
	          break;
	        }
	      }
	      if (!commentTerminated)
	      {
	        line->colorFlags |= LINE_ENDS_IN_COMMENT;
	        return;
	      }
      }
      else
	       line->colors[i] = YELLOW | bg;
    }
    else if (line->text[i] == '\'')
    {
      line->colors[i++] = MAGENTA | bg;
      for(; i < line->length; ++i)
      {
	       line->colors[i] = MAGENTA | bg;
	       if (line->text[i] == '\'')
	         break;
      }
    }
    else if (line->text[i] == '"')
    {
      line->colors[i++] = LTRED | bg;
      for(; i < line->length; ++i)
      {
	      line->colors[i] = LTRED | bg;
	      if (line->text[i] == '"')
	        break;
      }
    }
    else if (is_one_of(line->text[i], "$@\\`"))
      line->colors[i] = BGCOLOR(RED);
    else if (is_one_of(line->text[i], "!%&()*+,-.:;<=>?[]^{|}~"))
      line->colors[i] = YELLOW | bg;
    else if (line->text[i] == '#')
    {
#ifdef __C64__
#define COMMENT_COLOR (LTBLUE | BGCOLOR(CYAN))
#else
#define COMMENT_COLOR (BLUE | BGCOLOR(CYAN))
#endif
      unsigned char color = (i == indentLength) ? COMMENT_COLOR : (BLACK | BGCOLOR(RED));
      for(size_t j = i; j < line->length; ++j)
	       line->colors[j] = color;
      return;
    }
    else line->colors[i] = LTGREEN | bg;
  }
  line->colorFlags &= ~LINE_ENDS_IN_COMMENT;
}
#endif
