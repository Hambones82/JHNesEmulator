#pragma once

// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important to understand: SDL_Renderer is an _optional_ component of SDL. We do not recommend you use SDL_Renderer
// because it provides a rather limited API to the end-user. We provide this backend for the sake of completeness.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "IMGui/imgui.h"
#include "IMGui/imgui_impl_sdl.h"
#include "IMGui/imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <SDL.h>
#include <string>
#include <sstream>
#include <iomanip>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

import AudioDriver;
import APU;
import NESCPU;
import NESCPU_state;

class IMGuiNES {
private:
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    SDL_Window* window;
    SDL_Renderer* renderer;

    //NES internals
    const AudioDriver& audioDriver;
    const APU& apu;
    const NESCPU& cpu;
public:
    IMGuiNES(const AudioDriver& in_audioDriver,
             const APU& in_APU,
             const NESCPU& in_CPU) :
        audioDriver(in_audioDriver),
        apu(in_APU),
        cpu(in_CPU)
    {

    }
    void IMGuiInitialize(SDL_Window* in_window, SDL_Renderer* in_renderer) {      
        // Setup SDL
        // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
        // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to the latest version of SDL is recommended!)
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            return;
        }

        window = in_window;
        renderer = in_renderer;

        //SDL_RendererInfo info;
        //SDL_GetRendererInfo(renderer, &info);
        //SDL_Log("Current SDL_Renderer: %s", info.name);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        //NEED TO GET WINDOW AND RENDERER HERE
        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer_Init(renderer);


        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        //DONT DO A LOOP, THIS WILL BE CALLED, MAYBE AS "UPDATE"
        // Main loop
        //bool done = false;
        //while (!done)
        //{
            
            /*
            
            */
        //}

    }

    void DrawNESIMGui(SDL_Texture *NESTexture) {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            ImGui_ImplSDL2_ProcessEvent(&event);
            /*
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
                */
        }

        // Our state
        bool show_demo_window = true;
        bool show_another_window = false;


        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //NES Image
        {
            ImGui::Begin("Game");
            ImGui::Image(NESTexture, ImVec2(256 * 3, 240 * 3));
            ImGui::End();
        }

        
        //Info Window
        {
            ImGui::Begin("Info");                          // Create a window called "Hello, world!" and append into it.
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Regs");
            const int strWidth = 6;
            std::ostringstream labelText;
            labelText << std::setw(strWidth) << "X"
                << std::setw(strWidth) << "Y"
                << std::setw(strWidth) << "A"
                << std::setw(strWidth) << "PC";

            ImGui::Text(labelText.str().c_str());
            auto cpuState = cpu.GetStateObj();
            std::ostringstream regValues;
            regValues << std::setw(strWidth) << std::to_string(cpuState.X)
                << std::setw(strWidth) << std::to_string(cpuState.Y)
                << std::setw(strWidth) << std::to_string(cpuState.A)
                << std::setw(strWidth) << std::to_string(cpuState.PC);
            ImGui::Text(regValues.str().c_str());

            std::ostringstream square1Length;
            ImGui::Text("APU");
            ImGui::Text("Square 1");
            ImGui::Text("Length:");
            square1Length << std::setw(strWidth) << std::to_string(apu.GetAPUData().square1Data.length_counter);
            ImGui::Text(square1Length.str().c_str());

            std::ostringstream square2Length;
            ImGui::Text("Square 2");
            ImGui::Text("Length:");
            square2Length << std::setw(strWidth) << std::to_string(apu.GetAPUData().square2Data.length_counter);
            ImGui::Text(square2Length.str().c_str());

            std::ostringstream triangleLength;
            ImGui::Text("Triangle");
            ImGui::Text("Length:");
            triangleLength << std::setw(strWidth) << std::to_string(apu.GetAPUData().triangleData.length_counter);
            ImGui::Text(triangleLength.str().c_str());

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    void StopNESIMGui() {

        // Cleanup
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};


