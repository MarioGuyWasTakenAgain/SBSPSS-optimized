/******************/
/*** GFx System ***/
/******************/

#include "system/global.h"
#include "system\vid.h"
#include "gfx\prim.h"
#include "fileio\fileio.h"
#include "utils\lznp.h"


/*****************************************************************************/
#define MaxVBFuncs 	4

/*****************************************************************************/
static void (*VbFunc)(void);
static VbFuncType VbFuncList[MaxVBFuncs];

static u32			FrameCounter=0,TickCount=0,TickBuffer[2];
static u32			s_lastFrameCounter=0,s_vblsThisFrame=0;
static sVidScreen 	Screen[2];
static int			ScreenXOfs=0,ScreenYOfs=0;
static int			ScreenW, ScreenH;
static RECT 		ScreenRect;
/*static*/ int		FrameFlipFlag=0;
static int			ClearScreen=0;
static	u8			*ScreenImage=0;

static const CVECTOR s_defClearCol = {0, 0, 0};

/*****************************************************************************/
/*** Loading Icon Cack *******************************************************/
/*****************************************************************************/
POLY_FT4	LoadPoly;
static int	LoadX=430;
static int	LoadY=192-8;
static int	LoadBackY;
static int	LoadHalfWidth;
static int	LoadIconSide;
static int	DrawLoadIcon=0;
static RECT	LoadBackRect;
static int	LoadTime=0;
static const int	LoadBackInc=8;
static	DISPENV		*VblDispEnv=0;	// Disp End used as Vbl flip flag, so MUST be set after DrawEnv
static	DRAWENV		*VblDrawEnv=0;

/*****************************************************************************/
// Altered to keep aspect ratio
s8	LoadTab[]=
{ 
	21,21,21,21,20,20,20,20,19,19,19,18,18,17,17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,17,18,18,19,19,19,20,20,20,20,20,21
};
const int	LoadTabSize=sizeof(LoadTab)/sizeof(s8);

/*****************************************************************************/
void	LoadingIcon()
{
int			Dst;
int			rgb;
POLY_FT4	*PolyPtr=&LoadPoly;

		Dst=LoadTab[LoadTime];
		
		PolyPtr->x0=PolyPtr->x2=LoadX-Dst+LoadHalfWidth+2;
		PolyPtr->x1=PolyPtr->x3=LoadX+Dst+LoadHalfWidth+2;

		rgb=128-(LoadTab[(LoadTime+LoadTabSize/2)%LoadTabSize]*3);
		setRGB0(PolyPtr,rgb,rgb,rgb);

		MoveImage(&LoadBackRect,LoadX-LoadBackInc,LoadBackY);
		PutDrawEnv(&Screen[FrameFlipFlag^1].Draw);
		DrawPrim(PolyPtr);

		LoadTime++;
		if (LoadTime>=LoadTabSize) 
		{
			LoadTime=0;
		}
}

/*****************************************************************************/
void	SetUpLoadIcon(void *_fh)
{
sFrameHdr *fh=(sFrameHdr*)_fh;
		setPolyFT4(&LoadPoly);
		setXYWH(&LoadPoly,LoadX,LoadY,fh->W,fh->H);
		setUVWH(&LoadPoly,fh->U,fh->V,fh->W,fh->H);
		LoadPoly.tpage=fh->TPage;
		LoadPoly.clut=fh->Clut;

		LoadHalfWidth=fh->W/2;
		LoadBackRect.w=fh->W+(LoadBackInc*2);
		LoadBackRect.h=fh->H;

}
/*****************************************************************************/
int		OldClearScreen=0;
void	StartLoad(int _loadX,int _loadY)
{
		SYSTEM_DBGMSG("Start Load");

		if (_loadX!=-1) 
		{
			LoadX=_loadX;
		}
		if (_loadY!=-1)
		{
			LoadY=_loadY;
		}
		OldClearScreen=Screen[0].Draw.isbg;
		Screen[0].Draw.isbg=Screen[1].Draw.isbg=0;

		PutDrawEnv(&Screen[FrameFlipFlag^1].Draw);
		PutDispEnv(&Screen[FrameFlipFlag].Disp);

		LoadBackRect.x=LoadX-LoadBackInc;
		LoadBackRect.y=LoadY+((FrameFlipFlag)*256);;
		LoadBackY=LoadY+((FrameFlipFlag^1)*256);

		LoadTime=0;
		DrawLoadIcon=1;
		LoadIconSide=0;
}

/*****************************************************************************/
void	StopLoad()
{
	if(DrawLoadIcon)
	{
#if	defined(__USER_CDBUILD__)
		while(LoadTime) 
			{
			VSync(0);
			}
#endif
		Screen[0].Draw.isbg=Screen[1].Draw.isbg=1;

		DrawLoadIcon=0;
		SYSTEM_DBGMSG("Stop Load");
	}
	VidSetClearScreen(OldClearScreen);
}

/*****************************************************************************/
/*** VSync *******************************************************************/
/*****************************************************************************/
extern "C"
{
static void VidVSyncCallback()
{
	int i;
	FrameCounter++;
	TickCount++;
	if (DrawLoadIcon) LoadingIcon();
//	PutDispEnv(&Screen[FrameFlipFlag].Disp); 
//	PutDrawEnv(&Screen[FrameFlipFlag].Draw);
	if (VblDispEnv) 
	{
		PutDispEnv(VblDispEnv);
		PutDrawEnv(VblDrawEnv);
		VblDispEnv=0;
		VblDrawEnv=0;
	}

	if (VbFunc)
		{
		VbFunc();
		VbFunc = NULL;
		}
	for (i=0; i< MaxVBFuncs; i++) 
	{
		if (VbFuncList[i]) 
		{
			VbFuncList[i]();
		}
	}
}
}
/*****************************************************************************/
void VidAddVSyncFunc(VbFuncType v) 
{
	int i;
	for (i=0; i<MaxVBFuncs; i++) 
		{
		if (!VbFuncList[i]) 
			{
			VbFuncList[i] = v;
			return;
			}
		}
	ASSERT(!"Number of Vsync Funcs == MaxVBFuncs");

}

/*****************************************************************************/
void VidRemoveVSyncFunc(VbFuncType v)
{
	int i;
	for (i=0; i<MaxVBFuncs; i++) 
	{
		if (VbFuncList[i] == v) 
		{
			VbFuncList[i] = NULL;
			return;
		}
	}
	ASSERT(!"VSYNC Func Not Found");
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int VidGetXOfs() {
    return ScreenXOfs;
}

int VidGetYOfs() {
    return ScreenYOfs;
}

void VidSetXYOfs(int x, int y) {
    ScreenXOfs = x;
    ScreenYOfs = y;
}

int VidGetScrW() {
    return ScreenW;
}

int VidGetScrH() {
    return ScreenH;
}

sVidScreen* VidGetScreen() {
    return &Screen[FrameFlipFlag];
}

sVidScreen* VidGetDispScreen() {
    return VidGetScreen();
}

sVidScreen* VidGetDrawScreen() {
    return &Screen[FrameFlipFlag^1];
}

u32 VidGetFrameCount() {
    return FrameCounter;
}

u32 VidGetTickCount() {
    return TickBuffer[FrameFlipFlag^1];
}

int VidGetVblsThisFrame() {
    return s_vblsThisFrame;
}

void SetScreenImage(u8* Ptr) {
    ScreenImage = Ptr;
}

u8* GetScreenImage() {
    return ScreenImage;
}

void ClearScreenImage() {
    ScreenImage = 0;
}


/*****************************************************************************/
void ClearVRam() {
#if defined(__VERSION_DEBUG__) && !defined(__USER_CDBUILD__)
    RECT Rect;
    // Clear the entire video RAM
    setRECT(&Rect, INGAME_SCREENW, 0, 1024 - INGAME_SCREENW, 512);
    ClearImage(&Rect, 0, 0, 0);

    for (int X = 0; X < 8; X++) {
        u8 C0 = 0xFF * (X & 1);
        DrawSync(0);

        // Clear the top half of the screen
        setRECT(&Rect, X * 128, 0, 1024 / 8, 256);
        ClearImage(&Rect, C0, 0, 0xFF - C0);

        // Clear the bottom half of the screen
        setRECT(&Rect, X * 128, 256, 1024 / 8, 256);
        ClearImage(&Rect, 0xFF - C0, 0, C0);
    }
#endif
}


/*****************************************************************************/
void VidScrOn()
{
	Screen[0].Draw.isbg = ClearScreen;
	Screen[1].Draw.isbg = ClearScreen;
	VSync(0);							//	wait for V-BLANK
	SetDispMask(1);						//	display on
}

/*****************************************************************************/
void VidSetDrawEnv()
{
	int	x = VidGetScrW();
	int	y = VidGetScrH();

	SetDefDrawEnv( &Screen[0].Draw, 0, 0, x, y );
	SetDefDispEnv( &Screen[0].Disp, 0, y, x, y );

	SetDefDrawEnv( &Screen[1].Draw, 0, y, x, y );
	SetDefDispEnv( &Screen[1].Disp, 0, 0, x, y );

	Screen[0].Draw.isbg = ClearScreen;
	Screen[1].Draw.isbg = ClearScreen;

	Screen[0].Draw.dtd = 1;
	Screen[1].Draw.dtd = 1;

	VidSetClearColor( s_defClearCol );

	SetDrawEnv( &Screen[0].Draw.dr_env, &Screen[0].Draw );
	SetDrawEnv( &Screen[1].Draw.dr_env, &Screen[1].Draw );
}


/*****************************************************************************/

void VidSetClearColor( const CVECTOR & col )
{
	Screen[0].Draw.r0 = Screen[1].Draw.r0 = col.r;
	Screen[0].Draw.g0 = Screen[1].Draw.g0 = col.g;
	Screen[0].Draw.b0 = Screen[1].Draw.b0 = col.b;
}

/*****************************************************************************/
void VidSetClearScreen(int Flag)
{
	ClearScreen = Flag;
	VidSetDrawEnv();
}

/*****************************************************************************/
void VidSetRes(int x, int y)
{
	ASSERT( y == 256 );

	if ((VidGetScrW() != x) || (VidGetScrH() != y))
	{
		RECT	clrRect;

		ScreenW=x;
		ScreenH=y;

		VidSetDrawEnv();
		SetGeomOffset( (x / 2), (y / 2) );
	}
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void VidInit()
{
// Wap up a loading screen
u8	*screenData=LoadPakScreen(LOADINGSCREENS_BOOTSCREEN_GFX);
	SetScreenImage(screenData);

//	VidSetXYOfs( ScreenXOfs, ScreenYOfs );

	SetDispMask(0);
	#if defined(__TERRITORY_USA__) || defined(__TERRITORY_JAP__)
		SetVideoMode( MODE_NTSC );
		ScreenYOfs=0;	// Set screen ofs for NTSC
	#else
		SetVideoMode( MODE_PAL );
/* changed cos the recomended amount was too bloody low!!
		ScreenYOfs=16;	// Set screen ofs for PAL
*/
/*
		ScreenYOfs=0;	// Set screen ofs for PAL
*/
		// PKG - Changed back 7/9/01
		// NB: These have also been added to InitSys() in psxboot
		ScreenYOfs=16;	// Set screen ofs for PAL
	#endif

	VSync(0);
	VSync(0);
	ResetGraph(0);
	SetGraphDebug(0);
	ClearVRam();
	InitGeom();
	SetGeomScreen(GEOM_SCREEN_H);

	VidSetRes( INGAME_SCREENW, INGAME_SCREENH);
	DrawSync(0);
	VidSwapDraw();
	DrawSync(0);
	VidSwapDraw();
	DrawSync(0);
	SetScreenImage(0);
	MemFree(screenData);

// Init VBL
	VbFunc = NULL;
	for (int i=0; i<MaxVBFuncs; i++)
	{
		VbFuncList[i] = NULL;
	}
	VSyncCallback( VidVSyncCallback );

	// Delay to let the screen stabilise before we put up the loading screen
	for(int i=0; i<50; i++)
	{
		VSync(0);
	}
	VidScrOn();
}

/*****************************************************************************/
#ifdef __USER_paul__
int ScreenClipBox = 1;
#else
int ScreenClipBox = 0;
#endif

void VidSwapDraw()
{
    int LastFrame = FrameFlipFlag;
    int ScrH = VidGetScrH() * FrameFlipFlag;
    FrameFlipFlag ^= 1;
    TickBuffer[FrameFlipFlag] = TickCount;
    TickCount = 0;

    Screen[FrameFlipFlag].Disp.disp.x = 0;
    Screen[FrameFlipFlag].Disp.disp.y = ScrH;
    Screen[FrameFlipFlag].Disp.disp.w = ScreenW;
    Screen[FrameFlipFlag].Disp.disp.h = ScreenH;

    Screen[FrameFlipFlag].Disp.screen.x = ScreenXOfs;
    Screen[FrameFlipFlag].Disp.screen.y = ScreenYOfs;
    Screen[FrameFlipFlag].Disp.screen.w = 256;
    Screen[FrameFlipFlag].Disp.screen.h = ScreenH;

    VblDrawEnv = &Screen[FrameFlipFlag].Draw;
    VblDispEnv = &Screen[FrameFlipFlag].Disp;

    VSync(0);

    if (ScreenImage)
    {
        LoadImage(&Screen[LastFrame].Disp.disp, (u_long*)ScreenImage);
        while (DrawSync(1));
    }

#if defined(__VERSION_DEBUG__)

    if (ScreenClipBox == 1)
    {
        DrawLine(15, 25, ScreenW - 15, 25, 255, 0, 0, 0);
        DrawLine(15, ScreenH - 25, ScreenW - 15, ScreenH - 25, 255, 0, 0, 0);
        DrawLine(15, 25, 15, ScreenH - 25, 255, 0, 0, 0);
        DrawLine(ScreenW - 15, 25, ScreenW - 15, ScreenH - 25, 255, 0, 0, 0);

        DrawLine(0, 0, 511, 0, 0, 255, 0, 0);
        DrawLine(0, 255, 511, 255, 0, 255, 0, 0);
        DrawLine(0, 0, 0, 255, 0, 255, 0, 0);
        DrawLine(511, 0, 511, 255, 0, 255, 0, 0);
    }

    if (ScreenClipBox == 2)
    {
        for (int i = 0; i < 4; ++i)
        {
            POLY_F4* f4 = GetPrimF4();
            int x = (i == 2) ? ScreenW - 10 : 0;
            int y = (i == 1) ? ScreenH - 20 : 20;
            int w = (i == 2) ? 10 : ScreenW;
            int h = (i == 1 || i == 3) ? 20 : ScreenH - 40;

            setXYWH(f4, x, y, w, h);
            setRGB0(f4, 50, 50, 50);
            AddPrimToList(f4, 0);
        }
    }
#endif

    int fc = FrameCounter;
    s_vblsThisFrame = fc - s_lastFrameCounter;
    s_lastFrameCounter = fc;

    if (s_vblsThisFrame == 0)
    {
        s_vblsThisFrame = 1;
    }
}


/*****************************************************************************/
u8		*LoadPakScreen(int Filename)
{
u8		*PakData=CFileIO::loadFile((FileEquate)Filename,"PakScreen");
u8		*Screen=(u8*)MemAlloc(512*256*2,"Screen");

		LZNP_Decode(PakData,Screen);
		MemFree(PakData);

		return(Screen);
}

/*****************************************************************************/
/*** VRAM VIEWER *************************************************************/
/*****************************************************************************/

#if defined(__VERSION_DEBUG__) && defined(UseVRamViewer)

#include "pad\pads.H"

void VRamViewer()
{
    bool Done = false;
    sVidScreen* Scr = VidGetScreen();
    u16 Pad;
    int OldX = Scr->Disp.disp.x;
    int OldY = Scr->Disp.disp.y;

    while (!Done)
    {
        PadUpdate();
        DbgPollHost();

        Pad = PadGetHeld(0);

#ifdef __USER_paul__
        // my finger was hurting..
        if ((PadGetDown(0) & PAD_SELECT))
        {
            Done = true;
        }
#else
        if (!(Pad & PAD_SELECT))
        {
            Done = true;
        }
#endif

        if (Pad & PAD_LEFT)
        {
            if (Scr->Disp.disp.x)
            {
                Scr->Disp.disp.x--;
            }
        }
        if (Pad & PAD_RIGHT)
        {
            if (Scr->Disp.disp.x < 1024 - ScreenW)
            {
                Scr->Disp.disp.x++;
            }
        }
        if (Pad & PAD_UP)
        {
            if (Scr->Disp.disp.y)
            {
                Scr->Disp.disp.y--;
            }
        }
        if (Pad & PAD_DOWN)
        {
            if (Scr->Disp.disp.y < 512 - ScreenH)
            {
                Scr->Disp.disp.y++;
            }
        }

        PutDispEnv(&Scr->Disp);
        PutDrawEnv(&Scr->Draw);
    }

    Scr->Disp.disp.x = OldX;
    Scr->Disp.disp.y = OldY;
}

#endif
#endif
