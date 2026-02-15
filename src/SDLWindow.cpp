#include <gb/SDLWindow.h>
#include <gb/gbgpu.h>

void sdlWindow::present() {

	SDL_RenderClear(renderer);
	Color* framebuffer = nullptr;
	int pitch = 0;
	if (g_gb->gpu->lcdEnabled()) {
		SDL_LockTexture(texture, NULL, (void**)&framebuffer, &pitch);

		for (int y = 0; y < DISP_HEIGHT; y++) {
			for (int x = 0; x < DISP_WIDTH; x++) {
				framebuffer[(y * DISP_WIDTH) + x] = g_gb->gpu->frontBuffer[y][x];
			}
		}

		SDL_UnlockTexture(texture);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
	}
	SDL_RenderPresent(renderer);
}