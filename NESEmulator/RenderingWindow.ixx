#include <SDL.h>
#include <iostream>
#include <SDL_ttf.h>

export module RenderingWindow;

export class RenderingWindow {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* renderTexture; 
	//SDL_Texture* debugTexture;
	//SDL_Surface* text_surface;
	int window_width = 256;
	int window_height = 240;
	int screen_width = 256;
	int screen_height = 240;

	float scale_factor = 3;

	const int clearR = 0;
	const int clearG = 0;
	const int clearB = 0;

	int currentR = 0;
	int currentG = 0;
	int currentB = 0;

	int32_t pitch = 0;
	uint32_t* pixelBuffer = nullptr;

	bool texture_locked = false;
	//TTF_Font* Debug_Font;
public:
	RenderingWindow()
	{
		/*
		if (!SDL_WasInit(SDL_INIT_VIDEO)) {
			SDL_Init(SDL_INIT_VIDEO);
		}
		TTF_Init();
		Debug_Font = TTF_OpenFont(R"(C:\Users\Josh\source\Fonts\courier.ttf)", 24);
		*/
		//if (Debug_Font == NULL) std::cout << "error loading font\n";
		//std::cout << TTF_GetError();
		//text_surface = TTF_RenderUTF8_Solid(Debug_Font, "test", SDL_Color(255, 0, 255));
		//debugTexture = SDL_CreateTextureFromSurface(renderer, text_surface);
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
		SDL_CreateWindowAndRenderer(window_width * scale_factor, window_height * scale_factor, SDL_WINDOW_SHOWN, &window, &renderer);
		renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
		//debugTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
	}
	bool frame_markers_debug = false;
	
	void StartFrame()
	{
		//std::cout << "starting frame\n";
		SDL_LockTexture(renderTexture, NULL, (void**)&pixelBuffer, &pitch);
		texture_locked = true;
		pitch /= sizeof(int32_t);
		if (frame_markers_debug) {
			std::cout << "start frame\n";
		}
	}
	uint8_t frame_counter = 0;
	void EndFrame()
	{	
		//for (int i = 0; i < (10 * 10); i++) { //wtf???
		//	std::cout << (int)pixelBuffer[i] << "\n";
		//}
		SDL_UnlockTexture(renderTexture);
		texture_locked = false;
		SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
		//SDL_RenderCopy(renderer, debugTexture, NULL, NULL);
		SDL_RenderPresent(renderer);
		if (frame_markers_debug) {
			std::cout << "end frame\n";
		}
		frame_counter++;
		if (frame_counter == 10) {
			std::cout << std::flush;
			frame_counter = 0;
		}
		//std::cout << "end frame\n";
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