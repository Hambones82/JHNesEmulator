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

	int scale_factor = 4;

	const int clearR = 0;
	const int clearG = 0;
	const int clearB = 0;

	int currentR = 0;
	int currentG = 0;
	int currentB = 0;

	int32_t pitch = 0;
	uint32_t* pixelBuffer = nullptr;

	bool texture_locked = false;
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
		//std::cout << "starting frame\n";
		SDL_LockTexture(renderTexture, NULL, (void**)&pixelBuffer, &pitch);
		texture_locked = true;
		pitch /= sizeof(int32_t);
	}
	void EndFrame()
	{	
		//for (int i = 0; i < (10 * 10); i++) { //wtf???
		//	std::cout << (int)pixelBuffer[i] << "\n";
		//}
		SDL_UnlockTexture(renderTexture);
		texture_locked = false;
		SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	void SetColor(int r, int g, int b, int a)
	{
		//if(r != 0) 
			//std::cout << r << g << b << "\n";
		currentR = r;
		currentG = g;
		currentB = b;
	}
	void DrawPixel(int x, int y)
	{
		//only getting yellow???
		//std::cout << "trying to draw pixel\n";
		uint32_t color = ((currentR << 16) + (currentG << 8) + (currentB));
		//if (color != 0) std::cout << color << "\n";
		if (texture_locked) {
			//std::cout << "writing color: " << (int)color << "\n";
			pixelBuffer[x + y * window_width] = color;
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