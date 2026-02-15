
#include <BaseTsd.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <limits>

#include <gb/gbcpu.h>
#include <gb/gbinput.h>
#include <gb/gbgpu.h>
#include <gb/gbtimer.h>
#include <gb/SDLWindow.h>
#include <gb/gb.h>

Rom* readRom() {
	PWSTR fileName;

	HRESULT hr = openFileMenu(&fileName);
	if (!SUCCEEDED(hr)) return nullptr;

	std::filesystem::path path(fileName);
	Rom* rom = createRom(path);
	return rom;
}

void printHeader(Rom* rom) {
	const CartHeader* hdr = rom->getHeader();
	char str[17]{ 0 }; // 16 char max + \0
	if (hdr->cgb.mode & 0x80) {
		memcpy(str, hdr->cgb.title, 11);
	}
	else {
		memcpy(str, hdr->title, 16);
	}

	debugPrint("RomName: %s\nRomType: %s\n",
		str,
		rom->getType().c_str()
	);
}

bool pause = false;

void handleSDLkey(SDL_KeyboardEvent& event, bool down) {
	gbInputType input;
	switch (event.keysym.sym) {
	case SDLK_w:
	case SDLK_UP:
		input = gbInputType::UP;
		break;
	case SDLK_s:
	case SDLK_DOWN:
		input = gbInputType::DOWN;
		break;
	case SDLK_a:
	case SDLK_LEFT:
		input = gbInputType::LEFT;
		break;
	case SDLK_d:
	case SDLK_RIGHT:
		input = gbInputType::RIGHT;
		break;
	case SDLK_z:
		input = gbInputType::START;
		break;
	case SDLK_x:
		input = gbInputType::SELECT;
		break;
	case SDLK_q:
		input = gbInputType::A;
		break;
	case SDLK_e:
		input = gbInputType::B;
		break;
	case SDLK_p:
		if (!down) {
			pause = !pause;
		}
		return;
	default: return;
	}

	g_gb->input->setKey(input, down);
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	SDL_Event event;
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
	SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
		debugPrint("Could not init SDL: %\n", SDL_GetError());
		return 1;
	}

	int res = SDL_GetDisplayMode(0, 0, &mode);
	if (res) return 1;

	debugPrint("%d FPS - %d cycle per frame\n", mode.refresh_rate, GB_HZ / mode.refresh_rate);
	int cyclesPerFrame = GB_HZ / mode.refresh_rate;

	Rom* rom = readRom();
	if (rom == nullptr) return 1;

	printHeader(rom);

	g_gb = new GameboyEmu(rom);

	char title[17]{ 0 };
	const CartHeader* hdr = rom->getHeader();
	if (hdr->cgb.mode & 0x80) {
		memcpy(title, hdr->cgb.title, 11);
	}
	else {
		memcpy(title, hdr->title, 16);
	}
	sdlWindow mgr(title);

	UINT64 lastUpdateTime = SDL_GetPerformanceCounter();
	bool running = true;

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) 
				running = false;
			if (event.type == SDL_KEYDOWN) 
				handleSDLkey(event.key, true);
			if (event.type == SDL_KEYUP)
				handleSDLkey(event.key, false);
			if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_MOVED)
					lastUpdateTime = SDL_GetPerformanceCounter();
			}
		}

		UINT64 currentTime = SDL_GetPerformanceCounter();
		UINT64 deltaCounter = currentTime - lastUpdateTime;
		float elapsed = deltaCounter / (float)SDL_GetPerformanceFrequency();

		if (!pause) {
			int cyclesLeft = std::min(GB_HZ * elapsed, cyclesPerFrame * 1.5f);
			while (cyclesLeft > 0) {
				cyclesLeft -= 4;
				g_gb->step();
			}
		}

		lastUpdateTime = currentTime;
		mgr.present();
	}

	delete g_gb;

	return 0;
}