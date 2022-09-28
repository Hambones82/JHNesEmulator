#include <SDL.h>
#include <iostream>

export module RenderingWindow;

export class RenderingWindow {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* renderTexture; 
	int window_width = 256;
	int window_height = 240;

	int scale_factor = 4;//not performant...
	//change to draw to surface/texture

	const int clearR = 0;
	const int clearG = 0;
	const int clearB = 0;
public:
	RenderingWindow()
	{
		if (!SDL_WasInit(SDL_INIT_VIDEO)) {
			SDL_Init(SDL_INIT_VIDEO);
		}
		SDL_CreateWindowAndRenderer(window_width * scale_factor, window_height * scale_factor, SDL_WINDOW_SHOWN, &window, &renderer);
		renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
	}
	void StartFrame()
	{
		
	}
	void EndFrame()
	{	
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}
	void SetColor(int r, int g, int b, int a)
	{
		//std::cout << r << g << b << "\n";
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}
	void DrawPixel(int x, int y)
	{
		//this code isn't correct...
		for (int l_x = 0; l_x < scale_factor; l_x++) {
			for (int l_y = 0; l_y < scale_factor; l_y++) {
				SDL_RenderDrawPoint(renderer, x * scale_factor + l_x, y * scale_factor + l_y); 
			}
		}
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