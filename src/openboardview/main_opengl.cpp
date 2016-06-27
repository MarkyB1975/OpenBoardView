// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "BoardView.h"

#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <GL/gl3w.h>
#include <SDL.h>
#include "platform.h"
#include "resource.h"

int main(int argc, char **argv)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window *window = SDL_CreateWindow("Open Board Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	 SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    // Load Fonts
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("asset/FiraSans-Medium.ttf", 20.0f);

    BoardView app{};
	 app.History_set_filename("openboardview.history");
	 app.History_load();

    ImVec4 clear_color = ImColor(20, 20, 30);

    // Main loop
    bool done = false;
	 bool preload_required = false;
	
		if (argc == 2) {
					struct stat buffer;   
					fprintf(stderr,"checking argv[1]=%s\n",argv[1]);
					if ((stat(argv[1], &buffer) == 0)) { 
						preload_required = true;
					}
		}

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);

				if (event.type == SDL_DROPFILE) {
					// Validate the file before replacing the current one, not that we should have to, but always better to be safe
					struct stat buffer;   
					if (stat (event.drop.file, &buffer) == 0) { 
						app.LoadFile(strdup(event.drop.file));
					}
				}

            if (event.type == SDL_QUIT) done = true;
        }

		  if  (SDL_GetWindowFlags(window) & (SDL_WINDOW_MINIMIZED|SDL_WINDOW_HIDDEN)) { usleep(50000); continue; } // stops OVB/SDL consuming masses of CPU when it should be idling.

        ImGui_ImplSdlGL3_NewFrame(window);

		  // If we have a board to view being passed from command line, then "inject" it here.
		  if (preload_required) { 
			app.LoadFile(strdup(argv[1]));
			preload_required = false;
		  }

        app.Update();
        if (app.m_wantsQuit) {
            SDL_Event sdlevent;
            sdlevent.type = SDL_QUIT;
            SDL_PushEvent(&sdlevent);
        }

		  // Update the title of the SDL app if the board filename has changed. - PLD20160618
		  if (app.history_file_has_changed) {
			  char scratch[1024];
			  snprintf(scratch, sizeof(scratch),"Open Board Viewer - %s", app.history.history[0]);
			  SDL_SetWindowTitle( window, scratch );
			  app.history_file_has_changed = 0;
		  }


        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
