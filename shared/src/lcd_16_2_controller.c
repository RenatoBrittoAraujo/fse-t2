#include <wiringPiI2C.h>
#include <wiringPi.h>

int fd; // seen by all subroutines

// #ifdef __arm__

// #else

// #define LCD_BACKLIGHT 0

// int wiringPiSetup() { return 0; }
// int wiringPiI2CSetup(int addr) { return 0; }

// void wiringPiI2CReadReg8(int fd, int reg)
// {
//   printf("wiringPiI2CReadReg8\n");
// }

// void wiringPiI2CWriteReg8(int fd, int reg, int data)
// {
//   printf("wiringPiI2CWriteReg8\n");
// }

// #endif

int initialized = 0;

#include <stdlib.h>
#include <stdio.h>
#include <shared/inc/lcd_16_2_controller.h>
#include <shared/inc/time.h>
#include <shared/inc/shared_util.h>

// int main()
// {

//   if (wiringPiSetup() == -1)
//     exit(1);

//   fd = wiringPiI2CSetup(I2C_ADDR);

//   // printf("fd = %d ", fd);

//   lcd_init(); // setup LCD

//   char array1[] = "Hello world!";

//   while (1)
//   {

//     lcdLoc(LINE1);
//     typeln("Using wiringPi");
//     lcdLoc(LINE2);
//     typeln("Geany editor.");

//     wait_micro(2000);
//     ClrLcd();
//     lcdLoc(LINE1);
//     typeln("I2c  Programmed");
//     lcdLoc(LINE2);
//     typeln("in C not Python.");

//     wait_micro(2000);
//     ClrLcd();
//     lcdLoc(LINE1);
//     typeln("Arduino like");
//     lcdLoc(LINE2);
//     typeln("fast and easy.");

//     wait_micro(2000);
//     ClrLcd();
//     lcdLoc(LINE1);
//     typeln(array1);

//     wait_micro(2000);
//     ClrLcd(); // defaults LINE1
//     typeln("Int  ");
//     int value = 20125;
//     typeInt(value);

//     wait_micro(2000);
//     lcdLoc(LINE2);
//     typeln("Float ");
//     float FloatVal = 10045.25989;
//     typeFloat(FloatVal);
//     wait_micro(2000);
//   }

//   return 0;
// }

void init()
{
  if (initialized == 0)
  {
    initialized = 1;
    if (wiringPiSetup() == -1)
    {
      log_print("[lcd 16 2 controller] [init] failed to initialized wiringPi\n", LEVEL_ERROR);
      exit(1);
    }
    fd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init(); // setup LCD
  }
}

// float to string
void typeFloat(float myFloat)
{
  init();
  char buffer[20];
  sprintf(buffer, "%4.2f", myFloat);
  typeln(buffer);
}

// int to string
void typeInt(int i)
{
  init();
  char array1[20];
  sprintf(array1, "%d", i);
  typeln(array1);
}

// clr lcd go home loc 0x80
void ClrLcd(void)
{
  init();
  lcd_byte(0x01, LCD_CMD);
  lcd_byte(0x02, LCD_CMD);
}

// go to location on LCD
void lcdLoc(int line)
{
  init();
  lcd_byte(line, LCD_CMD);
}

// out char to LCD at current position
void typeChar(char val)
{
  init();
  lcd_byte(val, LCD_CHR);
}

// this allows use of any size string
void typeln(const char *s)
{
  init();
  while (*s)
    lcd_byte(*(s++), LCD_CHR);
}

void lcd_byte(int bits, int mode)
{
  init();
  // Send byte to data pins
  //  bits = the data
  //  mode = 1 for data, 0 for command
  int bits_high;
  int bits_low;
  // uses the two half byte writes to LCD
  bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
  bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

  // High bits
  wiringPiI2CReadReg8(fd, bits_high);
  lcd_toggle_enable(bits_high);

  // Low bits
  wiringPiI2CReadReg8(fd, bits_low);
  lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)
{
  init();
  // Toggle enable pin on LCD display
  wait_micro(500);
  wiringPiI2CReadReg8(fd, (bits | ENABLE));
  wait_micro(500);
  wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
  wait_micro(500);
}

void lcd_init()
{
  init();
  // Initialise display
  lcd_byte(0x33, LCD_CMD); // Initialise
  lcd_byte(0x32, LCD_CMD); // Initialise
  lcd_byte(0x06, LCD_CMD); // Cursor move direction
  lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
  lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
  lcd_byte(0x01, LCD_CMD); // Clear display
  wait_micro(500);
}