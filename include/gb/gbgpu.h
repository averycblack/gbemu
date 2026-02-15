#pragma once

#include <SDL.h>
#include "gbspace.h"
#include <BaseTsd.h>

#define DISP_WIDTH 160
#define DISP_HEIGHT 144

#define VRAM_SIZE 0x2000
#define CGB_VRAM_BANKS 2
#define SPRITE_ATTR_SIZE (0xFEA0 - 0xFE00)

#define LYC_INT 0x40
#define OAM_INT 0x20
#define VBLANK_INT 0x10
#define HBLANK_INT 0x08
#define STAT_INTS (LYC_INT | OAM_INT | VBLANK_INT | HBLANK_INT)
#define LYC_FLAG 0x4
#define MODE_MASK 0x3

enum class LcdMode {
	HBlank = 0,
	VBlank = 1,
	Search = 2,
	Render = 3,
};

struct gbSprite {
	UINT8 y_pos;
	UINT8 x_pos;
	UINT8 tile_idx;
	UINT8 flags;

	// Used for sorting in descending order
	bool operator > (const gbSprite& b) const {
		return x_pos > b.x_pos;
	}
};

struct Color {
	union {
		struct {
			UINT8 red;
			UINT8 green;
			UINT8 blue;
			UINT8 alpha;
		};
		UINT32 val;
	};

	bool operator != (const Color& b) const {
		return val != b.val;
	}

	bool operator == (const Color& b) const {
		return val == b.val;
	}
};

struct Palette {
	Color white;
	Color light_gray;
	Color dark_gray;
	Color black;
};

class gbGpu : public gbSpace {
public:
	// Palette from https://github.com/rvaccarim/FrozenBoy/blob/master/FrozenBoyUI/FrozenBoyGame.cs#L108-L114
	Palette palette[1] {
	{ { 224, 248, 208, 255 },
	  { 136, 192, 112, 255 },
	  { 52 , 104, 86 , 255 },
	  { 8  , 24 , 32 , 255 } }
	};

	union {
		UINT8 mem[SPRITE_ATTR_SIZE]{ 0 };
		gbSprite sprites[SPRITE_ATTR_SIZE / sizeof(gbSprite)];
	} oam;
	UINT8 vram[VRAM_SIZE * CGB_VRAM_BANKS]{ 0 };

	UINT8 lcdc = 0x91; // FF40 (lcd control)
	UINT8 stat = 0x81; // FF41 (status)
	UINT8 scy = 0;	// FF42 (Scroll y)
	UINT8 scx = 0;	// FF43 (Scroll x)
	UINT8 ly = 0;	// FF44 (LCD Y)
	UINT8 lyc = 0;	// FF45 (LCD Y cmp)
	UINT8 bgp = 0xFC;	// FF47 (BG Palette Data)
	UINT8 obp0 = 0; // FF48 (Obj Paleete 0 Data)
	UINT8 obp1 = 0; // FF49 (Obj Palette 1 Data)
	UINT8 wy = 0;	// FF4A (Window Y Position)
	UINT8 wx = 0;	// FF4B (Window X Position + 7)

	// OAM DMA
	UINT16 dmaCurrentByte = 0;
	UINT16 dmaStartAddr = 0;

	int ticks = 0;

	virtual int readByte(UINT16 addr) override;
	virtual int writeByte(UINT16 addr, UINT8 byte) override;

	bool lcdEnabled() { return lcdc & 0x80; }
	bool windowEnabled() { return lcdc & 0x20; }
	UINT16 windowTileMapArea() { return lcdc & 0x40 ? 0x9C00 : 0x9800;  }
	bool bgWindow8000Method() { return lcdc & 0x10; }
	UINT16 bgTileMapArea() { return lcdc & 0x08 ? 0x9C00 : 0x9800; }
	bool objSizeIs16() { return lcdc & 0x04; }
	bool objEnable() { return lcdc & 0x02; }
	bool bgWindowPriority() { return lcdc & 0x01; }

	Color backBuffer[DISP_HEIGHT][DISP_WIDTH]{ 0 };
	Color frontBuffer[DISP_HEIGHT][DISP_WIDTH]{ 0 };

	void renderSprites();
	void renderTile(int x, int y, int bgX, int bgY, UINT16 tilemap);
	void renderLine();
	void step();
};