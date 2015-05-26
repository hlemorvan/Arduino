/*
 * Output NTSC video using the UART in SPI master mode.
 * 40x25 character buffer is rasterized and scanned out in real-time.
 * 320X200 pixels out of 1000 RAM bytes and 1K Flash character ROM.
 *
 * Blatantly using code from:
 * TVout                 - http://playground.arduino.cc/Main/TVout
 * VGAout                - http://www.gammon.com.au/forum/?id=11608
 * Color TVout using SPI - http://www.hackster.io/janost/avr-videoblaster
 *
 * Dave Schmenk
 */
//
// Timing settings for NTSC
//
#define _CYCLES_PER_US          (F_CPU / 1000000)
#define _TIME_HSYNC             4.7
#define _TIME_VSYNC             58.85
#define _TIME_ACTIVE            46
#define _CYCLES_VSYNC           ((_TIME_VSYNC * _CYCLES_PER_US) - 1)
#define _CYCLES_HSYNC           ((_TIME_HSYNC * _CYCLES_PER_US) - 1)
#define _NTSC_TIME_SCANLINE      63.55
#define _NTSC_TIME_OUTPUT_START  12
#define _NTSC_LINE_FRAME         262
#define _NTSC_LINE_START_VSYNC   0
#define _NTSC_LINE_STOP_VSYNC    3
#define _NTSC_LINE_DISPLAY       216
#define _NTSC_LINE_MID           ((_NTSC_LINE_FRAME - _NTSC_LINE_DISPLAY)/2 + _NTSC_LINE_DISPLAY/2 + _NTSC_LINE_STOP_VSYNC)
#define _NTSC_CYCLES_SCANLINE    ((_NTSC_TIME_SCANLINE * _CYCLES_PER_US) - 1)
#define _NTSC_CYCLES_OUTPUT_START ((_NTSC_TIME_OUTPUT_START * _CYCLES_PER_US) - 1)
//
// video = UART SPI TX
//
#define PORT_VID     PORTD
#define	DDR_VID      DDRD
#define	VID_PIN       1
#define XCK0_DDR  DDRD
#define XCK0             4
//
// sync = OC1A
//
#define PORT_SYNC    PORTB
#define DDR_SYNC     DDRB
#define SYNC_PIN     1
//
// video buffer
//
#define VID_WIDTH    40
#define VID_HEIGHT   25
#define CELL_WIDTH   8
#define CELL_HEIGHT  8
byte xpos=0;
byte ypos=0;
char videomem[VID_WIDTH*VID_HEIGHT];
//
// rendering values
//
char *videoptr = videomem;
int scanline=0;
void (*line_handler)(void) = &blank_line;
volatile byte vblank=0;
//
// character definitions
//
PROGMEM const byte charROM [1024] = {
0x00,0xF0,0x0F,0x00,0x00,0xFF,0xF0,0xF0,0x0F,0x0F,0x00,0x0F,0xF0,0xFF,0xFF,0xFF,0x80,0x01,0xFF,0x18,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0xFF,0x00,0x30,0x6C,0x6C,0x30,0x00,0x38,0x60,0x18,0x60,0x00,0x00,0x00,0x00,0x00,0x06,0x7C,0x30,0x78,0x78,0x1C,0xFC,0x38,0xFC,0x78,0x78,0x00,0x00,0x18,0x00,0x60,0x78,0x7C,0x30,0xFC,0x3C,0xF8,0xFE,0xFE,0x3C,0xCC,0x78,0x1E,0xE6,0xF0,0xC6,0xC6,0x38,0xFC,0x78,0xFC,0x78,0xFC,0xCC,0xCC,0xC6,0xC6,0xCC,0xFE,0x78,0xC0,0x78,0x10,0x00,0x30,0x00,0xE0,0x00,0x1C,0x00,0x38,0x00,0xE0,0x30,0x0C,0xE0,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x18,0xE0,0x76,0xAA,
0x00,0xF0,0x0F,0x00,0x00,0xFF,0xF0,0xF0,0x0F,0x0F,0x00,0x0F,0xF0,0xFF,0xFF,0xFF,0xE0,0x07,0xFF,0x18,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0xFF,0x00,0x78,0x6C,0x6C,0x7C,0xC6,0x6C,0x60,0x30,0x30,0x66,0x30,0x00,0x00,0x00,0x0C,0xC6,0x70,0xCC,0xCC,0x3C,0xC0,0x60,0xCC,0xCC,0xCC,0x30,0x30,0x30,0x00,0x30,0xCC,0xC6,0x78,0x66,0x66,0x6C,0x62,0x62,0x66,0xCC,0x30,0x0C,0x66,0x60,0xEE,0xE6,0x6C,0x66,0xCC,0x66,0xCC,0xB4,0xCC,0xCC,0xC6,0xC6,0xCC,0xC6,0x60,0x60,0x18,0x38,0x00,0x30,0x00,0x60,0x00,0x0C,0x00,0x6C,0x00,0x60,0x00,0x00,0x60,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x18,0x30,0xDC,0x55,
0x00,0xF0,0x0F,0x00,0x00,0xFF,0xF0,0xF0,0x0F,0x0F,0x00,0x0F,0xF0,0xFF,0xFF,0xFF,0xF8,0x1F,0xC3,0x18,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x3C,0x7E,0x00,0x78,0x6C,0xFE,0xC0,0xCC,0x38,0xC0,0x60,0x18,0x3C,0x30,0x00,0x00,0x00,0x18,0xCE,0x30,0x0C,0x0C,0x6C,0xF8,0xC0,0x0C,0xCC,0xCC,0x30,0x30,0x60,0xFC,0x18,0x0C,0xDE,0xCC,0x66,0xC0,0x66,0x68,0x68,0xC0,0xCC,0x30,0x0C,0x6C,0x60,0xFE,0xF6,0xC6,0x66,0xCC,0x66,0xE0,0x30,0xCC,0xCC,0xC6,0x6C,0xCC,0x8C,0x60,0x30,0x18,0x6C,0x00,0x18,0x78,0x60,0x78,0x0C,0x78,0x60,0x76,0x6C,0x70,0x0C,0x66,0x30,0xCC,0xF8,0x78,0xDC,0x76,0xDC,0x7C,0x7C,0xCC,0xCC,0xC6,0xC6,0xCC,0xFC,0x30,0x18,0x30,0x00,0xAA,
0x00,0xF0,0x0F,0x00,0x00,0xFF,0xF0,0xF0,0x0F,0x0F,0x00,0x0F,0xF0,0xFF,0xFF,0xFF,0xFE,0x7F,0xC3,0x18,0xFF,0xFF,0x1F,0x1F,0xF8,0xF8,0x1F,0xF8,0xFF,0xFF,0x3C,0x7E,0x00,0x30,0x00,0x6C,0x78,0x18,0x76,0x00,0x60,0x18,0xFF,0xFC,0x00,0xFC,0x00,0x30,0xDE,0x30,0x38,0x38,0xCC,0x0C,0xF8,0x18,0x78,0x7C,0x00,0x00,0xC0,0x00,0x0C,0x18,0xDE,0xCC,0x7C,0xC0,0x66,0x78,0x78,0xC0,0xFC,0x30,0x0C,0x78,0x60,0xFE,0xDE,0xC6,0x7C,0xCC,0x7C,0x70,0x30,0xCC,0xCC,0xD6,0x38,0x78,0x18,0x60,0x18,0x18,0xC6,0x00,0x00,0x0C,0x7C,0xCC,0x7C,0xCC,0xF0,0xCC,0x76,0x30,0x0C,0x6C,0x30,0xFE,0xCC,0xCC,0x66,0xCC,0x76,0xC0,0x30,0xCC,0xCC,0xD6,0x6C,0xCC,0x98,0xE0,0x00,0x1C,0x00,0x55,
0x00,0x00,0x00,0xF0,0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0xFF,0xFF,0xFF,0x0F,0xF0,0xFF,0xFE,0x7F,0xC3,0x18,0xFF,0xFF,0x1F,0x1F,0xF8,0xF8,0x1F,0xF8,0xFF,0xFF,0x7E,0x3C,0x00,0x30,0x00,0xFE,0x0C,0x30,0xDC,0x00,0x60,0x18,0x3C,0x30,0x00,0x00,0x00,0x60,0xF6,0x30,0x60,0x0C,0xFE,0x0C,0xCC,0x30,0xCC,0x0C,0x00,0x00,0x60,0x00,0x18,0x30,0xDE,0xFC,0x66,0xC0,0x66,0x68,0x68,0xCE,0xCC,0x30,0xCC,0x6C,0x62,0xD6,0xCE,0xC6,0x60,0xDC,0x6C,0x1C,0x30,0xCC,0xCC,0xFE,0x38,0x30,0x32,0x60,0x0C,0x18,0x00,0x00,0x00,0x7C,0x66,0xC0,0xCC,0xFC,0x60,0xCC,0x66,0x30,0x0C,0x78,0x30,0xFE,0xCC,0xCC,0x66,0xCC,0x66,0x78,0x30,0xCC,0xCC,0xFE,0x38,0xCC,0x30,0x30,0x18,0x30,0x00,0xAA,
0x00,0x00,0x00,0xF0,0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0xFF,0xFF,0xFF,0x0F,0xF0,0xFF,0xF8,0x1F,0xC3,0x18,0x00,0x18,0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x18,0x7E,0x3C,0x00,0x00,0x00,0x6C,0xF8,0x66,0xCC,0x00,0x30,0x30,0x66,0x30,0x30,0x00,0x30,0xC0,0xE6,0x30,0xCC,0xCC,0x0C,0xCC,0xCC,0x30,0xCC,0x18,0x30,0x30,0x30,0xFC,0x30,0x00,0xC0,0xCC,0x66,0x66,0x6C,0x62,0x60,0x66,0xCC,0x30,0xCC,0x66,0x66,0xC6,0xC6,0x6C,0x60,0x78,0x66,0xCC,0x30,0xCC,0x78,0xEE,0x6C,0x30,0x66,0x60,0x06,0x18,0x00,0x00,0x00,0xCC,0x66,0xCC,0xCC,0xC0,0x60,0x7C,0x66,0x30,0xCC,0x6C,0x30,0xD6,0xCC,0xCC,0x7C,0x7C,0x60,0x0C,0x34,0xCC,0x78,0xFE,0x6C,0x7C,0x64,0x30,0x18,0x30,0x00,0x55,
0x00,0x00,0x00,0xF0,0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0xFF,0xFF,0xFF,0x0F,0xF0,0xFF,0xE0,0x07,0xFF,0x18,0x00,0x18,0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x18,0xFF,0x18,0x00,0x30,0x00,0x6C,0x30,0xC6,0x76,0x00,0x18,0x60,0x00,0x00,0x30,0x00,0x30,0x80,0x7C,0xFC,0xFC,0x78,0x1E,0x78,0x78,0x30,0x78,0x70,0x30,0x30,0x18,0x00,0x60,0x30,0x78,0xCC,0xFC,0x3C,0xF8,0xFE,0xF0,0x3E,0xCC,0x78,0x78,0xE6,0xFE,0xC6,0xC6,0x38,0xF0,0x1C,0xE6,0x78,0x78,0xFC,0x30,0xC6,0xC6,0x78,0xFE,0x78,0x02,0x78,0x00,0x00,0x00,0x76,0xDC,0x78,0x76,0x78,0xF0,0x0C,0xE6,0x78,0xCC,0xE6,0x78,0xC6,0xCC,0x78,0x60,0x0C,0xF0,0xF8,0x18,0x76,0x30,0x6C,0xC6,0x0C,0xFC,0x1C,0x18,0xE0,0x00,0xAA,
0x00,0x00,0x00,0xF0,0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0xFF,0xFF,0xFF,0x0F,0xF0,0xFF,0x80,0x01,0xFF,0x18,0x00,0x18,0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x18,0xFF,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0xF0,0x1E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x55,
};
/*
 * Set up UART SPI master mode and timers for sync pulses
 */
void setup(void)
{
  cli();
  UBRR0 = 0; // must be zero before enabling the transmitter
  XCK0_DDR |= _BV(XCK0); // set XCK pin as output to enable master mode
  UCSR0C = _BV (UMSEL00) | _BV (UMSEL01);  // SPI master mode
  DDR_VID |= _BV(VID_PIN);
  DDR_SYNC |= _BV(SYNC_PIN);
  PORT_VID &= ~_BV(VID_PIN);
  PORT_SYNC |= _BV(SYNC_PIN);
  TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11); // inverted fast pwm mode on timer 2
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = _NTSC_CYCLES_SCANLINE;
  OCR1A = _CYCLES_HSYNC;
  OCR1B = _NTSC_CYCLES_OUTPUT_START - 79;
  TIMSK1 = _BV(OCIE1B);
  TIMSK0 = 0; // turn timer0 off!
  SMCR = _BV(SE); // allow IDLE sleep mode
  //
  // Clear the video buffer and print out sample text
  //
  chrout(12);
  sei();
  printstr("0123456789012345678901234567890123456789");
  printstr("\n             Video SPI Demo\n\n");
  for (byte i = 0; i < ' '; i++)
    videomem[VID_WIDTH*(VID_HEIGHT-1) + 4 + i] = i;
  for (byte i = ' '; i < 128; i++)
    chrout(i);
}
//
// Handle scanline timing
//
ISR(TIMER1_COMPA_vect)
{
  sei();
  asm("sleep\n"); // for clock synchronization during active lines
}
ISR(TIMER1_COMPB_vect)
{
  line_handler();
  scanline++;
}
//
// Inactive scanlines
//
void blank_line(void)
{
  if (scanline == _NTSC_LINE_STOP_VSYNC)
  {
    OCR1A = _CYCLES_HSYNC;
  }
  else if ( scanline == _NTSC_LINE_MID - (VID_HEIGHT*CELL_HEIGHT)/2)
  {
    TIMSK1 = _BV(OCIE1A) | _BV(OCIE1B);
    videoptr = videomem;
    line_handler = &active_line;
  }
  else if (scanline > _NTSC_LINE_FRAME)
  {
    OCR1A = _CYCLES_VSYNC;
    scanline = 0;
  }
}
//
// Active scanlines
//
void active_line(void)
{
  register char *charPtr;
  register int charCol;
  byte p ;
  static byte charRow = 0;  

  UDR0 = 0;
  UCSR0B = _BV(TXEN0); // this spits out an idle bit (white pixel) so move it off the left side of the screen
  charPtr = videoptr; 
  while ((UCSR0A & _BV (UDRE0)) == 0);
  UDR0 = 0;
  charCol = (int)&charROM + (charRow<<7); // overlap with some initialization
  while ((UCSR0A & _BV (UDRE0)) == 0);
  UDR0 = 0;
  while ((UCSR0A & _BV (UDRE0)) == 0);
  UDR0 = 0;
  while ((UCSR0A & _BV (UDRE0)) == 0);
  UDR0 = 0;
  p = VID_WIDTH;
  while ((UCSR0A & _BV (UDRE0)) == 0);
  UDR0 = 0;
  while (p--) // draw the character line
    UDR0 = pgm_read_byte_near(charCol + *charPtr++);
  while ((UCSR0A & _BV (TXC0)) == 0); // wait for pixels to shift out
  UCSR0B = 0;
  if (++charRow == CELL_HEIGHT)
  {
    charRow = 0;
    if (scanline == _NTSC_LINE_MID + (VID_HEIGHT*CELL_HEIGHT)/2)
    {
      TIMSK1 = _BV(OCIE1B);
      line_handler = &blank_line;
      vblank=1; // signal new frame
    }
    else
    {
      videoptr += VID_WIDTH;
    }
  }
}
/*
 * Character output routines.
 */
void chrout(char ascii)
{
  switch (ascii) {
    case 10:
      xpos=0;
      ypos++;
      if (ypos>=VID_HEIGHT) {
       scrollscr(); 
       xpos=0;
       ypos=VID_HEIGHT-1;
      }
      break;
    case 12:
      clearscr();
      xpos=0;
      ypos=0;
      break;      
    default:
    if (ascii >= ' ' && ascii < 128) {
      videomem[xpos+ypos*VID_WIDTH] = ascii;
      xpos++;
      if (xpos>=VID_WIDTH) {
       xpos=0;
       ypos++;
       if (ypos>=VID_HEIGHT) {
        scrollscr(); 
        xpos=0;
        ypos=VID_HEIGHT-1;
       }
      }
    }
   }  
}
void clearscr(void)
{
  for (int i = VID_WIDTH*VID_HEIGHT-1; i >= 0; --i)
    videomem[i] = ' ';
}
void scrollscr(void)
{
  for (unsigned int i=VID_WIDTH; i < VID_WIDTH*VID_HEIGHT; i++)
    videomem[i-VID_WIDTH]=videomem[i];
  for (unsigned int i=VID_WIDTH*VID_HEIGHT - VID_WIDTH; i < VID_WIDTH*VID_HEIGHT; i++)
    videomem[i]=' ';
}
void printstr(char *string)
{
 while(*string)
   chrout(*string++);
}
void gotoxy(int x, int y)
{
  if (x < 0) x = 0;
  if (x > VID_WIDTH-1) x = VID_WIDTH-1;
  if (y < 0) y = 0;
  if (y > VID_HEIGHT-1) y = VID_HEIGHT-1;
  xpos = x;
  ypos = y;
}
/*
 * Run main loop
 */
byte tab = 0;
void loop(void)
{
  byte i;

  if (vblank) // only update every frame
  {
    gotoxy(tab, 16);
    printstr("        ");
    if (++tab > 32)
      tab = 0;
    gotoxy(tab, 16);
    printstr("Arduino!");
    vblank=0;
  }
}







