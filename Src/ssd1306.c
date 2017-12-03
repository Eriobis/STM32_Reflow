
#include"ssd1306.h"


// Databuffer voor het scherm
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Een scherm-object om lokaal in te werken
static SSD1306_t SSD1306;


//
//	Een byte sturen naar het commando register
//	Kan niet gebruikt worden buiten deze file
//
static void ssd1306_WriteCommand(uint8_t command)
{
  HAL_I2C_Mem_Write(&hi2c1,SSD1306_I2C_ADDR,0x00,1,&command,1,10);
}

#define swap(a, b)       { int16_t t = a; a = b; b = t; }

//
//	Het scherm initialiseren voor gebruik
//
uint8_t ssd1306_Init(void)
{
  // Even wachten zodat het scherm zeker opgestart is
  HAL_Delay(100);

  /* Init LCD */
  ssd1306_WriteCommand(0xAE); //display off
  ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
  ssd1306_WriteCommand(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
  ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
  ssd1306_WriteCommand(0x00); //---set low column address
  ssd1306_WriteCommand(0x10); //---set high column address
  ssd1306_WriteCommand(0x40); //--set start line address
  ssd1306_WriteCommand(0x81); //--set contrast control register
  ssd1306_WriteCommand(0xFF);
  ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127
  ssd1306_WriteCommand(0xA6); //--set normal display
  ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64)
  ssd1306_WriteCommand(0x3F); //
  ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  ssd1306_WriteCommand(0xD3); //-set display offset
  ssd1306_WriteCommand(0x00); //-not offset
  ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
  ssd1306_WriteCommand(0xF0); //--set divide ratio
  ssd1306_WriteCommand(0xD9); //--set pre-charge period
  ssd1306_WriteCommand(0x22); //
  ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration
  ssd1306_WriteCommand(0x12);
  ssd1306_WriteCommand(0xDB); //--set vcomh
  ssd1306_WriteCommand(0x20); //0x20,0.77xVcc
  ssd1306_WriteCommand(0x8D); //--set DC-DC enable
  ssd1306_WriteCommand(0x14); //
  ssd1306_WriteCommand(0xAF); //--turn on SSD1306 panel

  /* Clearen scherm */
  ssd1306_Fill(Black);

  /* Update screen */
  ssd1306_UpdateScreen();

  /* Set default values */
  SSD1306.CurrentX = 0;
  SSD1306.CurrentY = 0;

  /* Initialized OK */
  SSD1306.Initialized = 1;

  /* Return OK */
  return 1;
}

//
//	We zetten de hele buffer op een bepaalde kleur
// 	color 	=> de kleur waarin alles moet
//
void ssd1306_Fill(SSD1306_COLOR color)
{
  /* Set memory */
  uint32_t i;

  for(i = 0; i < sizeof(SSD1306_Buffer); i++)
  {
    SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
  }
}

//
//	Alle weizigingen in de buffer naar het scherm sturen
//
void ssd1306_UpdateScreen(void)
{
  uint8_t i;

  for (i = 0; i < 8; i++) {
    ssd1306_WriteCommand(0xB0 + i);
    ssd1306_WriteCommand(0x00);
    ssd1306_WriteCommand(0x10);

    // We schrijven alles map per map weg
    HAL_I2C_Mem_Write(&hi2c1,SSD1306_I2C_ADDR,0x40,1,&SSD1306_Buffer[SSD1306_WIDTH * i],SSD1306_WIDTH,100);
  }
}

//
//	1 pixel op het scherm tekenen
//	X => X coordinaat
//	Y => Y coordinaat
//	color => kleur die pixel moet krijgen
//
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    // We gaan niet buiten het scherm schrijven
    return;
  }

  // Kijken of de pixel geinverteerd moet worden
  if (SSD1306.Inverted)
  {
    color = (SSD1306_COLOR)!color;
  }

  // We zetten de juiste kleur voor de pixel
  if (color == White)
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  }
  else
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}

//
//	We willen 1 char naar het scherm sturen
//	ch 		=> char om weg te schrijven
//	Font 	=> Font waarmee we gaan schrijven
//	color 	=> Black or White
//
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color)
{
  uint32_t i, b, j;

  // Kijken of er nog plaats is op deze lijn
  if (SSD1306_WIDTH <= (SSD1306.CurrentX + Font.FontWidth) ||
    SSD1306_HEIGHT <= (SSD1306.CurrentY + Font.FontHeight))
  {
    // Er is geen plaats meer
    return 0;
  }

  // We gaan door het font
  for (i = 0; i < Font.FontHeight; i++)
  {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for (j = 0; j < Font.FontWidth; j++)
    {
      if ((b << j) & 0x8000)
      {
        ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
      }
      else
      {
        ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
      }
    }
  }

  // De huidige positie is nu verplaatst
  SSD1306.CurrentX += Font.FontWidth;

  // We geven het geschreven char terug voor validatie
  return ch;
}

//
//	Functie voor het wegschrijven van een hele string
// 	str => string om op het scherm te schrijven
//	Font => Het font dat gebruikt moet worden
//	color => Black or White
//
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color)
{
  // We schrijven alle char tot een nulbyte
  while (*str)
  {
    if (ssd1306_WriteChar(*str, Font, color) != *str)
    {
      // Het karakter is niet juist weggeschreven
      return *str;
    }

    // Volgende char
    str++;
  }

  // Alles gelukt, we sturen dus 0 terug
  return *str;
}

/**
 *  \brief Brief
 *
 *  \param [in] x0 Parameter_Description
 *  \param [in] y0 Parameter_Description
 *  \param [in] x1 Parameter_Description
 *  \param [in] y1 Parameter_Description
 *  \param [in] color Parameter_Description
 *  \return Return_Description
 *
 *  \details Details
 */
void ssd1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t steep = (abs(y1 - y0) > abs(x1 - x0));
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx            = x1 - x0;
  dy            = abs(y1 - y0);

  int16_t err   = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep       = 1;
  } else {
    ystep       = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      ssd1306_DrawPixel(y0, x0, color);
    } else {
      ssd1306_DrawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

/**************************************************************************/
/*!
    @brief Draws a filled rectangle
*/
/**************************************************************************/
void ssd1306_DrawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

    uint8_t *pbitmap;
    int16_t i, j, byteWidth = (w + 7) / 8;

    for(j=0; j<h; j++) {
      for(i=0; i<w; i++ ) {
        pbitmap = bitmap + j * byteWidth + i / 8;
        if((uint8_t)(*pbitmap) & (128 >> (i & 7))) {
          ssd1306_DrawPixel(x+i, y+j, color);
        }
      }
    }
  }

/**************************************************************************/
/*!
    @brief Draws a rectangle
*/
/**************************************************************************/
void ssd1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	int16_t x1, y1;
	if ( (w == 0) | (h == 0)) return;
	x1 = x + w - 1;
	y1 = y + h - 1;

	if((h > 2 ) | (w > 2)) {
		ssd1306_DrawLine(	 x,    y,   x1,  y, color);
		ssd1306_DrawLine(	 x,   y1,   x1, y1, color);
		ssd1306_DrawLine(	 x,  y+1,  x, y1-1, color);
		ssd1306_DrawLine(  x1,  y+1, x1, y1-1, color);
	} else {
		ssd1306_DrawLine(	 x,    y,   x1,  y, color);
		ssd1306_DrawLine(	 x,   y1,   x1, y1, color);
	}
}

/**************************************************************************/
/*!
    @brief Draws a filled rectangle
*/
/**************************************************************************/
void ssd1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
	uint8_t x0, x1, y1;

	x0 = x;
	x1 = x + w;
	y1 = y + h;
	for(; y < y1; y++)
			for(x = x0; x < x1; x++)
					ssd1306_DrawPixel( x, y, color);
}

  /**************************************************************************/
/*!
    @brief Draw a character
*/
/**************************************************************************/
void  ssd1306DrawChar(int16_t x, int16_t y, uint8_t c, uint8_t size, int16_t color)
{
    if( (x >= SSD1306_WIDTH)            || // Clip right
        (y >= SSD1306_HEIGHT)           || // Clip bottom
        ((x + 6 * size - 1) < 0) || // Clip left
        ((y + 8 * size - 1) < 0))   // Clip top
    return;

    for (int8_t i=0; i<6; i++ )
    {
        int8_t line;
        if (i == 5)
            line = 0x0;
        else
            line = (int8_t)*(font5x7+(c*5)+i);

        for (int8_t j = 0; j<8; j++)
        {
            if (line & 0x1) {
                if (size == 1) // default size
                ssd1306_DrawPixel(x+i, y+j, color);
                else {  // big size
                ssd1306_DrawRect(x+(i*size), y+(j*size), size, size, color);
                }
            }
            line >>= 1;
        }
    }
}

  /**************************************************************************/
/*!
    @brief  Draws a string using the supplied font data.
    @param[in]  x
                Starting x co-ordinate
    @param[in]  y
                Starting y co-ordinate
    @param[in]  text
                The string to render
    @param[in]  font
                Pointer to the FONT_DEF to use when drawing the string
    @section Example
    @code
    #include "drivers/lcd/bitmap/ssd1306/ssd1306.h"
    #include "drivers/lcd/smallfonts.h"

    // Configure the pins and initialise the LCD screen
    ssd1306Init();
    // Render some text on the screen
    ssd1306DrawString(1, 10, "5x8 System", Font_System5x8);
    ssd1306DrawString(1, 20, "7x8 System", Font_System7x8);
    // Refresh the screen to see the results
    ssd1306Refresh();
    @endcode
*/
/**************************************************************************/
void  ssd1306DrawString(int16_t x, int16_t y, const char *text, uint8_t size, uint16_t color)
{
    uint8_t l;
    for (l = 0; l < strlen(text); l++)
    {
       ssd1306DrawChar(x + (l * (5*size + 1)), y, text[l], size, color);
    }
}

//
//	Zet de cursor op een coordinaat
//
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
  /* Set write pointers */
  SSD1306.CurrentX = x;
  SSD1306.CurrentY = y;
}
