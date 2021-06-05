#include "snow04.xbm"
#include "snow01.xbm"
#include "snow02.xbm"
#include "snow03.xbm"
//#include "snow05.xbm"
#include "welle.xbm"


#define SNOWFLAKEMAXTYPE 1
#define MAXSNOWFLAKES 120     /* Max no. of snowflakes */
#define INITIALSNOWFLAKES 100  /* Initial no. of snowflakes */
#define MAXYSTEP 40             /* falling speed max */
#define MAXXSTEP 4             /* drift speed max */
#define WHIRLFACTOR 1//4
#define INITWINSNOWDEPTH 15
#define INITIALSCRPAINTSNOWDEPTH 8  /* Painted in advance */
#define INITSCRSNOWDEPTH 50    /* Will build up to INITSCRSNOWDEPTH */
#define SNOWFREE 25  /* Must stay snowfree on display :) */

typedef struct Snow {
    int intX;
    int intY;
    int xStep;    /* drift */
    int yStep;    /* falling speed */
    int active;
    int visible;
    int insnow;
    int whatFlake;
} Snow;

typedef struct SnowMap {
	char *snowBits;
	Pixmap pixmap;
	int width;
	int height;
} SnowMap;

SnowMap snowPix[] = {
	{snow03_bits, None, snow03_width, snow03_height},
	{snow04_bits, None, snow04_width, snow04_height},
	{snow01_bits, None, snow01_width, snow01_height},
	{snow02_bits, None, snow02_width, snow02_height},
	{snow03_bits, None, snow03_width, snow03_height},
	{snow01_bits, None, snow01_width, snow01_height},
	{snow02_bits, None, snow02_width, snow02_height}
};

typedef struct WelleMap
{
  char *wellenBits;
  Pixmap pixmap;
  int width;
  int height;
} WelleMap;

typedef struct Welle
{
  int x;
  int y;
  int xStep;
  int yStep;
  int active;
  int visible;
} Welle;

WelleMap wellenPix = { welle_bits, None, welle_width, welle_height };
