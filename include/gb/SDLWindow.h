#pragma once
#include <SDL.h>
#include <gb/gbgpu.h>
#include <gb/gb.h>

class sdlWindow {
public:
	sdlWindow(const char* name) {
		int rmask = 0x000000ff;
		int gmask = 0x0000ff00;
		int bmask = 0x00ff0000;
		int amask = 0xff000000;

		int width = DISP_WIDTH;
		int height = DISP_HEIGHT;

		screen = SDL_CreateWindow(
			name,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			width * 4,
			height * 4,
			0);

		if (!screen) {
			debugPrint("Could not create window\n");
			exit(1);
		}

		renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (!renderer) {
			debugPrint("Could not create renderer\n");
			exit(1);
		}

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
	}

	void present();

	~sdlWindow() {
		SDL_DestroyWindow(screen);
		SDL_Quit();
	}

private:
	SDL_Window* screen;
	SDL_Texture* texture;
	SDL_Renderer* renderer;
};