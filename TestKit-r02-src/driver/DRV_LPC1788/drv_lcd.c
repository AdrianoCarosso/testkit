// drv_spi.c - LCD1788 driver tasks

//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a LCD driver for LPC1788.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

#include <string.h>

//----------------------------------------------------------------------------
// only if we are well accepted
#ifdef USE_LCD1788

//----------------------------------------------------------------------------
// global values

unsigned short lcd1788_color ;

//----------------------------------------------------------------------------
// constant values

#define C_GLCD_PIX_CLK          (7500000)       // 7.5 MHz pixel clock

#define C_GLCD_H_SIZE           480
#define C_GLCD_H_PULSE          2       // range 1 - oo                         // it was 30
#define C_GLCD_H_BACK_PORCH     38      // PULSE+BACK: range 36 - 255 sugg.40   // it was 38
#define C_GLCD_H_FRONT_PORCH    5       // range 4 - 65 sugg.5                  // it was 20
#define C_GLCD_CLK_PER_LINE     (C_GLCD_H_SIZE + C_GLCD_H_PULSE + C_GLCD_H_FRONT_PORCH + C_GLCD_H_BACK_PORCH)
                                        // total clock per line: 525 -> 70us -> 14286 KHz

#define C_GLCD_V_SIZE           272
#define C_GLCD_V_PULSE          2       // range 1 - oo                         // it was 3
#define C_GLCD_V_BACK_PORCH     6       // PULSE+BACK: range 3 - 31 sugg.8      // it was 15
#define C_GLCD_V_FRONT_PORCH    8       // range 2 - 93 sugg.8                  // it was 5
#define C_GLCD_LINES_PER_FRAME  (C_GLCD_V_SIZE + C_GLCD_V_PULSE + C_GLCD_V_FRONT_PORCH + C_GLCD_V_BACK_PORCH)
                                        // total lines per frame: 288 -> 20.16 ms -> 49.6 Hz

#define LCD_VRAM_BASE_ADDR      ((uint32_t)(SDRAM_BASE+SDRAM_SIZE))
#define LCD_CURSOR_BASE_ADDR    ((uint32_t)(0x20088800))

#define DISP_ORIENTATION    0   // may be: 0 90 180 270 - only 0 verified

//----------------------------------------------------------------------------
// LCD initializer

void lcdstart(void)
{
    int i ;

    // enable it
    LPC_SC->PCONP |= CLKPWR_PCONP_PCLCD ;

    // pins are already configured by dio.c

    // Disable cursor
    LPC_LCD->CRSR_CTRL &= ~(1<<0) ;

    // disable GLCD controller
    LPC_LCD->CTRL = 0 ;
    // 16 bpp - 5:6:5 mode
    LPC_LCD->CTRL |= (6<<1) ;
    // TFT panel
    LPC_LCD->CTRL |= (1<<5) ;
    // single panel
    // LPC_LCD->CTRL &= ~(1<<7);
    // notmal output
    // LPC_LCD->CTRL &= ~(1<<8);
    LPC_LCD->CTRL |= (1<<8) ;   // swap R and B
    // little endian byte order
    // LPC_LCD->CTRL &= ~(1<<9);
    // little endian pix order
    // LPC_LCD->CTRL &= ~(1<<10);
    // disable power
    // LPC_LCD->CTRL &= ~(1<<11);

    // init pixel clock
    LPC_SC->LCD_CFG = (current_clock / C_GLCD_PIX_CLK) - 1 ;

    // bypass internal clk divider
    LPC_LCD->POL |=(1<<26) ;
    // clock source for the LCD block is CCLK - internal clock
    LPC_LCD->POL &= ~(1<<5) ;
    // LCDFP pin is active LOW and inactive HIGH
    LPC_LCD->POL |= (1<<11) ;
    // LCDLP pin is active LOW and inactive HIGH
    LPC_LCD->POL |= (1<<12) ;
    // data is driven out into the LCD on the falling edge
    LPC_LCD->POL |= (1<<13) ;
    // active high ENAB
    LPC_LCD->POL &= ~(1<<14) ;

    LPC_LCD->POL &= ~(0x3FF <<16);
    LPC_LCD->POL |= (C_GLCD_H_SIZE-1)<<16;

    // init Horizontal Timing
    LPC_LCD->TIMH = 0; //reset TIMH before set value
    LPC_LCD->TIMH |= (C_GLCD_H_BACK_PORCH - 1)<<24;
    LPC_LCD->TIMH |= (C_GLCD_H_FRONT_PORCH - 1)<<16;
    LPC_LCD->TIMH |= (C_GLCD_H_PULSE - 1)<<8;
    LPC_LCD->TIMH |= ((C_GLCD_H_SIZE/16) - 1)<<2;

    // init Vertical Timing
    LPC_LCD->TIMV = 0;  //reset TIMV value before setting
    LPC_LCD->TIMV |= (C_GLCD_V_BACK_PORCH)<<24;
    LPC_LCD->TIMV |= (C_GLCD_V_FRONT_PORCH)<<16;
    LPC_LCD->TIMV |= (C_GLCD_V_PULSE - 1)<<10;
    LPC_LCD->TIMV |= C_GLCD_V_SIZE - 1;

    // Frame Base Address doubleword aligned
    LPC_LCD->UPBASE = LCD_VRAM_BASE_ADDR & ~7UL ;
    LPC_LCD->LPBASE = LCD_VRAM_BASE_ADDR & ~7UL ;

    // clear display memory
    memset((void *)(LCD_VRAM_BASE_ADDR), 0, 2 * C_GLCD_H_SIZE * C_GLCD_V_SIZE) ;

    // LCD_CTRL_bit.LcdEn = 1;
    LPC_LCD->CTRL |= (1<<0) ;
    for(i=0 ; i<10 ; i++) tickwait(1000) ;  // wait 10ms

    // LCD_CTRL_bit.LcdPwr= 1;   // enable power
    LPC_LCD->CTRL |= (1<<11);

    lcd1788_color = 0xffff ;    // default white
}

//----------------------------------------------------------------------------
// LCD terminator

void lcdstop(void)
{
    int i ;

    // LCD_CTRL_bit.LcdPwr= 0;   // disable power
    LPC_LCD->CTRL &= ~(1<<11);
    for(i=0 ; i<10 ; i++) tickwait(1000) ;  // wait 10ms

    // LCD_CTRL_bit.LcdEn = 0;
    LPC_LCD->CTRL &= ~(1<<0);

    // disable it
    LPC_SC->PCONP &= ~CLKPWR_PCONP_PCLCD ;
}

//----------------------------------------------------------------------------
// write single pixel with current color

void lcd1788_PutPixel(int x, int y)
{
    int address ;

    // Coordinates transaltion to the address of controller
    #if (DISP_ORIENTATION == 0)
        address = LCD_VRAM_BASE_ADDR + 2 * ((y * C_GLCD_H_SIZE) + x) ;
    #elif (DISP_ORIENTATION == 90)
        address = LCD_VRAM_BASE_ADDR + 2 * ((x * C_GLCD_V_SIZE) + (C_GLCD_V_SIZE-1 - y)) ;
    #elif (DISP_ORIENTATION == 180)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - y) * C_GLCD_H_SIZE) + (C_GLCD_V_SIZE-1 - x)) ;
    #elif (DISP_ORIENTATION == 270)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - x) * C_GLCD_V_SIZE) + y) ;
    #endif

    *((unsigned short *)(address)) = lcd1788_color ;
}

//----------------------------------------------------------------------------
// write area with current color

void lcd1788_PutArea(int x, int y, int sizex, int sizey, unsigned char *tofill)
{
    unsigned long mask, offs, i, j ;
    int address ;

    // Coordinates transaltion to the address of controller
    #if (DISP_ORIENTATION == 0)
        address = LCD_VRAM_BASE_ADDR + 2 * ((y * C_GLCD_H_SIZE) + x) ;
    #elif (DISP_ORIENTATION == 90)
        address = LCD_VRAM_BASE_ADDR + 2 * ((x * C_GLCD_V_SIZE) + (C_GLCD_V_SIZE-1 - y)) ;
    #elif (DISP_ORIENTATION == 180)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - y) * C_GLCD_H_SIZE) + (C_GLCD_V_SIZE-1 - x)) ;
    #elif (DISP_ORIENTATION == 270)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - x) * C_GLCD_V_SIZE) + y) ;
    #endif

    // check if area flood or pattern fill
    if (tofill) {   // some real area

        for(i=0 ; i<sizey ; i++) {
            mask = 0x80 ;
            offs = i * 2 * C_GLCD_H_SIZE ;
            for(j=0 ; j<sizex ; j++) {
                if (*tofill & mask) {
                    // set foreground color
                    *((unsigned short *)(address + offs)) = lcd1788_color ;
                } else {
                    // set background color
                    *((unsigned short *)(address + offs)) = 0 ;
                }
                if (mask == 1) {
                    mask = 0x80 ;
                    tofill++ ;
                } else {
                    mask >>= 1 ;
                }
                offs += 2 ;
            }
            if (mask != 0x80) tofill++ ;
        }

    } else {

        for(i=0 ; i<sizey ; i++) {
            offs = i * 2 * C_GLCD_H_SIZE ;
            for(j=0 ; j<sizex ; j++) {
                *((unsigned short *)(address + offs)) = lcd1788_color ;
                offs += 2 ;
            }
        }

    }
}

//----------------------------------------------------------------------------
// write text with current color

void lcd1788_PutText(int x, int y, char *txt, const struct FONT_DEF * font)
{
    int i ;

    while(*txt) {
        i = (1+(font->u8Width - 1)/8) * font->u8Height * ( (*txt) - font->u8FirstChar ) ;
        lcd1788_PutArea(x, y, font->u8Width, font->u8Height, (unsigned char *)(&font->au8FontTable[i])) ;
        x += font->u8Width ;
        txt++ ;
    }
}

//----------------------------------------------------------------------------
// write coloured area

void lcd1788_PutColouredArea(int x, int y, int sizex, int sizey, unsigned char *area, int revmask)
{
    int address, offs ;
    register int i, j ;
    unsigned short sval ;

    // Coordinates transaltion to the address of controller
    #if (DISP_ORIENTATION == 0)
        address = LCD_VRAM_BASE_ADDR + 2 * ((y * C_GLCD_H_SIZE) + x) ;
    #elif (DISP_ORIENTATION == 90)
        address = LCD_VRAM_BASE_ADDR + 2 * ((x * C_GLCD_V_SIZE) + (C_GLCD_V_SIZE-1 - y)) ;
    #elif (DISP_ORIENTATION == 180)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - y) * C_GLCD_H_SIZE) + (C_GLCD_V_SIZE-1 - x)) ;
    #elif (DISP_ORIENTATION == 270)
        address = LCD_VRAM_BASE_ADDR + 2 * (((C_GLCD_H_SIZE-1 - x) * C_GLCD_V_SIZE) + y) ;
    #endif

    for(i=0 ; i<sizey ; i++) {
        offs = i * 2 * C_GLCD_H_SIZE ;
        for(j=0 ; j<sizex ; j++) {
            //sval = area[0] | (area[1]<<8) ;     // set pixel color
            sval = *((unsigned short *)(area)) ;  // set pixel color
            area += 2 ;
            if (sval) sval ^= revmask ;
            *((unsigned short *)(address + offs)) = sval ;
            offs += 2 ;
        }
    }
}


#endif // USE_LCD1788
