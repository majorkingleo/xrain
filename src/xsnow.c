#define debug 0

#define VERSION "Xrain 0.2, July by Martin Oberzalek"

#ifdef VMS
#include <socket.h>
# if defined(__SOCKET_TYPEDEFS) && !defined(CADDR_T)
#  define CADDR_T
# endif
#endif


#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>

//#include "xteddy_color.xpm"

/**** V R O O T ****/
/* For vroot.h see the credits at the beginning of Xsnow */
/* include <X11/vroot.h>  // if vroot.h installed in /usr/include/X11 */
#include "vroot.h"  /* Use this if vroot.h installed in current directory */


#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <limits.h>

#if defined(__STDC__) || defined(VMS)
#include <stdlib.h>
#else
long strtol();
/*double strtod();*/
char *getenv();
#endif

char Copyright[] = "\nXrain\nCopyright 1999 by Martin Oberzalek , all rights reserved.\n";

#include "snowmap.h"

/***typedef unsigned long Pixel;***/
typedef int ErrorHandler();

#if !defined(GRAB_SERVER)
#define GRAB_SERVER	0
#endif

Display *display;
int screen;
Window rootWin;
int display_width, display_height;
int center_x, center_y;
GC gc;
char *display_name = NULL;
Pixel black, white;

int done = 0;
int eventBlock = 0;
int errorVal = 0;

int flake;
int SmoothWhirl = 0;
int Rudolf = 1;
int NoKeepSnow = 0;
int NoKeepSBot = 1;
int NoKeepSWin = 1;
int NoPopuphandling = 0;
int NoWind = 1;
unsigned int borderWidth = 0;
int SnowOffset = 0;
int UseFillForClear = 0;

// -- wellen
int NoWellen = 0;
int WellenNum;
int WellenStartHeight = 200;
int WellenHeight;
int MaxWasserHeight = 50;
Welle *wellen;
Bool wassergrow;
Bool StillRaining = True;

//-- teddy
#include "xteddy_color.xpm"

Bool teddy = False;
Pixmap background_pixmap, shape_pixmap;
GC TeddyGC;

Snow *snowflakes;
int MaxSnowFlakeHeight = 0;  /* Biggest flake */
int maxSnowflakes = INITIALSNOWFLAKES;
int curSnowflakes = 0;
long snowDelay = 50000; /* useconds */
int MaxXStep = MAXXSTEP;
int MaxYStep = MAXYSTEP;
int WhirlFactor = WHIRLFACTOR;
int MaxWinSnowDepth = INITWINSNOWDEPTH;
int MaxScrSnowDepth = INITSCRSNOWDEPTH;

//Santa Claus;   

/* The default Santa 0,1,2 */
//int SantaSize = 1;
//int SantaSpeed = -1;  /* Not initialized yet */


/* For building up snow at bottom of screen and windows */
Region Windows = NULL;
Region SnowCatchR = NULL;
Region SnowAllowedR = NULL;
Region SubR = NULL;
XRectangle AddRect;
int AddX,AddY;
int ClearX, ClearY;

/* Wind stuff */
int diff=0;
int wind = 0;
int direction=0;
int WindTimer=3;
int current_snow_height;
int geta=0;

/* Forward decls */
void Usage();
void SigHandler();
void SigHupHandler();
void InitSnowflake();
void UpdateSnowflake();
void DrawSnowflake();
void EraseSnowflake();
void PaintSnowAtBottom();
void uSsleep();
int CalcWindowTops();
Pixel AllocNamedColor();
void sig_alarm();
void EraseWelle(int i);
void DrawWelle(int i);
void initWellen();
void UpdateWellen();

/* Colo(u)rs */
char *snowColor = "steelblue";
char *slcColor = "black";
char *blackColor = "black";
char *redColor = "red";
char *whiteColor = "white";
char *bgColor = "none";
char *trColor = "chartreuse";
char *wellenColor ="lightblue";
/* The author thoroughly recommends a cup of tea */
/* with a dash of green Chartreuse. Jum!         */
char *greenColor = "chartreuse"; 
Pixel redPix;
Pixel whitePix;
Pixel greenPix;
Pixel blackPix;
Pixel snowcPix;
Pixel bgPix;
Pixel trPix;
Pixel slcPix;
Pixel wellencPix;

/* GC's */
GC SnowGC[SNOWFLAKEMAXTYPE+1];
GC ClearSnowGC;
GC WellenGC;
GC WasserGC;
//GC SleighGCs[3];
GC /*SantaGC,*/RudolfGC,FurGC;

#ifdef VMS
int
#else
void
#endif
main(ac, av)
int ac;
char *av[];
{
    XGCValues xgcv;
    int ax;
    char *arg;
    SnowMap *rp;
    WelleMap *wp;
    XEvent ev;
    int needCalc;
    int Exposed;
    int ConfigureNotified;
    int i; 
    int x,y;

    
    printf("%s\n",VERSION);

    /* Some people... */
    if (maxSnowflakes > MAXSNOWFLAKES) {
      fprintf(stderr,"xx Maximum number of snowflakes is %d\n", MAXSNOWFLAKES);
      exit(1);
    }

    /* Eskimo warning */
    if (strstr(snowColor,"yellow") != NULL)
      printf("\nWarning: don't eat yellow snow!\n\n");

    /* Seed random */
#ifdef VMS
    srand((int)time((unsigned long *)NULL));
#else
    srand((int)time((long *)NULL));
#endif


    /*
       Catch some signals so we can erase any visible snowflakes.
    */
    signal(SIGKILL, SigHandler);
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);
#if debug
    signal(SIGHUP, SigHupHandler);
#else
    signal(SIGHUP, SigHandler);
#endif

    /* Wind stuff */
    signal(SIGALRM, sig_alarm);
    alarm(WindTimer);


    /* Open X */
    display = XOpenDisplay(display_name);
    if (display == NULL) {
	if (display_name == NULL) display_name = getenv("DISPLAY");
	(void) fprintf(stderr, "%s: cannot connect to X server %s\n", av[0],
	    display_name ? display_name : "(default)");
	exit(1);
    }

    screen = DefaultScreen(display);
    rootWin = RootWindow(display, screen);
    black = BlackPixel(display, screen);
    white = WhitePixel(display, screen);

    display_width = DisplayWidth(display, screen);
    display_height = DisplayHeight(display, screen);
    center_x = display_width / 2;
    center_y = display_height / 2;

    /* No snow at all yet */
    current_snow_height = display_height;

    /* Do not let all of the display snow under */
    if (MaxScrSnowDepth> (display_height-SNOWFREE)) {
      printf("xx Maximum snow depth set to %d\n", display_height-SNOWFREE);
      MaxScrSnowDepth = display_height-SNOWFREE;
    }
    

    /*
        P I X M A P S 
    */

    /* Create the snowflake pixmaps */
    for (flake=0; flake<=SNOWFLAKEMAXTYPE; flake++) {
      rp = &snowPix[flake];
      rp->pixmap = XCreateBitmapFromData(display, rootWin,
            rp->snowBits, rp->width, rp->height);
      /* Whats the biggest flake? (used for erasing kept snow later) */
      if (rp->height > MaxSnowFlakeHeight) MaxSnowFlakeHeight = rp->height;
    }

    /* Allocate structures containing the coordinates and things */
    snowflakes = (Snow *)malloc(sizeof(Snow) * MAXSNOWFLAKES);

    // -- Create Welle
    if(!NoWellen)
      {
	wp = &wellenPix;
	wp->pixmap = XCreateBitmapFromData(display, rootWin,
				       wp->wellenBits,
				       wp->width,
				       wp->height);
	WellenNum = display_width / wellenPix.width + 2;

	wellen = (Welle *)malloc(sizeof(Welle) * WellenNum);
	wellencPix = AllocNamedColor(wellenColor, white);


      }
    initWellen();
       
    
    /* Allocate colors just once */
    redPix =   AllocNamedColor(redColor, black);
    whitePix = AllocNamedColor(whiteColor, white);
    greenPix = AllocNamedColor(greenColor, black);
    blackPix = AllocNamedColor(blackColor, black);
    snowcPix = AllocNamedColor(snowColor, white);   
    trPix = AllocNamedColor(trColor, black);
    slcPix = AllocNamedColor(slcColor, black);

    gc = XCreateGC(display, rootWin, 0, NULL);
    XGetGCValues(display, gc, 0, &xgcv);
    XSetForeground(display, gc, blackPix);
    XSetFillStyle(display, gc, FillStippled);

    /* Set the background color, if specified */
    if(strcmp(bgColor,"none") != 0) {
      bgPix = AllocNamedColor(bgColor, white);

      XSetWindowBackground(display, rootWin, bgPix);
      XClearWindow(display, rootWin);
      XFlush(display);

      /* If -solidbg not specified notify that this may improve things! */
      if (!UseFillForClear) {
        printf("\nNote: when using backgrounds of one color also specifying\n");
        printf("      -solidbg may greatly improve performance!\n");
      }
      else {
        ClearSnowGC = XCreateGC(display, rootWin, 0L, &xgcv);
        XCopyGC(display,gc,0,ClearSnowGC);
        XSetForeground(display,ClearSnowGC, bgPix);
      }
    }
    else if (UseFillForClear) {
      printf("\n");
      printf(
       "Sorry, -solidbg can only be specified with a background color (-bg)");
      printf("\n");
      exit(1);
    }
    
    /* Fill in GCs for snowflakes */
    for (flake=0; flake<=SNOWFLAKEMAXTYPE; flake++) {
      SnowGC[flake] = XCreateGC(display, rootWin, 0L, &xgcv);
      XCopyGC(display,gc,0,SnowGC[flake]);
      XSetStipple(display, SnowGC[flake], snowPix[flake].pixmap);
      XSetForeground(display,SnowGC[flake],snowcPix);
      XSetFillStyle(display, SnowGC[flake], FillStippled);
    }

    if(!NoWellen)
      {
	WellenGC = XCreateGC(display, rootWin, 0, &xgcv);
	XCopyGC(display, gc, 0, WellenGC);
	XSetStipple(display, WellenGC, wellenPix.pixmap);
	XSetForeground(display, WellenGC, wellencPix);
	XSetFillStyle(display, WellenGC, FillStippled);

	WasserGC = XCreateGC(display, rootWin, 0, &xgcv);
	XCopyGC(display, gc, 0, WasserGC);
	XSetForeground(display, WasserGC, wellencPix);
      }

    Windows = XCreateRegion();
    SnowCatchR = XCreateRegion();
    SnowAllowedR = XCreateRegion();

    /* The initial catch region: a little bit below the bottom of the screen */
    AddRect.x = 0;
    AddRect.y = display_height;
    AddRect.width = display_width - 1;
    AddRect.height = MaxYStep+MaxSnowFlakeHeight;
    if (!NoKeepSBot)
      XUnionRectWithRegion(&AddRect, SnowCatchR, SnowCatchR);

    /* Initialize all snowflakes */
    for (i=0; i<maxSnowflakes; i++) InitSnowflake(i);

    //    InitSanta();   

    /* Notify if part of the root window changed */
    XSelectInput(display, rootWin, ExposureMask | SubstructureNotifyMask);


    needCalc = 0;
    /***if (!NoKeepSnow) needCalc = CalcWindowTops();***/
    needCalc = CalcWindowTops();

    if(teddy)
      {
	XpmAttributes attr;
	unsigned long gcmask;
	
	attr.valuemask = XpmCloseness | XpmReturnPixels| XpmColormap |XpmDepth;
	attr.closeness = 65535;
	attr.colormap = DefaultColormap(display, screen);
	attr.depth = DefaultDepth(display, screen);
	XpmCreatePixmapFromData(display, rootWin, xteddy_color,
				&background_pixmap, &shape_pixmap,
				&attr);

	TeddyGC = XCreateGC(display, rootWin, 0, &xgcv);
	XCopyGC(display, gc, 0, TeddyGC);
	XSetClipMask(display, TeddyGC, background_pixmap);
      }
  

    /*
     *  M A I N   L O O P
     */
    while (!done) {

      /* X event ? */
      /* Handle all the expose events and redo CalcWindowTops after */
      Exposed = 0;
      ConfigureNotified = 0;
      while (XPending(display)) {
        XNextEvent(display, &ev);

        /* No need to do all this computing if we're not keeping the snow */
        if (!NoKeepSnow) {

          switch (ev.type) {

              case Expose:    
                
                  Exposed = 1;

                  /* 
                   * Subtract the exposed area from the SnowCatchR 
                   */
                  AddX = ev.xexpose.x - MaxSnowFlakeHeight;
                  AddY = ev.xexpose.y - MaxWinSnowDepth;
                  AddRect.height = ev.xexpose.height +  MaxWinSnowDepth;
                  AddRect.width = ev.xexpose.width + 2*MaxSnowFlakeHeight;
                  if (AddY < 0) {
                    AddRect.height = AddRect.height + AddY;
                    AddY = 0;
                  }
                  if ((AddY+AddRect.height) > display_height) {
                    AddRect.height = display_height - AddY;
                  }

                  AddRect.x = AddX;
                  AddRect.y = AddY;

                  SubR= XCreateRegion();
                    XUnionRectWithRegion(&AddRect, SubR,SubR);
                  
                    /* Clear the snow from screen */
                    AddY = AddY - MaxYStep-MaxSnowFlakeHeight;
                    AddRect.height = 
                      MaxWinSnowDepth+MaxYStep*2+MaxSnowFlakeHeight*2;
                    if (AddY < 0) {
                      AddRect.height = AddRect.height + AddY;
                      AddY = 0;
                    }
                    AddRect.y = AddY;
                    if ((AddY+AddRect.height) > display_height) {
                      AddRect.height = display_height - AddY;
                    }
                    XUnionRectWithRegion(&AddRect, SubR,SubR);
                    XClearArea(display, rootWin,
                               AddX,
                               AddY,
                               AddRect.width,              
                               AddRect.height,
                               False);

                    /* Subtract the erased snow from the SnowCatchR */
                    XSubtractRegion(SnowCatchR,SubR, SnowCatchR);
                  XDestroyRegion(SubR);
                 

#if debug
		  printf("exp %d %d %d %d\n",
			 AddX,AddY,AddRect.width,AddRect.height);
#endif 
 
		  needCalc = 1;
		  wassergrow = True;
                  break;


              case MapNotify:
              case UnmapNotify:
              case ConfigureNotify:
#if debug
printf("Configurenotify!\n");
#endif
                  needCalc = 1;
                  break;

          } /* switch */

        }  /* if (!NoKeepSnow) */
       
      }  /* while Xpending */

      /* If things have changed while we were away... */
      if (needCalc) needCalc = CalcWindowTops();


#if debug
/***
XClearWindow(display, rootWin);
XFlush(display);
XSetForeground(display,FurGC,trPix);
XSetFillStyle(display,FurGC, FillSolid);
XSetRegion(display,FurGC,SnowCatchR);
XFillRectangle(display,rootWin,FurGC,0,0,display_width,display_height);
XFlush(display);
***/
#endif


      /* Sleep a bit */
      uSsleep(snowDelay);

      /* 
       *  Update     
       */

      /* Snowflakes */
      if(StillRaining)
	for (i=0; i<maxSnowflakes; i++) UpdateSnowflake(i);

      // -- Wellen
      if(!NoWellen)
	{
	  static int i;
	  if( i > 3)
	    {
	      UpdateWellen();
	      i=0;
	    }
	  
	  i++;
	  
	  if(teddy)
	    {
	      XSetClipOrigin(display, TeddyGC, 100, 100);
	      XCopyArea(display, rootWin, rootWin,TeddyGC, 100, 100,
			teddy_width, teddy_height,0,0);
	    }
	  /*	  XSetTSOrigin(display, WasserGC, 0, 
	   *       WellenHeight + wellenPix.height);
	   *	  XFillRectangle(display, rootWin, WasserGC, 0, 
	   *	 WellenHeight + wellenPix.height,
	   *	 display_width, display_height);
	   */
	}

      

    }
    
    XDestroyRegion(Windows);
    XDestroyRegion(SnowCatchR);
    XDestroyRegion(SnowAllowedR);

    XClearWindow(display, rootWin);

    XCloseDisplay(display);


    exit(0);
}


#define USEPRT(msg) fprintf(stderr, msg)

void
Usage()
{
    USEPRT("Usage: xsnow [options]\n\n");
    USEPRT("Options:\n");
    USEPRT("       -display     <displayname>\n");
    USEPRT("       -sc          <snowcolor>\n");
    USEPRT("       -tc          <tree color>\n");
    USEPRT("       -bg          <background color>\n");
    USEPRT("       -solidbg     (Performance improvement!)\n");
    USEPRT("       -slc         <sleigh color>\n");
    USEPRT("       -snowflakes  <numsnowflakes>\n");
    USEPRT("       -delay       <delay in milliseconds>\n");
    USEPRT("       -unsmooth\n");
    USEPRT("       -whirl       <whirlfactor>\n");
    USEPRT("       -nowind\n");
    USEPRT("       -windtimer   <Time between windy periods in seconds>\n");
    USEPRT("       -xspeed      <max xspeed snowflakes>\n");
    USEPRT("       -yspeed      <max yspeed snowflakes>\n");
    USEPRT("       -wsnowdepth  <max snow depth on windows>\n");
    USEPRT("       -offset      <shift snow down>\n");
    USEPRT("       -ssnowdepth  <max snow depth at bottom of display>\n");
    USEPRT("       -notrees\n");
    USEPRT("       -nosanta\n");
    USEPRT("       -norudolf\n");
    USEPRT("       -santa       <santa>\n");
    USEPRT("       -santaspeed  <santa_speed>\n");
    USEPRT("       -nokeepsnow\n");
    USEPRT("       -nokeepsnowonwindows\n");
    USEPRT("       -nokeepsnowonscreen\n");
    USEPRT("       -nonopopup\n");
    USEPRT("       -version\n\n");
    USEPRT("Recommended: xsnow -bg SkyBlue3\n");
    
    exit(1);
}

void
SigHandler()
{
  done = 1;
}


void
SigHupHandler()
{
  void (*Sig_Hup_ptr)() = SigHupHandler;

  /*XClearWindow(display, rootWin);*/
  XFlush(display);
  XSetForeground(display,FurGC,trPix);
  XSetFillStyle(display,FurGC, FillSolid);
  XSetRegion(display,FurGC,SnowCatchR);
  XFillRectangle(display,rootWin,FurGC,0,0,display_width,display_height);
  XFlush(display);

  signal(SIGHUP, Sig_Hup_ptr);
}

/*
   Generate random integer between 0 and maxVal-1.
*/
int
RandInt(maxVal)
int maxVal;
{
	return rand() % maxVal;
}




/*
 * sleep for a number of micro-seconds
 */
void uSsleep(usec) 
unsigned long usec;
{
#ifdef SVR3
    poll((struct poll *)0, (size_t)0, usec/1000);   /* ms resolution */
#else
#ifdef VMS
    void lib$wait();
    float t;
    t= usec/1000000.0;
    lib$wait(&t);
#else
    struct timeval t;
    t.tv_usec = usec%(unsigned long)1000000;
    t.tv_sec = usec/(unsigned long)1000000;
    select(0, (void *)0, (void *)0, (void *)0, &t);
#endif
#endif
}


/*
   Check for topleft point of snowflake in specified rectangle.
*/
int
SnowPtInRect(snx, sny, recx, recy, width, height)
int snx;
int sny;
int recx;
int recy;
int width;
int height;
{
    if (snx < recx) return 0;
    if (snx > (recx + width)) return 0;
    if (sny < recy) return 0;
    if (sny > (recy + height)) return 0;
    
    return 1;
}


/*
   Give birth to a snowflake.
*/
void
InitSnowflake(rx)
int rx;
{
    Snow *r;
    int xx;

    r = &snowflakes[rx];

  if (curSnowflakes < maxSnowflakes) {
    r->whatFlake = RandInt(SNOWFLAKEMAXTYPE+1);

    /* Wind strikes! */
    if (wind) {
      if (direction == 1) 
        /* Create snow on the left of the display */
        r->intX = RandInt(display_width/3);
      else
        /* Create snow on the right of the display */
        r->intX = display_width - RandInt(display_width/3);
      r->intY =  RandInt(display_height);
    }
    else  {
      r->intX = RandInt(display_width - snowPix[r->whatFlake].width);
      r->intY =  RandInt(display_height/10) - snowPix[r->whatFlake].height;
    }

    xx = RandInt(MaxYStep+1);
    if( xx < (MaxYStep / 2) ) 
      xx = MaxYStep / 2;

    r->yStep = xx;
    r->xStep = 0;
    //    if (RandInt(1000) > 500) r->xStep = -r->xStep;
    r->active = 1;
    r->visible = 1;
    r->insnow = 0;
  }
}



/*
   Move a snowflake by erasing and redraw
*/
void
UpdateSnowflake(rx)
int rx;
{
Snow *snow;

int NewX;
int NewY;
int tmp_x;
int TouchDown;
int InVisible;
int NoErase;
    
  /* move an active snowflake */

  snow = &snowflakes[rx];
  NoErase = 0;  /* Always erase the flakes unless special case */


  /* Activate a new snowflake */
  if (!snow->active) {
    InitSnowflake(rx);
    /* Make sure newly created flakes aren't erased before actually drawn */
    /* Especially with wind, where flakes are created all over the screen */
    snow->insnow = 1; 
  }

  /* New x,y */
  if (wind) {
    tmp_x = abs(snow->xStep);
    if( wind == 1 ){  /* going to stop winding */
        tmp_x = tmp_x + (RandInt(WhirlFactor+1) - WhirlFactor/2);
    }else{            /* WINDY */
        tmp_x = tmp_x + (RandInt(20));
    }
    snow->xStep = tmp_x * direction;

    if (snow->xStep > 50) snow->xStep = 50;
    if (snow->xStep < -50) snow->xStep = -50;

    if( wind == 1 ){
        NewY = NewY+3;
    }else{
      //        NewY = NewY+10;
        NewY = NewY+20;
    }
  }
  NewX = snow->intX + snow->xStep;
  NewY = snow->intY + snow->yStep;


  /* If flake disappears offscreen (below bottom) it becomes inactive */
  /* In case of wind also deactivate flakes blown offscreen sideways, */
  /* so they'll reappear immediately. Make sure snow at bottom isn't  */
  /* disrupted.                          */

  /* Snowflake at the end of it's life? */
  snow->active = (NewY < WellenHeight - snowPix[snow->whatFlake].height);//display_height);
  /* If it is windy we need the flakes that blow offscreen! */
  if (wind) snow->active = 
      (snow->active && (NewX > 0) && (NewX < display_width));
    

  /* If flake covered by windows don't bother drawing it */
  InVisible = XRectInRegion(Windows,
                            NewX,NewY,
                            snowPix[snow->whatFlake].width,
                            snowPix[snow->whatFlake].height);
  snow->visible = (InVisible != RectangleIn);


  /*
   *  Snow touches snow
   */
  if (snow->visible) {

    /* Do not use XRectInRegion here, as that makes snow add to the bottom */
    /* of layers on windows too */
    /* Use middle bottom of flake */
    TouchDown = XPointInRegion(SnowCatchR,
                               NewX+snowPix[snow->whatFlake].width/2,
                               NewY+snowPix[snow->whatFlake].height);

    /* If the flake already is completely inside the region */
    if (TouchDown && snow->visible && !NoKeepSnow) {
      /* Only add snow that's visible, else snow will grow */
      /* on the *back* of windows */

      NoErase = 1;

      /* Add a bit to the snow bottom region to make snow build up */
      /* Snow isn't built up more than about a certain amount: inside */
      /* SnowAllowedR */

      /* If flake already inside snow layer don't add it again */
      TouchDown = XRectInRegion(SnowCatchR,
                                NewX,NewY,
                                snowPix[snow->whatFlake].width,
                                snowPix[snow->whatFlake].height);
      if (TouchDown == RectanglePart) {
        AddRect.x = NewX;
        AddRect.y = NewY + snowPix[snow->whatFlake].height - 2;
        AddRect.height = 2;
        AddRect.width = snowPix[snow->whatFlake].width;               
        if (XRectInRegion(SnowAllowedR,
                          AddRect.x, AddRect.y,
                          AddRect.width, AddRect.height) == RectangleIn) {
          XUnionRectWithRegion(&AddRect, SnowCatchR,SnowCatchR);
        }
      }
    }


    /* Adjust horizontal speed to mimic whirling */
    if (SmoothWhirl) 
      snow->xStep = snow->xStep + (RandInt(WhirlFactor+1) - WhirlFactor/2);
    else {
      /* Jerkier movement */
      snow->xStep = snow->xStep + RandInt(WhirlFactor);
      if (RandInt(1000) > 500) snow->xStep = -snow->xStep;
    }

  }  /* if snow->visible */



  if (!wind) {
    if (snow->xStep > MaxXStep) snow->xStep = MaxXStep;
    if (snow->xStep < -MaxXStep) snow->xStep = -MaxXStep;
  }

  /* Limit X speed inside snowlayers. 's only natural... */
  if (NoErase && !wind) snow->xStep = snow->xStep/2;
  /* xsnow 1.29 method: */
  /***if (NoErase) {
    snow->xStep = 0;
    NewX = snow->intX;
  }***/


  /* Erase, unless something special has happened */
  if (!snow->insnow) EraseSnowflake(rx);

  /* New coords, unless it's windy */
  snow->intX = NewX;
  snow->intY = NewY;

  /* If snowflake inside a snow layer now don't erase it next time */
  snow->insnow = NoErase;

  /* Draw if still active  and visible*/
  if (snow->active && snow->visible) DrawSnowflake(rx);
 
}


    
/*
   Draw a snowflake.
*/
void
DrawSnowflake(rx)
int rx;
{
    Snow *snow;

    snow = &snowflakes[rx];

    XSetTSOrigin(display, SnowGC[snow->whatFlake], snow->intX, snow->intY);
    XFillRectangle(display, rootWin, SnowGC[snow->whatFlake],
         snow->intX, snow->intY,
         snowPix[snow->whatFlake].width, snowPix[snow->whatFlake].height);
}


/*
   Erase a snowflake.
*/
void
EraseSnowflake(rx)
int rx;
{
    Snow *snow;

    snow = &snowflakes[rx];
   
    if (UseFillForClear) {
      XFillRectangle(display, rootWin, ClearSnowGC,
                 snow->intX, snow->intY,
                 snowPix[snow->whatFlake].width, 
                 snowPix[snow->whatFlake].height);
      }
    else {
      XClearArea(display, rootWin, 
                 snow->intX, snow->intY,
                 snowPix[snow->whatFlake].width, 
                 snowPix[snow->whatFlake].height,
                 False);
    }
}


/*
  Paint thick snow at bottom of screen for impaintient people
*/
void
PaintSnowAtBottom(depth)
int depth;
{
int x,y;
Snow *snow;

  for (y=0; y < depth; y++) {
    for (x=0; x<maxSnowflakes; x++) {
      InitSnowflake(x);
      snow = &snowflakes[x];
      snow->intY = display_height - y;
      DrawSnowflake(x);
    }
  }

}


/*
  Update Santa (How can you update the oldest icon in the world? Oh well. )
*/

#if !GRAB_SERVER

int
RoachErrors(dpy, err)
Display *dpy;
XErrorEvent *err;
{
    errorVal = err->error_code;

    return 0;
}

#endif /* GRAB_SERVER */

/*
   Calculate Visible region of tops of windows to catch snowflakes
*/
int
CalcWindowTops()
{
    Window *children;
    unsigned int nChildren;
    Window dummy;
    XWindowAttributes wa;
    int wx;
    XRectangle CatchRect;
    XRectangle AllowRect;
    XRectangle WinRect;
    int winX, winY;
    int NouMoe;
    unsigned int winHeight, winWidth;
    unsigned int depth;

    /*
       If we don't grab the server, the XGetWindowAttribute or XGetGeometry
       calls can abort us.  On the other hand, the server grabs can make for
       some annoying delays.
    */
#if GRAB_SERVER
    XGrabServer(display);
#else
    XSetErrorHandler(RoachErrors);
#endif

    /* Rebuild regions */
    XDestroyRegion(Windows);
    XDestroyRegion(SnowAllowedR);
    Windows = XCreateRegion();
    SnowAllowedR = XCreateRegion();

    /*
       Get children of root.
    */
    XQueryTree(display, rootWin, &dummy, &dummy, &children, &nChildren);

    /*
       For each mapped child, add the rectangle on top of the window
       where the snow could be to the region.
    */
    for (wx=0; wx<nChildren; wx++) {
        if (XEventsQueued(display, QueuedAlready)) {
          XFree((char *)children); 
          return 1;
        }
        errorVal = 0;
        XGetWindowAttributes(display, children[wx], &wa);
        if (errorVal) continue;

        /* Popup? */
#if debug
if (wa.save_under) printf("POPUP discarded\n");;
#endif
        if (!NoPopuphandling && wa.save_under) continue;

        if (wa.map_state == IsViewable) {
            XGetGeometry(display, children[wx], &dummy, &winX, &winY,
                &winWidth, &winHeight, &borderWidth, &depth);
#if debug
printf("\nw %d %d %d %d - %d %d \n", winX,winY,winWidth,winHeight,borderWidth,depth);
#endif
            if (errorVal) continue;
 
winY = winY + SnowOffset;
            /* Entirely offscreen? */
            if (winX > display_width) continue;
            if (winY > display_height) continue;
            if (winY <= 0) continue;


            /* Geometry of the window, borders inclusive */
            winX = winX-borderWidth;
            winY = winY-borderWidth;
            winHeight = winHeight + 2*borderWidth;
            winWidth = winWidth + 2*borderWidth;

            /* Entirely offscreen? */
            NouMoe = winX + winWidth;
            if (NouMoe < 0) continue;

            /* Make sure the initial catch region isn't punctured */
            if ((winY+winHeight) > display_height) {
              winHeight = display_height-winY;
            }

            if (winY < 0) {
              winHeight = winHeight + winY;
              winY = 0;
            }

            /* If window partly on screen */
            if (winX < 0) {
              winWidth = winWidth + winX - borderWidth;
              winX = 0;
            }

            WinRect.x = winX;
            WinRect.y = winY;
            WinRect.height = winHeight + 2*borderWidth;
            WinRect.width = winWidth + 2*borderWidth;

            /* The area of the windows themselves */
            XUnionRectWithRegion(&WinRect, Windows, Windows);





            /* Subtract this window from all previous catch regions! */
            SubR= XCreateRegion();
              XUnionRectWithRegion(&WinRect, SubR, SubR);

              XSubtractRegion(SnowCatchR,SubR, SnowCatchR);
            XDestroyRegion(SubR);
              


            /* CatchR */
            CatchRect.y = WinRect.y;
            CatchRect.x = WinRect.x;
            CatchRect.height = MaxYStep+MaxSnowFlakeHeight;
            CatchRect.width = WinRect.width;

	    if (!NoKeepSWin) 
              XUnionRectWithRegion(&CatchRect, SnowCatchR, SnowCatchR);
#if debug
printf("c %d %d %d %d\n",CatchRect.x,CatchRect.y,CatchRect.height,CatchRect.width);
#endif


            /* AllowedR */
            AllowRect.x = CatchRect.x;
            winY = CatchRect.y - MaxWinSnowDepth;
            AllowRect.height = CatchRect.height+MaxWinSnowDepth;
            if (winY < 0) {
              AllowRect.height = winY+CatchRect.height;
              winY = 0;
            }
            AllowRect.y = winY;
            AllowRect.width = CatchRect.width;
            if (!NoKeepSWin)
              XUnionRectWithRegion(&AllowRect, SnowAllowedR, SnowAllowedR);


        }
    }

    /* SnowAllowed at screen bottom */
    AllowRect.x = 0;
    AllowRect.y = display_height - MaxScrSnowDepth;
    AllowRect.width = display_width - 1;
    AllowRect.height = MaxYStep+MaxSnowFlakeHeight+MaxScrSnowDepth;
    if (!NoKeepSWin)
      XUnionRectWithRegion(&AllowRect, SnowAllowedR, SnowAllowedR);


    XFree((char *)children);

#if GRAB_SERVER
    XUngrabServer(display);
#else
    XSetErrorHandler((ErrorHandler *)NULL);
#endif


    return 0;
}

 


/*
   Allocate a color by name.
*/
Pixel
AllocNamedColor(colorName, dfltPix)
char *colorName;
Pixel dfltPix;
{
	Pixel pix;
	XColor scrncolor;
	XColor exactcolor;

	if (XAllocNamedColor(display, DefaultColormap(display, screen),
		colorName, &scrncolor, &exactcolor)) {
		pix = scrncolor.pixel;
	}
	else {
		pix = dfltPix;
	}

	return pix;
}


void sig_alarm()
{
    int rand=RandInt(100);
    void (*sig_alarm_ptr)() = sig_alarm;

    if(!NoWellen)
      {
	if( WellenHeight > MaxWasserHeight )
	  {
	    wassergrow = True;
	    signal(SIGALRM, sig_alarm_ptr);
	    alarm(WindTimer);
	  } else {
	    printf("Stopped raining\n");
	    StillRaining = False;
	  }
      }
    /* No wind at all? */
    if (NoWind) return;

    if( rand > 80 ){
        geta -= 1;
    }else if( rand > 55 ){
        geta += 1;
    }

    if( rand > 65 ){
        if( RandInt(10) > 4 ){
            direction = 1;
        }else{
            direction = -1;
        }

        wind = 2;
        signal(SIGALRM, sig_alarm_ptr);

        alarm(abs(RandInt(5))+1);
    }else{
        if( wind == 2 ){
            wind = 1;
            signal(SIGALRM, sig_alarm_ptr);
            alarm(RandInt(3)+1);
        }else{
            wind = 0;
            signal(SIGALRM, sig_alarm_ptr);
            alarm(WindTimer);
        }
    }
    
}
/************** initWellen() *************/
void initWellen()
{
  int i,x;
  WellenStartHeight = display_height - WellenStartHeight;
  WellenHeight = WellenStartHeight;
  x = -wellenPix.width;

  for( i=0; i<WellenNum; i++)
    {
      x = x + wellenPix.width;
      wellen[i].visible = 0;
      wellen[i].x = x;
      wellen[i].y = WellenStartHeight;
      wellen[i].xStep = 3;
      wellen[i].yStep = 1;
    }
}
/************* UpdateWellen -************/
void UpdateWellen()
{
  int Visible;
  int i;

  if(wassergrow == True)
    WellenHeight = WellenHeight - wellen[0].yStep;

  for( i=0; i<WellenNum; i++)
    {
      if(wellen[i].visible) EraseWelle(i);
      
      wellen[i].x = wellen[i].x + wellen[i].xStep;
      if(wassergrow == True)
	{
 	  wellen[i].y = wellen[i].y - wellen[i].yStep;
	}

      if(wellen[i].x >= display_width) wellen[i].x = -wellenPix.width;
      
      Visible = XRectInRegion(Windows,
			      wellen[i].x, wellen[i].y,
			      wellenPix.width, wellenPix.height);
      wellen[i].visible = (Visible != RectangleIn);
      wellen[i].visible = (wellen[i].visible || (wellen[i].x < 0));
      
      if(Visible != RectangleOut)
	{
	  SubR = XCreateRegion();
	  
	  AddRect.x = wellen[i].x;
	  AddRect.y = wellen[i].y;
	  AddRect.width = wellenPix.width;
	  AddRect.height = wellenPix.height;
	  XUnionRectWithRegion(&AddRect, SubR, SubR);
	  
	  XSubtractRegion(SnowCatchR, SubR, SnowCatchR);
	  XDestroyRegion(SubR);
	}

      if(wellen[i].visible) DrawWelle(i);
    }
  if(wassergrow == True || WellenStartHeight < display_height)
    {
      wassergrow = False;
      XSetTSOrigin(display, WasserGC, 0, 
		   WellenHeight + wellenPix.height);
      XFillRectangle(display, rootWin, WasserGC, 0, 
		     WellenHeight + wellenPix.height,
		     display_width, display_height);

    }
}
/************ EraseWellen **************/
void EraseWelle(int i) 
{
  XClearArea(display, rootWin, wellen[i].x, wellen[i].y, 
	     wellenPix.width, wellenPix.height, False);
}
/************** DrawWellen ****************/
void DrawWelle(int i)
{
  XSetTSOrigin(display, WellenGC, wellen[i].x, wellen[i].y);
  XFillRectangle(display, rootWin, WellenGC, wellen[i].x, wellen[i].y,
		 wellenPix.width, wellenPix.height);
  
}
