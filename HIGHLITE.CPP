#include "iassert.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "line.h"
#include "highlite.h"
#include "screen.h"
#include "state.h"

#ifdef ENABLE_SYNTAX_HIGHLIGHTING

/* Keywords recognized by Borland Turbo C++ 3.0:
auto break case catch char class const continue default delete do double else
enum extern float for friend goto if inline int long new operator private
protected public register return short signed sizeof static struct switch
template this throw try typedef union unsigned virtual void volatile while */

static int strneq(const char huge *s, const char *s2, int n)
{
  for(int i = 0; i < n; ++i)
    if (s[i] != s2[i])
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
    if (*s == 'd') return E1("o");
    return E("if");
  case 3:
    if (*s == 'f') return E1("or");
    if (*s == 'i') return E1("nt");
    if (*s == 'n') return E1("ew");
    return E("try");
  case 4:
    if (*s == 'a') return E1("uto");
    if (*s == 'c') return E1("ase") || E1("har");
    if (*s == 'e') return E1("lse") || E1("num");
    if (*s == 'g') return E1("oto");
    if (*s == 'l') return E1("ong");
    if (*s == 't') return E1("this");
    return E("void");
  case 5:
    if (*s == 'b') return E1("reak");
    if (*s == 'c') return E1("atch") || E1("lass") || E1("onst");
    if (*s == 'f') return E1("loat");
    if (*s == 's') return E1("hort");
    if (*s == 't') return E1("hrow");
    if (*s == 'u') return E1("nion");
    return E("while");
  case 6:
    if (*s == 'd') return E1("elete") || E1("ouble");
    if (*s == 'e') return E1("xtern");
    if (*s == 'f') return E1("riend");
    if (*s == 'i') return E1("nline");
    if (*s == 'p') return E1("ublic");
    if (*s == 's') return E1("igned") || E1("izeof") || E1("tatic") || E1("truct") || E1("witch");
    return E("return");
  case 7:
    if (*s == 'd') return E1("efault");
    if (*s == 'p') return E1("rivate");
    if (*s == 't') return E1("ypedef");
    return E("virtual");
  case 8:
    if (*s == 'c') return E1("ontinue");
    if (*s == 'o') return E1("perator");
    if (*s == 'r') return E1("egister");
    if (*s == 't') return E1("emplate");
    if (*s == 'u') return E1("nsigned");
    return E("volatile");
  case 9:
    return E("protected");
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
  assert(s[1] == 'x'); // these are known from calling this function
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
  assert(line);
  int prevLineFlags = line->prevLine ? line->prevLine->colorFlags : 0;
  if (!line->colors)
  {
    if (!line->capacity) return;
    line->colors = (unsigned char far *)Farmalloc(line->capacity);
    line->colorFlags = 0;
  }
  else
  {
    if ((prevLineFlags >> 1) == line->colorFlags)
      return;
  }
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

  for(; i < line->length; ++i)
  {                                  //   % 'asdfasd'
    if (line->text[i] <= 32)          // !     ! () 0        :;<=>? @ |
      line->colors[i] = editorTextColor;// &  *+,-./  01 0x23 [ ~ \ ^ _ ` { }
    else if (isalnum(line->text[i]) || line->text[i] == '_')
    {
      size_t j = i+1;
      while(j < line->length && (isalnum((unsigned char)line->text[j]) || line->text[j] == '_'))
	++j;
      unsigned char color = LTGREEN | bg;
      if (line->text[i] == '0' && j-i>1)
      {
	if (line->text[i+1] == 'x') color = scan_hex_number(&line->text[i], j-i);
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
      line->colors[i++] = RED | bg;
      for(; i < line->length; ++i)
      {
	line->colors[i] = RED | bg;
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
