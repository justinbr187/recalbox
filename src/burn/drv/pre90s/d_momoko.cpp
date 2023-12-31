// FB Alpha Momoko 120% driver module
// Based on MAME driver by Uki

#include "tiles_generic.h"
#include "z80_intf.h"
#include "burn_ym2203.h"
#include "watchdog.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *DrvZ80ROM0;
static UINT8 *DrvZ80ROM1;
static UINT8 *DrvGfxROM0;
static UINT8 *DrvGfxROM1;
static UINT8 *DrvGfxROM1a;
static UINT8 *DrvGfxROM2;
static UINT8 *DrvGfxROM3;
static UINT8 *DrvBankROM;
static UINT8 *DrvBgCPROM;
static UINT8 *DrvFgMPROM;
static UINT8 *DrvColPROM;
static UINT8 *DrvZ80RAM0;
static UINT8 *DrvZ80RAM1;
static UINT8 *DrvSprRAM;
static UINT8 *DrvPalRAM;
static UINT8 *DrvVidRAM;

static UINT8 *DrvTransTab[4];

static UINT32 *DrvPalette;
static UINT8 DrvRecalc;

static UINT8 *soundlatch;
static UINT8 *flipscreen;
static UINT8 *fg_scrolly;
static UINT8 *fg_scrollx;
static UINT8 *fg_select;
static UINT8 *tx_scrolly;
static UINT8 *tx_mode;
static UINT8 *bg_scrolly;
static UINT8 *bg_scrollx;
static UINT8 *bg_select;
static UINT8 *bg_latch;
static UINT8 *bg_priority;
static UINT8 *bg_bank;

static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvDips[3];
static UINT8 DrvInputs[3];
static UINT8 DrvReset;

static struct BurnInputInfo MomokoInputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	DrvJoy3 + 7,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy1 + 1,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy1 + 3,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"	},

	{"P2 Start",		BIT_DIGITAL,	DrvJoy1 + 7,	"p2 start"	},
	{"P2 Up",			BIT_DIGITAL,	DrvJoy2 + 0,	"p2 up"		},
	{"P2 Down",			BIT_DIGITAL,	DrvJoy2 + 1,	"p2 down"	},
	{"P2 Left",			BIT_DIGITAL,	DrvJoy2 + 3,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"	},

	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Momoko)

static struct BurnDIPInfo MomokoDIPList[]=
{
	{0x10, 0xff, 0xff, 0x7f, NULL			},
	{0x11, 0xff, 0xff, 0xff, NULL			},
	{0x12, 0xff, 0xff, 0x00, NULL			},

	{0   , 0xfe, 0   ,    4, "Lives"		},
	{0x10, 0x01, 0x03, 0x03, "3"			},
	{0x10, 0x01, 0x03, 0x02, "4"			},
	{0x10, 0x01, 0x03, 0x01, "5"			},
	{0x10, 0x01, 0x03, 0x00, "255 (Cheat)"		},

	{0   , 0xfe, 0   ,    8, "Coinage"		},
	{0x10, 0x01, 0x1c, 0x10, "5 Coins 1 Credits"	},
	{0x10, 0x01, 0x1c, 0x14, "3 Coins 1 Credits"	},
	{0x10, 0x01, 0x1c, 0x18, "2 Coins 1 Credits"	},
	{0x10, 0x01, 0x1c, 0x1c, "1 Coin  1 Credits"	},
	{0x10, 0x01, 0x1c, 0x0c, "1 Coin  2 Credits"	},
	{0x10, 0x01, 0x1c, 0x04, "2 Coins 5 Credits"	},
	{0x10, 0x01, 0x1c, 0x08, "1 Coin  5 Credits"	},
	{0x10, 0x01, 0x1c, 0x00, "Free Play"		},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x10, 0x01, 0x60, 0x40, "Easy"			},
	{0x10, 0x01, 0x60, 0x60, "Normal"		},
	{0x10, 0x01, 0x60, 0x20, "Hard"			},
	{0x10, 0x01, 0x60, 0x00, "Hardest"		},

	{0   , 0xfe, 0   ,    4, "Bonus Life"		},
	{0x11, 0x01, 0x03, 0x01, "20k"			},
	{0x11, 0x01, 0x03, 0x03, "30k"			},
	{0x11, 0x01, 0x03, 0x02, "50k"			},
	{0x11, 0x01, 0x03, 0x00, "100k"			},

	{0   , 0xfe, 0   ,    2, "Cabinet"		},
	{0x11, 0x01, 0x10, 0x00, "Upright"		},
	{0x11, 0x01, 0x10, 0x10, "Cocktail"		},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x11, 0x01, 0x20, 0x00, "Off"			},
	{0x11, 0x01, 0x20, 0x20, "On"			},

//	{0   , 0xfe, 0   ,    2, "Flip Screen (Fake)"	},
//	{0x12, 0x01, 0x01, 0x00, "Off"			},
//	{0x12, 0x01, 0x01, 0x01, "On"			},
};

STDDIPINFO(Momoko)

static inline void palette_write(UINT16 offset)
{
	INT32 r = DrvPalRAM[offset + 0] & 0x0f;
	INT32 g = DrvPalRAM[offset + 1] >> 4;
	INT32 b = DrvPalRAM[offset + 1] & 0x0f;

	DrvPalette[offset/2] = BurnHighCol(r + (r * 16), g + (g * 16), b + (b * 16), 0);
}

static void bankswitch(INT32 data)
{
	*bg_bank = data;

	ZetMapMemory(DrvBankROM + (data & 0x1f) * 0x1000, 0xf000, 0xffff, MAP_ROM);
}

static void __fastcall momoko_main_write(UINT16 address, UINT8 data)
{
	if ((address & 0xfc00) == 0xd800) {
		DrvPalRAM[(address & 0x3ff)] = data;
		palette_write(address & 0x3fe);
		return;
	}

	switch (address)
	{
		case 0xd402:
			*flipscreen = data & 0x01;
		return;

		case 0xd404:
			BurnWatchdogWrite();
		return;

		case 0xd406:
			*soundlatch = data;
		return;

		case 0xdc00:
			*fg_scrolly = data;
		return;

		case 0xdc01:
			*fg_scrollx = data;
		return;

		case 0xdc02:
			*fg_select = data;
		return;

		case 0xe800:
			*tx_scrolly = data;
		return;

		case 0xe801:
			*tx_mode = data;
		return;

		case 0xf000:
		case 0xf001:
			bg_scrolly[address & 1] = data;
		return;

		case 0xf002:
		case 0xf003:
			bg_scrollx[address & 1] = data;
		return;

		case 0xf004:
			bankswitch(data);
		return;

		case 0xf006:
			//*bg_select = data;
			*bg_latch = data;
		return;

		case 0xf007:
			*bg_priority = data & 0x01;
		return;
	}
}

static UINT8 __fastcall momoko_main_read(UINT16 address)
{
	switch (address)
	{
		case 0xd400:
			return DrvInputs[0];

		case 0xd402:
			return DrvInputs[1];

		case 0xd406:
			return (DrvInputs[2] & 0x80) | (DrvDips[0] & 0x7f);

		case 0xd407:
			return DrvDips[1];
	}

	return 0;
}

static void __fastcall momoko_sound_write(UINT16 address, UINT8 data)
{
	switch (address)
	{
		case 0xa000:
		case 0xa001:
			BurnYM2203Write(0, address & 1, data);
		return;

		case 0xc000:
		case 0xc001:
			BurnYM2203Write(1, address & 1, data);
		return;
	}
}

static UINT8 __fastcall momoko_sound_read(UINT16 address)
{
	switch (address)
	{
		case 0xa000:
		case 0xa001:
			return BurnYM2203Read(0, address & 1);

		case 0xc000:
		case 0xc001:
			return BurnYM2203Read(1, address & 1);
	}

	return 0;
}

static UINT8 momoko_sound_read_port_A(UINT32)
{
	return *soundlatch;
}

static INT32 DrvDoReset(INT32 clear)
{
	if (clear) {
		memset(AllRam, 0, RamEnd - AllRam);
	}

	ZetOpen(0);
	ZetReset();
	ZetClose();

	ZetOpen(1);
	ZetReset();
	BurnYM2203Reset();
	ZetClose();

	BurnWatchdogReset();

	HiscoreReset();

	return 0;
}

static INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	DrvZ80ROM0		= Next; Next += 0x00c000;
	DrvZ80ROM1		= Next; Next += 0x008000;

	DrvBankROM		= Next; Next += 0x020000;
	DrvBgCPROM		= Next; Next += 0x002000;
	DrvFgMPROM		= Next; Next += 0x004000;
	DrvColPROM		= Next; Next += 0x000120;

	DrvGfxROM0		= Next; Next += 0x008000;
	DrvGfxROM1		= Next; Next += 0x080000;
	DrvGfxROM1a		= Next; Next += 0x020000;
	DrvGfxROM2		= Next; Next += 0x008000;
	DrvGfxROM3		= Next; Next += 0x040000;

	DrvTransTab[0]	= Next; Next += 0x008000 / 0x08;
	DrvTransTab[1]	= Next; Next += 0x000200;
	DrvTransTab[2]	= Next; Next += 0x008000 / 0x40;
	DrvTransTab[3]	= Next; Next += 0x040000 / 0x80;

	DrvPalette		= (UINT32*)Next; Next += 0x0200 * sizeof(UINT32);

	AllRam			= Next;

	DrvSprRAM		= Next; Next += 0x000100;
	DrvZ80RAM0		= Next; Next += 0x001000;
	DrvZ80RAM1		= Next; Next += 0x000800;
	DrvPalRAM		= Next; Next += 0x000400;
	DrvVidRAM		= Next; Next += 0x000400;

	soundlatch		= Next; Next += 0x000001;
	flipscreen		= Next; Next += 0x000001;

	fg_scrolly		= Next; Next += 0x000001;
	fg_scrollx		= Next; Next += 0x000001;
	fg_select		= Next; Next += 0x000001;
	tx_scrolly		= Next; Next += 0x000001;
	tx_mode			= Next; Next += 0x000001;
	bg_scrolly		= Next; Next += 0x000002;
	bg_scrollx		= Next; Next += 0x000002;
	bg_select		= Next; Next += 0x000001;
	bg_latch		= Next; Next += 0x000001;
	bg_priority		= Next; Next += 0x000001;
	bg_bank			= Next; Next += 0x000001;

	RamEnd			= Next;

	MemEnd			= Next;

	return 0;
}

static INT32 DrvGfxDecode()
{
	INT32 Planes[4]  = { 4,0,12,8 };
	INT32 XOffs0[8]  = { STEP4(0,1), STEP4(256*8*8,1) };
	INT32 YOffs0[8]  = { STEP8(0, 8) };
	INT32 XOffs1[8]  = { STEP4(0,1), STEP4(4096*8,1) };
	INT32 YOffs1[16] = { STEP16(0, 16) };

	UINT8 *tmp = (UINT8*)BurnMalloc(0x20000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, DrvGfxROM0, 0x02000);

	GfxDecode(0x0200, 2, 8, 8, Planes, XOffs0, YOffs0, 0x040, tmp, DrvGfxROM0);

	memcpy (DrvGfxROM1a, DrvGfxROM1, 0x20000);

	GfxDecode(0x2000, 4, 8, 8, Planes, XOffs1, YOffs1, 0x080, DrvGfxROM1a, DrvGfxROM1);

	memcpy (tmp, DrvGfxROM2, 0x02000);

	GfxDecode(0x0100, 2, 8, 8, Planes, XOffs0, YOffs0, 0x040, tmp, DrvGfxROM2);

	memcpy (tmp, DrvGfxROM3, 0x10000);

	GfxDecode(0x0800, 4, 8, 16, Planes, XOffs1, YOffs1, 0x100, tmp, DrvGfxROM3);

	BurnFree(tmp);

	return 0;
}

static void DrvFillTransTab(INT32 tab, UINT8 *gfx, INT32 len, INT32 size)
{
	memset (DrvTransTab[tab], 1, len / size);

	for (INT32 i = 0; i < len; i+= size) {
		for (INT32 j = 0; j < size; j++) {
			if (gfx[i+j]) {
				DrvTransTab[tab][i/size] = 0;
				break;
			}
		}
	}
}

static void DrvFillTransMask()
{
	for (INT32 i = 0x100; i < 0x200; i+=0x10) {
		memset (DrvTransTab[1] + i + 8, 0xff, 8);
	}
}

static INT32 DrvInit()
{
	BurnAllocMemIndex();

	{
		if (BurnLoadRom(DrvZ80ROM0 + 0x00000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x08000,  1, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM1 + 0x00000,  2, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  3, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x00000,  4, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM3 + 0x00001,  5, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x00000,  6, 2)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  7, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x00001,  8, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x10000,  9, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x10001, 10, 2)) return 1;

		if (BurnLoadRom(DrvBankROM + 0x00000, 11, 1)) return 1;
		if (BurnLoadRom(DrvBankROM + 0x08000, 12, 1)) return 1;
		if (BurnLoadRom(DrvBankROM + 0x10000, 13, 1)) return 1;
		if (BurnLoadRom(DrvBankROM + 0x18000, 14, 1)) return 1;

		if (BurnLoadRom(DrvBgCPROM + 0x00000, 15, 1)) return 1;

		if (BurnLoadRom(DrvFgMPROM + 0x00000, 16, 1)) return 1;

		if (BurnLoadRom(DrvColPROM + 0x00000, 17, 1)) return 1;
		if (BurnLoadRom(DrvColPROM + 0x00100, 18, 1)) return 1;

		if (DrvGfxDecode()) return 1;

		DrvFillTransTab(0, DrvGfxROM0, 0x008000, 0x08);
		DrvFillTransTab(3, DrvGfxROM3, 0x040000, 0x80);
		DrvFillTransTab(2, DrvGfxROM2, 0x008000, 0x40);
		DrvFillTransMask();
	}

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM0,	0x0000, 0xbfff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM0,	0xc000, 0xcfff, MAP_RAM);
	ZetMapMemory(DrvSprRAM,		0xd000, 0xd0ff, MAP_RAM);
	ZetMapMemory(DrvPalRAM,		0xd800, 0xdbff, MAP_ROM); // write through handler
	ZetMapMemory(DrvVidRAM,		0xe000, 0xe3ff, MAP_RAM);
	ZetSetWriteHandler(momoko_main_write);
	ZetSetReadHandler(momoko_main_read);
	ZetClose();

	ZetInit(1);
	ZetOpen(1);
	ZetMapMemory(DrvZ80ROM1,	0x0000, 0x7fff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM1,	0x8000, 0x87ff, MAP_RAM);
	ZetSetWriteHandler(momoko_sound_write);
	ZetSetReadHandler(momoko_sound_read);
	ZetClose();

	BurnWatchdogInit(DrvDoReset, 180);

	BurnYM2203Init(2, 1250000, NULL, 0);
	BurnYM2203SetPorts(1, momoko_sound_read_port_A, NULL, NULL, NULL);
	BurnTimerAttachZet(2500000);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_YM2203_ROUTE, 0.40, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_1, 0.15, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_2, 0.15, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_3, 0.15, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_YM2203_ROUTE, 0.40, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_1, 0.15, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_2, 0.15, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_3, 0.15, BURN_SND_ROUTE_BOTH);

	GenericTilesInit();

	DrvDoReset(1);

	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();

	ZetExit();

	BurnYM2203Exit();

	BurnFreeMemIndex();

	return 0;
}

static void draw_bg_layer(int pri)
{
	INT32 dx   = ~bg_scrollx[0] & 7;
	INT32 dy   = ~bg_scrolly[0] & 7;
	INT32 rx   = (bg_scrollx[0] + bg_scrollx[1] * 256) >> 3;
	INT32 ry   = (bg_scrolly[0] + bg_scrolly[1] * 256) >> 3;
	INT32 bank = (*bg_select & 0x0f) * 0x200;

	for (INT32 offs = 0; offs < 32 * 32; offs++)
	{
		INT32 sx = (offs & 0x1f);
		INT32 sy = (offs / 0x20);

		INT32 ofst  = (((ry + sy + 2) & 0x3ff) << 7) + ((rx + sx) & 0x7f);
		INT32 code  = DrvBankROM[ofst] + bank;
		INT32 color = DrvBgCPROM[code + (*bg_priority * 0x100)] & 0x1f;

		if ((color & 0x10) == pri && !pri) continue;

		sx = (sx * 8) + dx - 6;
		sy = (sy * 8) + dy + 9 - 16;

		if (pri) {
			Render8x8Tile_Clip(pTransDraw, code, sx - 8, sy, color&0x0f, 4, 0x100, DrvGfxROM1);
		} else {
			RenderTileTranstab(pTransDraw, DrvGfxROM1, code, ((color&0x0f)<<4)+0x100, 0, sx - 8, sy, 0, 0, 8, 8, DrvTransTab[1]);
		}
	}
}

static void draw_fg_layer()
{
	INT32 dx   = ~*fg_scrollx & 7;
	INT32 dy   = ~*fg_scrolly & 7;
	INT32 rx   = (*fg_scrollx) >> 3;
	INT32 ry   = (*fg_scrolly) >> 3;
	INT32 bank = (*fg_select & 0x03) * 0x800;

	for (INT32 offs = 0; offs < 32 * 32; offs++)
	{
		INT32 sx = (offs & 0x1f);
		INT32 sy = (offs / 0x20);

		INT32 ofst = ((ry + sy + 34) & 0x3f) * 0x20 + ((rx + sx) & 0x1f) + bank;
		INT32 code = DrvFgMPROM[ofst];

		sx = (sx * 8) + dx - 6;
		sy = (sy * 8) + dy + 1; //9 - (8);

		if (DrvTransTab[2][code]) continue;

		Render8x8Tile_Mask_Clip(pTransDraw, code, sx, sy, 0, 2, 0, 0, DrvGfxROM2);
	}
}

static void draw_txt_layer()
{
	for (INT32 offs = 16 * 32; offs < 240 * 32; offs++)
	{
		INT32 color;
		INT32 sx = (offs & 0x1f) * 8;
		INT32 sy = (offs / 0x20);
		INT32 y = sy;

		if (*tx_mode)
		{
			if ((DrvColPROM[sy] & 0xf8) == 0) sy += *tx_scrolly;

			color = (DrvColPROM[y] & 0x07) | 0x10;
		}
		else
		{
			color = DrvColPROM[(sy >> 3) + 0x100] & 0x0f;
		}

		INT32 code = DrvVidRAM[((sy >> 3) << 5) + (sx >> 3)] * 8 + (sy & 7);

		if (DrvTransTab[0][code] || (sy-16) >= nScreenHeight || (sx-8) >= nScreenWidth) continue;

		RenderCustomTile_Mask_Clip(pTransDraw, 8, 1, code, sx - 8, y - 16, color, 2, 0, 0, DrvGfxROM0);
	}
}

static void draw_sprites(INT32 start, INT32 end)
{
	UINT8 *sprite_ram = DrvSprRAM + 0x64;

	for (INT32 offs = start; offs < end; offs += 4)
	{
		INT32 sy    = 239 - sprite_ram[offs + 0] - 16;
		INT32 code  = sprite_ram[offs + 1] | ((sprite_ram[offs + 2] & 0x60) << 3);
		      code  = ((code & 0x380) << 1) | (code & 0x7f);
		INT32 color = sprite_ram[offs + 2] & 0x07;
		INT32 flipy = sprite_ram[offs + 2] & 0x08;
		INT32 flipx = ~sprite_ram[offs + 2] & 0x10;
		INT32 sx    = sprite_ram[offs + 3] - 8;

		if (DrvTransTab[3][code]) continue;

		DrawCustomMaskTile(pTransDraw, 8, 16, code, sx, sy, flipx, flipy, color, 4, 0, 0x80, DrvGfxROM3);
	}
}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		for (INT32 i = 0; i < 0x400; i+=2) {
			palette_write(i);
		}

		DrvRecalc = 0;
	}

	BurnTransferClear();

	if (((*bg_select | *bg_latch) & 0x10) == 0)
	{
		if (nBurnLayer & 1) draw_bg_layer(0x10);
	}
	else
	{
		BurnTransferClear(0x100);
	}

	if (nSpriteEnable & 1) draw_sprites(0, 0x24);

	if (((*bg_select | *bg_latch) & 0x10) == 0)
	{
		if (nBurnLayer & 2) draw_bg_layer(0);
	}

	if (nSpriteEnable & 2) draw_sprites(0x24, 0x100-0x64);

	if (nBurnLayer & 4) draw_txt_layer();

	if ((*fg_select & 0x10) == 0)
	{
		if (nBurnLayer & 8) draw_fg_layer();
	}

	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 DrvFrame()
{
	BurnWatchdogUpdate();

	if (DrvReset) {
		DrvDoReset(1);
	}

	ZetNewFrame();

	{
		memset (DrvInputs, 0xff, 3 * sizeof(UINT8));

		for (INT32 i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		}
	}

	INT32 nInterleave = 100;
	INT32 nCyclesTotal[2] = { 5000000 / 60, 2500000 / 60 };
	INT32 nCyclesDone[2] = { 0, 0 };

	for (INT32 i = 0; i < nInterleave; i++)
	{
		ZetOpen(0);
		CPU_RUN(0, Zet);
		if (i == nInterleave - 1) ZetSetIRQLine(0, CPU_IRQSTATUS_HOLD);
		ZetClose();

		ZetOpen(1);
		CPU_RUN_TIMER(1);
		ZetClose();
	}

	ZetOpen(1);
	if (pBurnSoundOut) {
		BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
	}
	ZetClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	*bg_select = *bg_latch; // delay 1 frame, gets rid of corruption on scene changes -dink july 31, 2020

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029702;
	}

	if (nAction & ACB_VOLATILE) {
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All RAM";
		BurnAcb(&ba);

		ZetScan(nAction);

		BurnYM2203Scan(nAction, pnMin);

		BurnWatchdogScan(nAction);
	}

	if (nAction & ACB_WRITE)
	{
		ZetOpen(0);
		bankswitch(*bg_bank);
		ZetClose();
	}

	return 0;
}


// Momoko 120% (Japanese text)

static struct BurnRomInfo momokoRomDesc[] = {
	{ "momoko03.m6",	0x8000, 0x386e26ed, 0x01 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "momoko02.m5",	0x4000, 0x4255e351, 0x01 | BRF_PRG | BRF_ESS }, //  1

	{ "momoko01.u4",	0x8000, 0xe8a6673c, 0x02 | BRF_PRG | BRF_ESS }, //  2 Z80 #1 Code

	{ "momoko13.u4",	0x2000, 0x2745cf5a, 0x03 | BRF_GRA },           //  3 Character Tiles

	{ "momoko14.p2",	0x2000, 0xcfccca05, 0x04 | BRF_GRA },           //  4 Foreground Tiles

	{ "momoko16.e5",	0x8000, 0xfc6876fc, 0x05 | BRF_GRA },           //  5 Sprite Tiles
	{ "momoko17.e6",	0x8000, 0x45dc0247, 0x05 | BRF_GRA },           //  6

	{ "momoko09.e8",	0x8000, 0x9f5847c7, 0x06 | BRF_GRA },           //  7 Background Tiles
	{ "momoko11.c8",	0x8000, 0x9c9fbd43, 0x06 | BRF_GRA },           //  8
	{ "momoko10.d8",	0x8000, 0xae17e74b, 0x06 | BRF_GRA },           //  9
	{ "momoko12.a8",	0x8000, 0x1e29c9c4, 0x06 | BRF_GRA },           // 10

	{ "momoko04.r8",	0x8000, 0x3ab3c2c3, 0x07 | BRF_GRA },           // 11 Background Map (Banks used by Z80 #0)
	{ "momoko05.p8",	0x8000, 0x757cdd2b, 0x07 | BRF_GRA },           // 12
	{ "momoko06.n8",	0x8000, 0x20cacf8b, 0x07 | BRF_GRA },           // 13
	{ "momoko07.l8",	0x8000, 0xb94b38db, 0x07 | BRF_GRA },           // 14

	{ "momoko08.h8",	0x2000, 0x69b41702, 0x08 | BRF_GRA },           // 15 Background Color/Priority Table

	{ "momoko15.k2",	0x4000, 0x8028f806, 0x09 | BRF_GRA },           // 16 Foreground Map

	{ "momoko-c.bin",	0x0100, 0xf35ccae0, 0x0a | BRF_GRA },           // 17 Text Layer Color PROMs
	{ "momoko-b.bin",	0x0020, 0x427b0e5c, 0x0a | BRF_GRA },           // 18
};

STD_ROM_PICK(momoko)
STD_ROM_FN(momoko)

struct BurnDriver BurnDrvMomoko = {
	"momoko", NULL, NULL, NULL, "1986",
	"Momoko 120% (Japanese text)\0", NULL, "Jaleco", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_HISCORE_SUPPORTED, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, momokoRomInfo, momokoRomName, NULL, NULL, NULL, NULL, MomokoInputInfo, MomokoDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	240, 216, 4, 3
};


// Momoko 120% (English text)

static struct BurnRomInfo momokoeRomDesc[] = {
	{ "3.m6",			0x8000, 0x84053a7d, 0x01 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "2.m5",			0x4000, 0x98ad397b, 0x01 | BRF_PRG | BRF_ESS }, //  1

	{ "momoko01.u4",	0x8000, 0xe8a6673c, 0x02 | BRF_PRG | BRF_ESS }, //  2 Z80 #1 Code

	{ "momoko13.u4",	0x2000, 0x2745cf5a, 0x03 | BRF_GRA },           //  3 Character Tiles

	{ "momoko14.p2",	0x2000, 0xcfccca05, 0x04 | BRF_GRA },           //  4 Foreground Tiles

	{ "momoko16.e5",	0x8000, 0xfc6876fc, 0x05 | BRF_GRA },           //  5 Sprite Tiles
	{ "momoko17.e6",	0x8000, 0x45dc0247, 0x05 | BRF_GRA },           //  6

	{ "momoko09.e8",	0x8000, 0x9f5847c7, 0x06 | BRF_GRA },           //  7 Background Tiles
	{ "momoko11.c8",	0x8000, 0x9c9fbd43, 0x06 | BRF_GRA },           //  8
	{ "momoko10.d8",	0x8000, 0xae17e74b, 0x06 | BRF_GRA },           //  9
	{ "momoko12.a8",	0x8000, 0x1e29c9c4, 0x06 | BRF_GRA },           // 10

	{ "momoko04.r8",	0x8000, 0x3ab3c2c3, 0x07 | BRF_GRA },           // 11 Background Map (Banks used by Z80 #0)
	{ "momoko05.p8",	0x8000, 0x757cdd2b, 0x07 | BRF_GRA },           // 12
	{ "momoko06.n8",	0x8000, 0x20cacf8b, 0x07 | BRF_GRA },           // 13
	{ "momoko07.l8",	0x8000, 0xb94b38db, 0x07 | BRF_GRA },           // 14

	{ "momoko08.h8",	0x2000, 0x69b41702, 0x08 | BRF_GRA },           // 15 Background Color/Priority Table

	{ "momoko15.k2",	0x4000, 0x8028f806, 0x09 | BRF_GRA },           // 16 Foreground Map

	{ "momoko-c.bin",	0x0100, 0xf35ccae0, 0x0a | BRF_GRA },           // 17 Text Layer Color PROMs
	{ "momoko-b.bin",	0x0020, 0x427b0e5c, 0x0a | BRF_GRA },           // 18
};

STD_ROM_PICK(momokoe)
STD_ROM_FN(momokoe)

struct BurnDriver BurnDrvMomokoe = {
	"momokoe", "momoko", NULL, NULL, "1986",
	"Momoko 120% (English text)\0", NULL, "Jaleco", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, momokoeRomInfo, momokoeRomName, NULL, NULL, NULL, NULL, MomokoInputInfo, MomokoDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	240, 216, 4, 3
};


// Momoko 120% (bootleg)
// bootleg board, almost exact copy of an original one

static struct BurnRomInfo momokobRomDesc[] = {
	{ "3.bin",			0x8000, 0xa18d7e78, 0x01 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "2.bin",			0x4000, 0x2dcf50ed, 0x01 | BRF_PRG | BRF_ESS }, //  1

	{ "momoko01.u4",	0x8000, 0xe8a6673c, 0x02 | BRF_PRG | BRF_ESS }, //  2 Z80 #1 Code

	{ "momoko13.u4",	0x2000, 0x2745cf5a, 0x03 | BRF_GRA },           //  3 Character Tiles

	{ "momoko14.p2",	0x2000, 0xcfccca05, 0x04 | BRF_GRA },           //  4 Foreground Tiles

	{ "16.bin",			0x8000, 0x49de49a1, 0x05 | BRF_GRA },           //  5 Sprite Tiles
	{ "17.bin",			0x8000, 0xf06a3d1a, 0x05 | BRF_GRA },           //  6

	{ "momoko09.e8",	0x8000, 0x9f5847c7, 0x06 | BRF_GRA },           //  7 Background Tiles
	{ "momoko11.c8",	0x8000, 0x9c9fbd43, 0x06 | BRF_GRA },           //  8
	{ "10.bin",			0x8000, 0x68b9156d, 0x06 | BRF_GRA },           //  9
	{ "12.bin",			0x8000, 0xc32f5e19, 0x06 | BRF_GRA },           // 10

	{ "4.bin",			0x8000, 0x1f0226d5, 0x07 | BRF_GRA },           // 11 Background Map (Banks used by Z80 #0)
	{ "momoko05.p8",	0x8000, 0x757cdd2b, 0x07 | BRF_GRA },           // 12
	{ "momoko06.n8",	0x8000, 0x20cacf8b, 0x07 | BRF_GRA },           // 13
	{ "momoko07.l8",	0x8000, 0xb94b38db, 0x07 | BRF_GRA },           // 14

	{ "momoko08.h8",	0x2000, 0x69b41702, 0x08 | BRF_GRA },           // 15 Background Color/Priority Table

	{ "momoko15.k2",	0x4000, 0x8028f806, 0x09 | BRF_GRA },           // 16 Foreground Map

	{ "momoko-c.bin",	0x0100, 0xf35ccae0, 0x0a | BRF_GRA },           // 17 Text Layer Color PROMs
	{ "momoko-b.bin",	0x0020, 0x427b0e5c, 0x0a | BRF_GRA },           // 18
};

STD_ROM_PICK(momokob)
STD_ROM_FN(momokob)

struct BurnDriver BurnDrvMomokob = {
	"momokob", "momoko", NULL, NULL, "1986",
	"Momoko 120% (bootleg)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG | BDF_HISCORE_SUPPORTED, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, momokobRomInfo, momokobRomName, NULL, NULL, NULL, NULL, MomokoInputInfo, MomokoDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	240, 216, 4, 3
};
