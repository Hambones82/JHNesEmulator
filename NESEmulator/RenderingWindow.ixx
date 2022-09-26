#include <SDL.h>
#include <iostream>

export module RenderingWindow;

export class RenderingWindow {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* renderTexture;
	int window_width = 1024;
	int window_height = 512;

	const int clearR = 0;
	const int clearG = 0;
	const int clearB = 0;
public:
	RenderingWindow()
	{
		if (!SDL_WasInit(SDL_INIT_VIDEO)) {
			SDL_Init(SDL_INIT_VIDEO);
		}
		SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_SHOWN, &window, &renderer);
	}
	void StartFrame()
	{
		SDL_RenderClear(renderer);
	}
	void EndFrame()
	{	
		SDL_RenderPresent(renderer);
	}
	void SetColor(int r, int g, int b, int a)
	{
		//std::cout << r << g << b << "\n";
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

	~RenderingWindow()
	{
		ShutDown();
	}


};