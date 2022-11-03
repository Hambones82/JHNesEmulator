module;
#include <SDL.h>
#include <iostream>
#include <SDL_ttf.h>
#include <chrono>
#include "IMGuiNES.h"

export module IMGuiRenderingWindow;
import NESConstants;

export class IMGuiRenderingWindow {
private:
	IMGuiNES imGUINESRenderer;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* renderTexture;
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


	uint32_t obuffer1[256 * 240] = { 0 };
	uint32_t obuffer2[256 * 240] = { 0 };
	bool usebuffer1 = true;

	bool texture_locked = false;
	std::chrono::time_point<std::chrono::system_clock> prev_frame_time;
	std::chrono::duration<double> frame_time{ 1. / (FPS * RUNTIME_SPEED_MULTIPLIER) };
public:
	IMGuiRenderingWindow()
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
		//SDL_CreateWindowAndRenderer(window_width * scale_factor, window_height * scale_factor, SDL_WINDOW_SHOWN, &window, &renderer);
		// Setup window
		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
		// Setup SDL_Renderer instance
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
		if (renderer == NULL)
		{
			SDL_Log("Error creating SDL_Renderer!");
			return;
		}

		imGUINESRenderer.IMGuiInitialize(window, renderer);

		//not sure about the next line -- i think they are needed as the output texture to show as an image...
		renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
		prev_frame_time = std::chrono::system_clock::now();
	}
	bool frame_markers_debug = false;

	void StartFrame()
	{
		if (frame_markers_debug) {
			std::cout << "start frame\n";
		}
		usebuffer1 = !usebuffer1;

	}
	uint8_t frame_counter = 0;

	void EndFrame()
	{
		
		
		SDL_LockTexture(renderTexture, NULL, (void**)&pixelBuffer, &pitch);
		for (int i = 0; i < 240 * 256; i++) {
			pixelBuffer[i] = usebuffer1 ? obuffer1[i] : obuffer2[i];
		}
		SDL_UnlockTexture(renderTexture);


		imGUINESRenderer.DrawNESIMGui(renderTexture);
		//SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
		//SDL_RenderPresent(renderer);
		//timing
		std::chrono::duration<double, std::milli> time_since_last_frame =
			std::chrono::system_clock::now() - prev_frame_time;
		if (time_since_last_frame < frame_time) {
			std::this_thread::sleep_until(prev_frame_time + frame_time);
			prev_frame_time = prev_frame_time + std::chrono::duration_cast<std::chrono::microseconds>(frame_time);
		}
		else {
			prev_frame_time = std::chrono::system_clock::now();
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
		uint32_t color = ((currentR << 16) + (currentG << 8) + (currentB));
		if (x + y * 256 > 240 * 256) { std::cout << "draw out bounds error\n"; }
		if (usebuffer1) {
			obuffer1[x + y * 256] = color;
		}
		else {
			obuffer2[x + y * 256] = color;
		}
	}

	void ShutDown()
	{
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	~IMGuiRenderingWindow()
	{
		ShutDown();
	}


};