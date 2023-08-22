/***************************/
/*** 3d Tile Layer Class ***/
/***************************/

#include 	"system\global.h"
#include	"system\vid.h"
#include	<DStructs.h>
#include 	"utils\utils.h"
#include 	"gfx\prim.h"
#include	"game\game.h"

#include	"LayerTile.h"
#include	"LayerTile3d.h"


#if		1
#if		defined(__USER_sbart__) // || defined(__USER_daveo__)
#define	_SHOW_POLYZ_	1
#include	"gfx\font.h"	
static		FontBank		*Font;
int			ShowPolyz=0;
#endif
#endif

static const int	BLOCK_SIZE				=16;
#if defined(__TERRITORY_EUR__)
static const int	SCREEN_TILE_ADJ_U		=2;
static const int	SCREEN_TILE_ADJ_D		=3;	// Extra 2 lines needed, cos of extra height & perspective :o(
static const int	SCREEN_TILE_ADJ_L		=3;
static const int	SCREEN_TILE_ADJ_R		=3;
#else
static const int	SCREEN_TILE_ADJ_U		=2;
static const int	SCREEN_TILE_ADJ_D		=1;
static const int	SCREEN_TILE_ADJ_L		=3;
static const int	SCREEN_TILE_ADJ_R		=3;
#endif
static const int	SCREEN_TILE3D_WIDTH		=(INGAME_SCREENW/BLOCK_SIZE)+SCREEN_TILE_ADJ_L+SCREEN_TILE_ADJ_R;
static const int	SCREEN_TILE3D_HEIGHT	=(INGAME_SCREENH/BLOCK_SIZE)+SCREEN_TILE_ADJ_U+SCREEN_TILE_ADJ_D;

static const int	RENDER_X_OFS			=INGAME_SCREENOFS_X-(SCREEN_TILE_ADJ_L*BLOCK_SIZE)+INGAME_RENDER_OFS_X;
static const int	RENDER_Y_OFS			=INGAME_SCREENOFS_Y-(SCREEN_TILE_ADJ_U*BLOCK_SIZE)+INGAME_RENDER_OFS_Y;

static const int	DeltaTableSizeX=SCREEN_TILE3D_WIDTH+1;
static const int	DeltaTableSizeY=SCREEN_TILE3D_HEIGHT+1;

/*****************************************************************************/
// now uses a single delta table for front and back (interleaved) to reduce register use
// 0 LUF
// 1 RUF
// 2 LDF
// 3 RDF

// 4 LUB
// 5 RUB
// 6 LDB
// 7 RDB

sFlipTable	FlipTable[4]=
{
/*00 <0*/	{{+4096,0,+4096,0},	{	((DVECTOR*)SCRATCH_RAM)+0,
									((DVECTOR*)SCRATCH_RAM)+1,
									((DVECTOR*)SCRATCH_RAM)+2,
									((DVECTOR*)SCRATCH_RAM)+3,
									((DVECTOR*)SCRATCH_RAM)+4,
									((DVECTOR*)SCRATCH_RAM)+5,
									((DVECTOR*)SCRATCH_RAM)+6,
									((DVECTOR*)SCRATCH_RAM)+7,
								}, 0<<31},	
/*01 >0*/	{{-4096,0,+4096,0},	{	((DVECTOR*)SCRATCH_RAM)+1,
									((DVECTOR*)SCRATCH_RAM)+0,
									((DVECTOR*)SCRATCH_RAM)+3,
									((DVECTOR*)SCRATCH_RAM)+2,
									((DVECTOR*)SCRATCH_RAM)+5,
									((DVECTOR*)SCRATCH_RAM)+4,
									((DVECTOR*)SCRATCH_RAM)+7,
									((DVECTOR*)SCRATCH_RAM)+6,
								},1<<31},
/*10 >0*/	{{+4096,0,-4096,0},	{	((DVECTOR*)SCRATCH_RAM)+2,
									((DVECTOR*)SCRATCH_RAM)+3,
									((DVECTOR*)SCRATCH_RAM)+0,
									((DVECTOR*)SCRATCH_RAM)+1,
									((DVECTOR*)SCRATCH_RAM)+6,
									((DVECTOR*)SCRATCH_RAM)+7,
									((DVECTOR*)SCRATCH_RAM)+4,
									((DVECTOR*)SCRATCH_RAM)+5,
								},1<<31},
/*11 <0*/	{{-4096,0,-4096,0},	{	((DVECTOR*)SCRATCH_RAM)+3,
									((DVECTOR*)SCRATCH_RAM)+2,
									((DVECTOR*)SCRATCH_RAM)+1,
									((DVECTOR*)SCRATCH_RAM)+0,
									((DVECTOR*)SCRATCH_RAM)+7,
									((DVECTOR*)SCRATCH_RAM)+6,
									((DVECTOR*)SCRATCH_RAM)+5,
									((DVECTOR*)SCRATCH_RAM)+4,
								},0<<31}
};

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
CLayerTile3d::CLayerTile3d(sLevelHdr *LevelHdr,sLayerHdr *Hdr,u8 *_RGBMap,u8 *_RGBTable) : CLayerTile(LevelHdr,Hdr)
{
	ElemBank3d=LevelHdr->ElemBank3d;
	TriList=LevelHdr->TriList;
	QuadList=LevelHdr->QuadList;
	VtxList=LevelHdr->VtxList;
	VtxIdxList=LevelHdr->VtxIdxList;
	RGBMap=_RGBMap;
	RGBTable=_RGBTable;

	#if defined(_SHOW_POLYZ_)
		Font=new ("PrimFont") FontBank;
		Font->initialise( &standardFont );
		Font->setOt( 0 );
		Font->setTrans(1);
	#endif
}

/*****************************************************************************/
CLayerTile3d::~CLayerTile3d()
{
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void	CLayerTile3d::init(DVECTOR &MapPos,int Shift)
{
		CLayerTile::init(MapPos,Shift);
		CalcDelta();
}

/*****************************************************************************/
void	CLayerTile3d::shutdown()
{
	#if defined(_SHOW_POLYZ_)
		Font->dump();
		delete Font;
	#endif
	for (int i=0; i<16; i++)
	{
		MemFree(DeltaTableX[i]);
		MemFree(DeltaTableY[i]);
	}

}

/*****************************************************************************/
void CLayerTile3d::CalcDelta()
{
    VECTOR BlkPos;
    SVECTOR Pnt = {-BLOCK_SIZE / 2, -BLOCK_SIZE / 2, -BLOCK_SIZE * 4};
    s16* TabX = nullptr;
    s16* TabY = nullptr;

    CGameScene::setCameraMtx();

    for (int i = 0; i < 16; ++i)
    {
        TabX = (s16*)MemAlloc(DeltaTableSizeX * 2 * sizeof(s16), "DeltaTableXTable");
        DeltaTableX[i] = TabX;
        ASSERT(TabX);

        TabY = (s16*)MemAlloc(DeltaTableSizeY * 2 * sizeof(s16), "DeltaYTable");
        DeltaTableY[i] = TabY;
        ASSERT(TabY);

        BlkPos.vx = RENDER_X_OFS - i;
        BlkPos.vy = RENDER_Y_OFS;

        for (int j = 0; j < DeltaTableSizeX; ++j)
        {
            s32 Tmp;
            DVECTOR O;
            CMX_SetTransMtxXY(&BlkPos);
            
            Pnt.vz = -BLOCK_SIZE * 4;
            RotTransPers(&Pnt, (s32*)&O, &Tmp, &Tmp);
            *TabX++ = O.vx;
            
            Pnt.vz = +BLOCK_SIZE * 4;
            RotTransPers(&Pnt, (s32*)&O, &Tmp, &Tmp);
            *TabX++ = O.vx;
            
            BlkPos.vx += BLOCK_SIZE;
        }

        BlkPos.vx = RENDER_X_OFS;
        BlkPos.vy = RENDER_Y_OFS - i;

        for (int j = 0; j < DeltaTableSizeY; ++j)
        {
            s32 Tmp;
            DVECTOR O;
            CMX_SetTransMtxXY(&BlkPos);

            Pnt.vz = -BLOCK_SIZE * 4;
            RotTransPers(&Pnt, (s32*)&O, &Tmp, &Tmp);
            *TabY++ = O.vy;

            Pnt.vz = +BLOCK_SIZE * 4;
            RotTransPers(&Pnt, (s32*)&O, &Tmp, &Tmp);
            *TabY++ = O.vy;

            BlkPos.vy += BLOCK_SIZE;
        }
    }

    DeltaF = DeltaTableX[0][(1 * 2) + 0] - DeltaTableX[0][(0 * 2) + 0];
    DeltaB = DeltaTableY[0][(1 * 2) + 1] - DeltaTableY[0][(0 * 2) + 1];
}


/*****************************************************************************/
void CLayerTile3d::think(DVECTOR &MapPos)
{
    MapXY.vx = (MapPos.vx >> 4) - SCREEN_TILE_ADJ_L;
    MapXY.vy = (MapPos.vy >> 4) - SCREEN_TILE_ADJ_U;

    ShiftX = MapPos.vx & 15;
    ShiftY = MapPos.vy & 15;

    RenderOfs.vx = RenderOfs.vy = 0;
    DeltaFOfs.vx = DeltaFOfs.vy = 0;
    DeltaBOfs.vx = DeltaBOfs.vy = 0;

    if (MapXY.vx < 0)
    {
        int absVx = -MapXY.vx;
        RenderOfs.vx = absVx * BLOCK_SIZE;
        DeltaFOfs.vx = absVx * DeltaF;
        DeltaBOfs.vx = absVx * DeltaB;
        MapXY.vx = 0;
    }

    if (MapXY.vy < 0)
    {
        int absVy = -MapXY.vy;
        RenderOfs.vy = absVy * BLOCK_SIZE;
        DeltaFOfs.vy = absVy * DeltaF;
        DeltaBOfs.vy = absVy * DeltaB;
        MapXY.vy = 0;
    }

    RenderW = std::min(SCREEN_TILE3D_WIDTH, MapWidth - MapXY.vx);
    RenderH = std::min(SCREEN_TILE3D_HEIGHT, MapHeight - MapXY.vy);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void CLayerTile3d::CacheElemVtx(sElem3d* Elem)
{
    int Count = Elem->VtxTriCount;
    sVtx* V0, * V1, * V2;
    u16* IdxTable = &VtxIdxList[Elem->VtxIdxStart];
    s32* OutVtx = (s32*)SCRATCH_RAM + 8;

    V0 = &VtxList[*IdxTable++];
    V1 = &VtxList[*IdxTable++];
    V2 = &VtxList[*IdxTable++];
    gte_ldv3(V0, V1, V2);

    while (Count--)
    {
        s32* OutPtr = OutVtx;
        OutVtx += 3;

        V0 = &VtxList[*IdxTable++];
        V1 = &VtxList[*IdxTable++];
        V2 = &VtxList[*IdxTable++];

        gte_ldv3(V0, V1, V2);

        gte_rtpt_b();
        gte_stsxy3c(OutPtr);
    }
}

/*****************************************************************************/
void CLayerTile3d::render()
{
    VECTOR BlkPos;
    DVECTOR DP[8];  // Delta pointers

    int MapOfs = GetMapOfs();
    sTileMapElem* MapPtr = Map + MapOfs;
    u8* RGBMapPtr = RGBMap + MapOfs;
    u8* PrimPtr = GetPrimPtr();
    const u8* XYList = (const u8*)SCRATCH_RAM;

    #if defined(_SHOW_POLYZ_)
    	s16 TCount = 0, QCount = 0;
    #endif

    BlkPos.vx = RENDER_X_OFS - ShiftX + RenderOfs.vx;
    BlkPos.vy = RENDER_Y_OFS - ShiftY + RenderOfs.vy;

    for (int Y = 0; Y < RenderH; ++Y)
    {
        sTileMapElem* MapRow = MapPtr;
        u8* RGBRow = RGBMapPtr;
        s32 BlkXOld = BlkPos.vx;
        s16* DeltaTabX = DeltaTableX[ShiftX];

        for (int X = 0; X < RenderW; ++X)
        {
            u16 Tile = MapRow->Tile;
            u16 TileIdx = Tile >> 2;
            sElem3d* Elem = &ElemBank3d[TileIdx];

            int TriCount = Elem->TriCount;
            int QuadCount = Elem->QuadCount;

            if (TriCount || QuadCount)
            {
                sFlipTable* FTab = &FlipTable[Tile & 3];
                u8* RGB = &RGBTable[RGBRow[X] * (16 * 4)];

                CMX_SetTransMtxXY(&BlkPos);
                CMX_SetRotMatrixXY(&FTab->Mtx);

                // Cache vertex pointers
                cacheVertices(Elem);

                // Cache delta pointers
                cacheDeltas(FTab, DeltaTabX, DeltaTableY[ShiftY]);

                // Render triangles
                renderTriangles(Elem, XYList, PrimPtr, TriCount, RGB, FTab->ClipCode);
                
                // Render quads
                renderQuads(Elem, XYList, PrimPtr, QuadCount, RGB, FTab->ClipCode);

            }
            BlkPos.vx += BLOCK_SIZE;
            DeltaTabX += 2;
        }

        MapPtr += MapWidth;
        RGBMapPtr += MapWidth;
        BlkPos.vx = BlkXOld;
        BlkPos.vy += BLOCK_SIZE;
    }

    SetPrimPtr((u8*)PrimPtr);

    #if defined(_SHOW_POLYZ_)
    	char Txt[256];
    	sprintf(Txt, "TC %i\nQC %i", TCount, QCount);
    	Font->print(128, 32, Txt);
    #endif
}
