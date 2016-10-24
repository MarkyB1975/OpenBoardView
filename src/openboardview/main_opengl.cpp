// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "history.h"
#include "BoardView.h"

#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <GL/gl3w.h>
#include <SDL2/SDL.h>
#include "platform.h"
#include "resource.h"
#include "confparse.h"
uint32_t byte4swap( uint32_t x ) {
	/*
	 * used to convert RGBA -> ABGR etc
	 */
	return (
		(( x & 0x000000ff ) << 24)
		| (( x & 0x0000ff00 ) << 8 )
		| (( x & 0x00ff0000 ) >> 8)
		| (( x & 0xff000000 ) >> 24)
	  	);
}

int main(int argc, char **argv)
{
	Confparse obvconfig;
	int sizex, sizey;

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}


	obvconfig.Load("openboardview.conf");
	sizex = obvconfig.ParseInt("windowX", 1280);
	sizey = obvconfig.ParseInt("windowY", 900);

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
	SDL_Window *window = SDL_CreateWindow("OpenFlex Board Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, sizex, sizey, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	gl3wInit();

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(window);

#ifdef CUSTOM_FONT
	// Load Fonts
	ImGuiIO &io = ImGui::GetIO();
	std::string fontpath = get_asset_path("FiraSans-Medium.ttf");
	io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 20.0f);
#endif
	ImGuiIO &io = ImGui::GetIO();
	std::string fontpath = get_asset_path(obvconfig.ParseStr("fontPath","DroidSans.ttf"));
	io.Fonts->AddFontFromFileTTF(fontpath.c_str(), obvconfig.ParseDouble("fontSize",20.0f));
	//	io.Fonts->AddFontDefault();
	

	BoardView app{};
	app.fhistory.Set_filename("openboardview.history");
	app.fhistory.Load();

	/*
	 * Some machines (Atom etc) don't have enough CPU/GPU
	 * grunt to cope with the large number of AA'd circles
	 * generated on a large dense board like a Macbook Pro
	 * so we have the lowCPU option which will let people
	 * trade good-looks for better FPS
	 */
	app.slowCPU = obvconfig.ParseBool("slowCPU", false);
	if (app.slowCPU == true) {
		ImGuiStyle &style = ImGui::GetStyle();
		style.AntiAliasedShapes = false;
	}

	app.showFPS = obvconfig.ParseBool("showFPS", false);

	/*
	 * Colours in ImGui can be represented as a 4-byte packed uint32_t as ABGR
	 * but most humans are more accustomed to RBGA, so for the sake of readability
	 * we use the human-readable version but swap the ordering around when 
	 * it comes to assigning the actual colour to ImGui.
	 */
	app.m_colors.backgroundColor	= byte4swap(obvconfig.ParseHex("backgroundColor", 0xa0000000));
	app.m_colors.partTextColor		= byte4swap(obvconfig.ParseHex("partTextColor", 0xff808000));
	app.m_colors.boardOutline		= byte4swap(obvconfig.ParseHex("boardOutline", 0xffff00ff));
	app.m_colors.boxColor			= byte4swap(obvconfig.ParseHex("boxColor", 0xffcccccc)); 
	app.m_colors.pinDefault			= byte4swap(obvconfig.ParseHex("pinDefault",0xff0000ff));
	app.m_colors.pinGround			= byte4swap(obvconfig.ParseHex("pinGround",0xff0000bb));
	app.m_colors.pinNotConnected	= byte4swap(obvconfig.ParseHex("pinNotConnected", 0xffff0000));
	app.m_colors.pinTestPad			= byte4swap(obvconfig.ParseHex("pinTestPad", 0xff888888));
	app.m_colors.pinSelected		= byte4swap(obvconfig.ParseHex("pinSelected", 0xffeeeeee));
	app.m_colors.pinHighlighted	= byte4swap(obvconfig.ParseHex("pinHighlighted", 0xffffffff));
	app.m_colors.pinHighlightSameNet = byte4swap(obvconfig.ParseHex("pinHighlightSameNet", 0xff88f8ff));
	app.m_colors.annotationPartAlias = byte4swap(obvconfig.ParseHex("annotationPartAlias", 0xff00ffff));
	app.m_colors.partHullColor		= byte4swap(obvconfig.ParseHex("partHullColor", 0x80808080));

	ImVec4 clear_color = ImColor(20, 20, 30);

	// Main loop
	bool done = false;
	bool preload_required = false;

	if (argc == 2) {
		struct stat buffer;   
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
			snprintf(scratch, sizeof(scratch),"OpenFlex Board Viewer - %s", app.fhistory.history[0]);
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
