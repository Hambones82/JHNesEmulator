#include <SDL.h>
#include <iostream>
#include <SDL_ttf.h>
#include <chrono>

import NESConstants;

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


	uint32_t obuffer1[256*240] = { 0 };
	uint32_t obuffer2[256*240] = { 0 };
	bool usebuffer1 = true;

	bool texture_locked = false;
	std::chrono::time_point<std::chrono::system_clock> prev_frame_time;
	std::chrono::duration<double> frame_time{ 1. / (FPS * RUNTIME_SPEED_MULTIPLIER) };
	//std::chrono::time_point<std::chrono::system_clock> next_frame_time;
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
		prev_frame_time = std::chrono::system_clock::now();
		//debugTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
	}
	bool frame_markers_debug = false;
	
	void StartFrame()
	{
		//std::cout << "starting frame\n";
		//SDL_LockTexture(renderTexture, NULL, (void**)&pixelBuffer, &pitch);
		//texture_locked = true;
		//pitch /= sizeof(int32_t);
		if (frame_markers_debug) {
			std::cout << "start frame\n";
		}
		usebuffer1 = !usebuffer1;

	}
	uint8_t frame_counter = 0;
	
	void EndFrame()
	{	
		//for (int i = 0; i < (10 * 10); i++) { //wtf???
		//	std::cout << (int)pixelBuffer[i] << "\n";
		//}
		//uint8_t* current_buffer = usebuffer1 ? obuffer1 : obuffer2;

		if (true) {
			SDL_LockTexture(renderTexture, NULL, (void**)&pixelBuffer, &pitch);
			for (int i = 0; i < 240 * 256; i++) {
				pixelBuffer[i] = usebuffer1 ? obuffer1[i] : obuffer2[i];
			}
			SDL_UnlockTexture(renderTexture);

			SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
			//SDL_RenderCopy(renderer, debugTexture, NULL, NULL);
			SDL_RenderPresent(renderer);
			//timing
			std::chrono::duration<double, std::milli> time_since_last_frame = 
				std::chrono::system_clock::now() - prev_frame_time;
			if (time_since_last_frame < frame_time) {
				//std::chrono::duration<double, std::milli> sleep_time = frame_time - time_since_last_frame;
				//std::this_thread::sleep_for(sleep_time); 
				std::this_thread::sleep_until(prev_frame_time + frame_time);
				prev_frame_time = prev_frame_time + std::chrono::duration_cast<std::chrono::milliseconds>(frame_time);
			}

			else {
				prev_frame_time = std::chrono::system_clock::now();
			}

		}
		
		if (frame_markers_debug) {
			std::cout << "end frame\n";
		}
		frame_counter++;
		if (frame_counter == 10) {
			std::cout << std::flush;
			frame_counter = 0;
		}
	}
	void SetColor(int r, int g, int b, int a)
	{
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
		if (x + y * 256 > 240 * 256) { std::cout << "draw out bounds error\n"; }
		if (usebuffer1) {
			obuffer1[x + y * 256] = color;
		}
		else {
			obuffer2[x + y * 256] = color;
		}
		/*
		if (texture_locked) {
			//std::cout << "writing color: " << (int)color << "\n";
			pixelBuffer[x + y * window_width] = color;
		}*/
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