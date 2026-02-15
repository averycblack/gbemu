#include <gb/gbgpu.h>
#include <gb/gbcpu.h>
#include <gb/gb.h>
#include <vector>
#include <algorithm>
#include <assert.h>

Color& getColor(Palette& palette, UINT8 idx) {
	assert(idx < 4);

	switch (idx) {
	case 0: return palette.white;
	case 1: return palette.light_gray;
	case 2: return palette.dark_gray;
	case 3: return palette.black;
	}

	return palette.white;
}

void gbGpu::renderTile(int x, int y, int bgX, int bgY, UINT16 tilemap) {
	Palette& palette = this->palette[0];
	int tileLocX = bgX / 8;
	int tileLocY = bgY / 8;

	// 32 tiles per row
	int tileIdx = (tileLocY * 32) + tileLocX;

	UINT8 tileNumber = vram[tilemap - 0x8000 + tileIdx];

	UINT16 tileAddr;
	if (bgWindow8000Method()) {
		tileAddr = 0x8000 + (tileNumber * 16);
	} else {
		tileAddr = 0x9000 + INT8(tileNumber) * 16;
	}

	int tileX = bgX % 8;
	int tileY = bgY % 8;

	int byte = tileAddr + (tileY * 2);
	UINT8 low = (vram[byte - 0x8000] >> (7 - tileX)) & 0x1;
	UINT8 high = (vram[byte + 1 - 0x8000] >> (7 - tileX)) & 0x1;
	UINT8 paletteIdx = low | (high << 1);

	// Grab color from Palette data
	UINT8 paletteColor = (bgp >> (paletteIdx * 2)) & 0x3;

	Color& color = getColor(palette, paletteColor);

	assert(y >= 0 && y < DISP_HEIGHT&&
		   x >= 0 && x < DISP_WIDTH);
	
	backBuffer[y][x] = color;
}

/*
 * Works in 3 steps
 * 1. Create list of up to 10 sprites to render
 * 2. From lowest to highest priority, draw sprites
 * 3. Copy pixels to framebuffer, ignoring any pixels which
 *        should be overrided by the background
 */
void gbGpu::renderSprites() {
	Color lineColors[DISP_WIDTH]{ 0 };
	bool bgOverSprite[DISP_WIDTH]{ false };

	int line = ly;
	Palette& palette = this->palette[0];

	// Find all sprites to render on current scanline
	std::vector<gbSprite> queue;
	for (gbSprite& sprite : oam.sprites) {
		int spriteMinY = sprite.y_pos - 16;
		int spriteMaxY = spriteMinY + (objSizeIs16() ? 16 : 8);
		if (line >= spriteMinY && line < spriteMaxY) {
			queue.push_back(sprite);
		}

		// PPU can render a max of 10 sprites per line
		if (queue.size() == 10) break;
	}

	// Render from biggest x coordinate to smallest, as smaller X has higher priority
	std::sort(queue.begin(), queue.end(), std::greater<gbSprite&>());

	for (gbSprite& sprite : queue) {
		UINT8 palleteData = sprite.flags & 0x10 ? obp1 : obp0;
		int spriteY = line - (sprite.y_pos - 16);
		bool yflip = sprite.flags & 0x40;
		bool xflip = sprite.flags & 0x20;
		bool bgPriority = sprite.flags & 0x80;

		int idx = sprite.tile_idx;
		if (objSizeIs16()) {
			idx &= 0xFFFE;
		}

		if (yflip) {
			if (objSizeIs16()) {
				spriteY = 15 - spriteY;
			} else {
				spriteY = 7 - spriteY;
			}
		}

		UINT16 addr = 0x8000 + (idx * 16);

		addr += spriteY * 2;
		UINT8 high = readByte(addr + 1);
		UINT8 low = readByte(addr);

		for (int spriteX = 7; spriteX >= 0; spriteX--) {
			// Grab color from Palette data
			UINT8 paletteIdx = (low & 1) | ((high & 1) << 1);
			UINT8 paletteColor = (palleteData >> (paletteIdx * 2)) & 0x3;

			UINT8 correctX = xflip ? 7 - spriteX : spriteX;

			high >>= 1;
			low >>= 1;

			// Index 0 is ignored and transparent
			if (paletteIdx == 0) continue;
			Color& color = getColor(palette, paletteColor);

			int drawX = correctX + sprite.x_pos - 8;

			// Sprites can be partially (or completely) off screen.
			if (line >= 0 && line < DISP_HEIGHT &&
				drawX >= 0 && drawX < DISP_WIDTH) {
				lineColors[drawX] = color;
				bgOverSprite[drawX] = bgPriority;
			}
		}
	}

	// Copy pixel data to framebuffer
	// If background has priority and the background color is NOT idx 0,
	// then do not copy sprite pixel
	UINT8 bgTransparentIdx = bgp & 0x3;
	Color& bgTransparent = getColor(palette, bgTransparentIdx);

	assert(line >= 0 && line < DISP_HEIGHT);

	for (int x = 0; x < DISP_WIDTH; x++) {
		Color& color = lineColors[x];
		if (color.val == 0) continue;
		if (bgOverSprite[x] && backBuffer[line][x] != bgTransparent) continue;
		backBuffer[line][x] = color;
	}
}

void gbGpu::renderLine() {
	int line = ly;

	for (int x = 0; x < DISP_WIDTH; x++) {
		if (bgWindowPriority()) {
			int bgX = (scx + x) % 256;
			int bgY = (scy + line) % 256;
			renderTile(x, line, bgX, bgY, bgTileMapArea());
		}

		if (windowEnabled() && line >= wy && x >= wx - 7) {
			int wX = x - wx + 7;
			int wY = line - wy;
			renderTile(x, line, wX, wY, windowTileMapArea());
		}
	}

	if (objEnable()) {
		renderSprites();
	}
}

void gbGpu::step() {

	LcdMode mode = static_cast<LcdMode>(stat & MODE_MASK);

	if (dmaCurrentByte > 0) {
		dmaCurrentByte--;
		UINT8 byte = g_gb->readByte(dmaStartAddr + dmaCurrentByte);
		writeByte(0xFE00 + dmaCurrentByte, byte);
	}

	if (!lcdEnabled()) return;

	ticks += 4;
	switch (mode) {
	case LcdMode::Search:
		if (ticks == 4 && (stat & LYC_INT) != 0 && lyc == ly) {
			g_gb->cpu->interruptFlags |= LCDSTAT_INTR;
		}

		if (ticks >= 80) {
			mode = LcdMode::Render;
			ticks = 0;
		}
		break;
	case LcdMode::Render:
		if (ticks >= 172) {
			mode = LcdMode::HBlank;
			ticks = 0;

			if (stat & HBLANK_INT) {
				g_gb->cpu->interruptFlags |= LCDSTAT_INTR;
			}
		}
		break;
	case LcdMode::HBlank:
		if (ticks < 204) {
			break;
		}

		renderLine();

		ticks = 0;
		ly++;

		if (ly == 144) {
			mode = LcdMode::VBlank;
			g_gb->cpu->interruptFlags |= VBLANK_INTR;
			if (stat & VBLANK_INT) {
				g_gb->cpu->interruptFlags |= LCDSTAT_INTR;
			}

			// Done rendering, copy to front buffer
			size_t size = sizeof(Color) * DISP_WIDTH * DISP_HEIGHT;
			memcpy(frontBuffer, backBuffer, size);
		}
		else {
			mode = LcdMode::Search;
			if (stat & OAM_INT) {
				g_gb->cpu->interruptFlags |= LCDSTAT_INTR;
			}
		}
		break;
	case LcdMode::VBlank:
		if (ticks < 456) {
			break;
		}

		ly++;
		ticks = 0;

		if (ly == 154) {
			ly = 0;
			mode = LcdMode::Search;
			if (stat & OAM_INT) {
				g_gb->cpu->interruptFlags |= LCDSTAT_INTR;
			}
		}
		break;
	}

	stat &= ~(LYC_FLAG | MODE_MASK);
	stat |= static_cast<int>(mode);
	stat |= (ly == lyc) << 2;
}

int gbGpu::writeByte(UINT16 addr, UINT8 val) {
	if (addr >= 0x8000 && addr < 0xA000) {
		vram[addr - 0x8000] = val;
		return 0;
	}

	// OAM Sprite Table
	if (addr >= 0xFE00 && addr < 0xFEA0) {
		oam.mem[addr - 0xFE00] = val;
		return 0;
	}

	if (addr == 0xFF40) {
		debugPrint("LCDC = 0x%x\n", val);
		if (lcdEnabled() && (val & 0x80) == 0) {
			ticks = 0;
			ly = 0;
			LcdMode mode = LcdMode::Search;
			stat &= ~(MODE_MASK);
			stat |= static_cast<int>(mode);
		}

		lcdc = val;
		return 0;
	}

	switch (addr) {
	case 0xFF41: stat = (stat & ~STAT_INTS) | (val & STAT_INTS); break;
	case 0xFF42: scy = val; break;
	case 0xFF43: scx = val; break;
	case 0xFF45: lyc = val; break;
	case 0xFF46: dmaStartAddr = val * 0x100; dmaCurrentByte = 160;  break;
	case 0xFF47: bgp = val; break;
	case 0xFF48: obp0 = val; break;
	case 0xFF49: obp1 = val; break;
	case 0xFF4A: wy = val; break;
	case 0xFF4B: wx = val; break;
	default: return -1;
	}

	return 0;
}

int gbGpu::readByte(UINT16 addr) {
	if (addr >= 0x8000 && addr < 0xA000) {
		return vram[addr - 0x8000];
	}

	if (addr >= 0xFE00 && addr < 0xFEA0) {
		return oam.mem[addr - 0xFE00];
	}

	switch (addr) {
	case 0xFF40: return lcdc;
	case 0xFF41: return stat;
	case 0xFF42: return scy;
	case 0xFF43: return scx;
	case 0xFF44: return ly;
	case 0xFF45: return lyc;
	case 0xFF46: return dmaStartAddr / 10;
	case 0xFF47: return bgp;
	case 0xFF48: return obp0;
	case 0xFF49: return obp1;
	case 0xFF4A: return wy;
	case 0xFF4B: return wx;
	default: return -1;
	}
}

void GameboyEmu::step() {
	timer->step();
	cpu->step();
	gpu->step();
	sound->step();
}