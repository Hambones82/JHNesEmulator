#include <SDL.h>
export module NESRenderer;
//Screen dimension constants
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 480;

export class NESRenderer {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* renderTexture;
public:
	NESRenderer()
	{
		if (!SDL_WasInit(SDL_INIT_VIDEO)) {
			SDL_Init(SDL_INIT_VIDEO);
		}
		SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
		renderTexture =
			SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 50, 50);
	}
	
	void StartFrame()
	{
		SDL_SetRenderTarget(renderer, renderTexture);
	}
	void EndFrame()
	{
		SDL_RenderPresent(renderer);

		SDL_SetRenderTarget(renderer, NULL);

		SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	void SetColor(int r, int g, int b, int a)
	{
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}
	void DrawPixel(int x, int y)
	{
		SDL_RenderDrawPoint(renderer, x, y);
	}

	void ShutDown()
	{
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	~NESRenderer()
	{
		ShutDown();
	}


};

