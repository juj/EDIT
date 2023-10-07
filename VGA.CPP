#include "vga.h"
#include "memory.h"

#if defined(__TURBOC__) && defined(__MSDOS__)

#define VIDEO_INTERRUPT 0x10

unsigned char read_CRTC_subreg(int subRegIndex)
{
  disable();
  outp(0x3D4, subRegIndex);
  unsigned char val = inp(0x3D5);
  enable();
  return val;
}

void write_CRTC_subreg(int subRegIndex, unsigned char newValue)
{
  disable();
  unsigned int val = (((unsigned int)newValue) << 8) | subRegIndex;
  outpw(0x3D4, val);
  enable();
}

void write_SEQ_subreg(int subRegIndex, unsigned char newValue)
{
  disable();
  unsigned int val = (((unsigned int)newValue) << 8) | subRegIndex;
  outpw(0x3C4, val);
  enable();
}

void dump_CRTC()
{
  clrscr();
  unsigned int reg0 = (unsigned int)read_CRTC_subreg(0);
  unsigned int reg1 = (unsigned int)read_CRTC_subreg(1);
  printf("0h: Horizontal total: %u (+5=%u) \n", reg0, reg0 + 5);
  printf("1h: Horizontal display enable end: %u (# char clocks displayed=+1: %u) \n", reg1, reg1+1);
  printf("2h: Start horizontal blank: %u \n", (unsigned int)read_CRTC_subreg(2));
  unsigned int reg3 = (unsigned int)read_CRTC_subreg(3);
  printf("3h: End horizontal blank: %u (bits 0-4), whole reg: %u \n", reg3 & 31, reg3);
  printf("4h: Start horizontal retrace: %u \n", (unsigned int)read_CRTC_subreg(4));
  printf("5h: End horizontal retrace: %u (bits 0-4), whole reg: %u \n", (unsigned int)read_CRTC_subreg(5) & 31, (unsigned int)read_CRTC_subreg(5));
  unsigned int reg6 = (unsigned int)read_CRTC_subreg(6);
  unsigned int reg7 = (unsigned int)read_CRTC_subreg(7);
  unsigned int vertTotal = reg6 | ((reg7&1) << 8) | ((reg7&16)<<9);
  printf("6h: Vertical total: %u (reg value=%u) \n", vertTotal, reg6);
  printf("7h: Overflow: %u \n", reg7);
  unsigned int reg9 = (unsigned int)read_CRTC_subreg(9);
  printf("9h: Maximum scan line: %u \n", reg9);
  printf("10h: Start vertical retrace: %u \n", (unsigned int)read_CRTC_subreg(0x10));
  printf("11h: End vertical retrace: %u \n", (unsigned int)read_CRTC_subreg(0x11));
  printf("12h: Vertical display enable end: %u \n", (unsigned int)read_CRTC_subreg(0x12));
  printf("13h: Offset: %u \n", (unsigned int)read_CRTC_subreg(0x13));
  printf("15h: Start vertical blank: %u \n", (unsigned int)read_CRTC_subreg(0x15));
  printf("16h: End vertical blank: %u \n", (unsigned int)read_CRTC_subreg(0x16));
}

// height must be one of 8, 14 or 16.
void set_vga_text_mode(int mode)
{
  unsigned int videoMode = 0x03;
  unsigned char characterMode = 0;
  switch(mode)
  {
  case TEXT_MODE_90x60: characterMode = 0x12; SCREEN_WIDTH = 90; SCREEN_HEIGHT = 60; break;
  case TEXT_MODE_80x50: characterMode = 0x12; SCREEN_WIDTH = 80; SCREEN_HEIGHT = 50; break;
  case TEXT_MODE_80x28: characterMode = 0x11; SCREEN_WIDTH = 80; SCREEN_HEIGHT = 28; break;
  case TEXT_MODE_80x25: characterMode = 0x14; SCREEN_WIDTH = 80; SCREEN_HEIGHT = 25; break;
  case TEXT_MODE_40x50: videoMode = 0x01; characterMode = 0x12; SCREEN_WIDTH = 40; SCREEN_HEIGHT = 50; break;
  case TEXT_MODE_40x28: videoMode = 0x01; characterMode = 0x11; SCREEN_WIDTH = 40; SCREEN_HEIGHT = 28; break;
  case TEXT_MODE_40x25: videoMode = 0x01; characterMode = 0x14; SCREEN_WIDTH = 40; SCREEN_HEIGHT = 25; break;
  default:
    ERROR_BEEP();
    assert(0);
    return;
  }
  disable();
  union REGS regs = {0};
  regs.x.ax = videoMode;
  int86(VIDEO_INTERRUPT, &regs, &regs);

  /*
  regs.x.ax = 0x4F02;
  for(int i = 0; i < 0x200; ++i)
  {
  regs.x.bx = i;
  int86(VIDEO_INTERRUPT, &regs, &regs);
  if (regs.h.ah == 0)
  {
  SCREEN_WIDTH = 80;
  SCREEN_HEIGHT = 60;
  break;
  }
  else
  {
  ERROR_BEEP();
  }
  }
  */
  regs.h.ah = 0x11;
  regs.h.al = characterMode;
  regs.h.bl = 0;
  int86(VIDEO_INTERRUPT, &regs, &regs);

  /*
  disable();

  // Specify 8 pixels per character mode
  outpw(0x3C4, 0x0100); // Start sequencer reset by clearing bit 1
  outp(0x3C4, 0x01); // Choose to read from Sequencer sub-register 1.
  unsigned int x = inp(0x3C5); // read sub-register
  x = (x << 8) | 0x0101;
  outpw(0x3C4, x); // write sub-register 1 to set bit 0.
  outpw(0x3C4, 0x0300); // Finish sequencer reset by setting bit 1

  enable();
  */

  /* Programming the VGA's sequecer to produce 8 pixels per character
  asm cli
  asm mov dx, 3C4h
  asm mov ax, 0100h
  asm out dx, ax
  asm mov al, 1
  asm out dx, al
  asm inc dx
  asm in al, dx
  asm dec dx
  asm or al,1
  asm mov ah, al
  asm mov al,1
  asm out dx,ax
  asm mov ax, 0300h
  asm out dx,ax
  asm sti*/


  //  dump_CRTC();
  //  getch();
  if (mode == TEXT_MODE_90x60)
  {
    // Write MISC register
    outp(0x3C2, 0xe7);

    write_SEQ_subreg(0, 0x03);
    write_SEQ_subreg(1, 0x01);
    write_SEQ_subreg(2, 0x03);
    write_SEQ_subreg(3, 0x00);
    write_SEQ_subreg(4, 0x02);

    write_CRTC_subreg(0x11, read_CRTC_subreg(0x11)&0x7f);
    write_CRTC_subreg(0, 107);  // horizontal total-4: default=95
    write_CRTC_subreg(1, 89);   // horizontal display enable end: def=79
    write_CRTC_subreg(2, 90);   // hblank start: def=80
    write_CRTC_subreg(3, 128 | 2);  // hblank end (bits 0-4): def=2 (AND 0x3F)
    write_CRTC_subreg(4, 95/*96*/);   // hretrace start: def=85
    write_CRTC_subreg(5, 128 | 12/*13*/);  // hretrace end (bits 0-4): def=129 (AND 0x1F)
    write_CRTC_subreg(6, 0x0b);
    write_CRTC_subreg(7, 0x3e);
    write_CRTC_subreg(8, 0x00);
    write_CRTC_subreg(9, 0x47);
    write_CRTC_subreg(10, 0x06);
    write_CRTC_subreg(11, 0x07);
    write_CRTC_subreg(12, 0x00);
    write_CRTC_subreg(13, 0x00);
    write_CRTC_subreg(14, 0x00);
    write_CRTC_subreg(15, 0x00);
    write_CRTC_subreg(16, 0xea);
    write_CRTC_subreg(17, 0x0c);
    write_CRTC_subreg(18, 0xdf);
    write_CRTC_subreg(19, 0x2d);
    write_CRTC_subreg(20, 0x08);
    write_CRTC_subreg(21, 0xe8);
    write_CRTC_subreg(22, 0x05);
    write_CRTC_subreg(23, 0xa3);
    write_CRTC_subreg(24, 0xff);

    //  dump_CRTC();
    //  getch();

    unsigned int far *biosMem = (unsigned int far *)MK_FP(0x0040, 0);
    biosMem[37] = 90;
    biosMem[38] = 90*60*2;
    for(int i = 0; i < 8; ++i)
      biosMem[40+i] = 0;
    biosMem[48] = 0x0607;
    biosMem[0x84/2] = 59;
    biosMem[0x85/2] = 8;
  }
}

struct SvgaInfoRec
{
  unsigned char vesaSig[4]; // string "VESA"
  unsigned char versionLo; // VESA version number low
  unsigned char versionHi; // VESA version number high
  unsigned char far *pfszOEMStr; // Manufacturer ID
  unsigned long abilities; // bitflags
  unsigned short far *supportedModes;
  unsigned char modeData[238];
};

struct SvgaModeInfoRec
{
  unsigned short attributes;
  unsigned char winAttrs[2];
  unsigned short winGranularityKB;
  unsigned short winSizeKB;
  unsigned short winSegments[2];
  unsigned long windowFnPtr;
  unsigned short bytesPerScanline;

  // Extended info:
  unsigned short xRes, yRes;
  unsigned char charWidth, charHeight;
  unsigned char planeCount;
  unsigned char bitsPerPixel;
  unsigned char bankCount;
  unsigned char memModel;
  unsigned char bankSize;
  unsigned char reserved[227];
};

const char *svga_memmodel_to_string(unsigned int memmodel)
{
  static const char *memmodels[] = { "Text", "CGA", "Hercules",
    "4-plane planar", "Packed pixel", "non-chain 4, 256 color",
    "VESA reserved", "OEM defined" };

  if (memmodel <= 5) return memmodels[memmodel];
  if (memmodel < 0x0f) return memmodels[6];
  return memmodels[7];
}

void query_svga_modes()
{
  SREGS sregs;
  segread(&sregs);
  union REGS regs = {0};
  regs.x.ax = 0x4f00;

  unsigned char far *f = (unsigned char far *)farmalloc(256);
  unsigned char far *f2 = (unsigned char far *)farmalloc(256);
  sregs.es = FP_SEG(f);
  regs.x.di = FP_OFF(f);

  int86x(VIDEO_INTERRUPT, &regs, &regs, &sregs);

  if (regs.x.ax == 0x004f)
  {
    SvgaInfoRec far *s = (SvgaInfoRec far *)f;
    if (s->vesaSig[0] == 'V' && s->vesaSig[1] == 'E' && s->vesaSig[2] == 'S' && s->vesaSig[3] == 'A')
    {
      printf("VESA Version: %d.%d\n", (int)s->versionHi, (int)s->versionLo);
      printf("Manufacturer: %s\n", s->pfszOEMStr);
      printf("Supported modes:\n");
      int np = 0;
      for(unsigned short far *modes = s->supportedModes; *modes != 0xFFFF; ++modes)
      {

        memset(&regs, 0, sizeof(REGS));
        regs.x.ax = 0x4f01;
        regs.x.cx = *modes;
        segread(&sregs);
        sregs.es = FP_SEG(f2);
        regs.x.di = FP_OFF(f2);
        int86x(VIDEO_INTERRUPT, &regs, &regs, &sregs);
        if (regs.x.ax == 0x004f)
        {
          SvgaModeInfoRec far *m = (SvgaModeInfoRec far *)f2;
          if ((m->attributes & 1) != 0)
          {
            char *color = (m->attributes & 8) ? "COL":"MON";
            char *graph = (m->attributes & 16) ? "GFX":"TXT";
            printf("0x%X: %s %s winSize: %uKB, gran: %uKB, scanlineSize: %ub \n",
              (unsigned int)*modes, color, graph, (unsigned int)m->winSizeKB, (unsigned int)m->winGranularityKB, (unsigned int)m->bytesPerScanline);
            for(int i = 0; i < 2; ++i)
              if ((m->winAttrs[i] & 1) != 0) printf("Window %d: %s%s, seg: 0x%X ", i,
                (m->winAttrs[i] & 2) ? "READ ": "",
                (m->winAttrs[i] & 4) ? "WRITE ": "",
                (unsigned int)m->winSegments[i]);
            printf("\n");
            if ((m->attributes & 2) != 0)
            {
              printf("Res: %ux%u, charSz: %ux%u, %u planes, %u banks (%uKB each), bpp: %u, memmodel: %s\n",
                (unsigned int)m->xRes, (unsigned int)m->yRes,
                (unsigned int)m->charWidth, (unsigned int)m->charHeight,
                (unsigned int)m->planeCount, (unsigned int)m->bankCount,
                (unsigned int)m->bankSize, (unsigned int)m->bitsPerPixel,
                svga_memmodel_to_string(m->memModel));
            }
            printf("\n");
            if (++np >= 4)
            {
              np = 0;
              getch();
              clrscr();
            }
          }
        }
      }
    }
    else printf("Signature mismatch!\n");
  }
  else
    printf("Failed!\n");
  farfree(f);
  farfree(f2);
  getch();
}
#endif
