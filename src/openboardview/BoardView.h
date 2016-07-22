#pragma once

#include "Board.h"
#include "confparse.h"
#include "history.h"
#include "imgui/imgui.h"
#include <stdint.h>
#include <vector>

struct BRDPart;
struct BRDFile;

struct BitVec {
	uint32_t *m_bits;
	// length of the BitVec in bits
	uint32_t m_size;

	~BitVec();
	void Resize(uint32_t new_size);

	bool operator[](uint32_t index) {
		return 0 != (m_bits[index >> 5] & (1u << (index & 0x1f)));
	}

	void Set(uint32_t index, bool val) {
		uint32_t &slot = m_bits[index >> 5];
		uint32_t bit = (1u << (index & 0x1f));
		slot &= ~bit;
		if (val) {
			slot |= bit;
		}
	}

	void Clear() {
		uint32_t num_ints = m_size >> 5;
		for (uint32_t i = 0; i < num_ints; i++) {
			m_bits[i] = 0;
		}
	}
};

struct ColorScheme {
	uint32_t backgroundColor = 0xa0000000;
	uint32_t partTextColor = 0xff808000;
	uint32_t boardOutline = 0xff0000ff;

	uint32_t boxColor = 0xffcccccc;

	uint32_t pinDefault = 0xffff0000;
	uint32_t pinGround = 0xffdd0000;
	uint32_t pinNotConnected = 0xffdd0000;
	uint32_t pinTestPad = 0xff888888;

	uint32_t pinSelected         = 0xffeeeeee;
	uint32_t pinHalo             = 0x8f00ff00;
	uint32_t pinHighlighted = 0xffffffff;
	uint32_t pinHighlightSameNet = 0xff99f8ff;

	uint32_t annotationPartAlias = 0xcc00ffff;

  uint32_t partHullColor = 0x80808080;
  uint32_t selectedMaskPins = 0x4FFFFFFF;
  uint32_t selectedMaskParts = 0x8FFFFFFF;
  uint32_t selectedMaskOutline = 0x8FFFFFFF;
};

enum DrawChannel { kChannelImages = 0, kChannelPolylines = 1, kChannelText = 2, kChannelAnnotations = 3, NUM_DRAW_CHANNELS = 4 };

struct BoardView {
	BRDFile *m_file;
	Board *m_board;

	Confparse obvconfig;
	FHistory fhistory;
	int history_file_has_changed = 0;
	float zoomFactor             = 0.5f;
	int zoomModifier             = 5;
	int panFactor                = 30;
	int panModifier              = 5;
	float pinSizeThresholdLow    = 0.0f;
	bool pinShapeSquare          = false;
	bool pinShapeCircle          = true;
	bool slowCPU = false;
	bool showFPS = false;
	bool pinHalo = true;
	bool showPosition			= true;
	int pinBlank				= 0;
	uint32_t FZKey[44]				= {0};

	int ConfigParse(void);
	uint32_t byte4swap(uint32_t x);
	void CenterView(void);
	void Pan( int direction, int amount );
	void Zoom( float osd_x, float osd_y, float zoom );
	void DrawDiamond( ImDrawList *draw, ImVec2 c, double r, uint32_t color );
	void DrawHex( ImDrawList *draw, ImVec2 c, double r, uint32_t color );
	void DrawBox( ImDrawList *draw, ImVec2 c, double r, uint32_t color );
	void SetFZKey( char *keytext );
	void HelpAbout(void);
	void HelpControls(void);
	void SearchNet(void);
	void SearchComponent(void);

	Pin *m_pinSelected = nullptr;
	vector<Pin *> m_pinHighlighted;
	vector<Component *> m_partHighlighted;
	char m_cachedDrawList[sizeof(ImDrawList)];
	ImVector<char> m_cachedDrawCommands;
	SharedVector<Net> m_nets;
	char m_search[128];
  char m_search2[128];
  char m_search3[128];
	char m_netFilter[128];
	char *m_lastFileOpenName;
  float m_dx; // display top-right coordinate?
	float m_dy;
  float m_mx; // board *maxiumum* size? scaled relative to m_boardwidth/height
	float m_my;
	float m_scale = 1.0f;
	float m_lastWidth; // previously checked on-screen window size; use to redraw
	                   // when window is resized?
	float m_lastHeight;
  int m_rotation; // set to 0 for original orientation [0-4]
	int m_current_side;
  int m_boardWidth; // board size in what coordinates? thou?
	int m_boardHeight;

	ColorScheme m_colors;

	// TODO: save settings to disk
	// pinDiameter: diameter for all pins.  Unit scale: 1 = 0.025mm, boards are
	// done in "thou" (1/1000" = 0.0254mm)
	int m_pinDiameter = 20;
	bool m_flipVertically = true;

	// Annotation layer specific
	bool m_annotationsVisible = true;

	// Do a hacky thing (memcpy the window draw list) to save us from rerendering
	// the board
	// The app will crash or break if this flag is not set when it should be.
	bool m_needsRedraw = true;
	bool m_draggingLastFrame;
	bool m_showNetfilterSearch;
	bool m_showComponentSearch;
	bool m_showNetList;
	bool m_showPartList;
	bool m_showHelpAbout;
	bool m_showHelpControls;
	bool m_firstFrame = true;
	bool m_lastFileOpenWasInvalid;
	bool m_wantsQuit;

	~BoardView();

	void ShowNetList(bool *p_open);
	void ShowPartList(bool *p_open);

	void Update();
	void HandleInput();
	void RenderOverlay();
	void DrawOutline(ImDrawList *draw);
	void DrawPins(ImDrawList *draw);
	void DrawParts(ImDrawList *draw);
	void DrawBoard();
	void SetFile(BRDFile *file);
  int LoadFile(char *filename);
	ImVec2 CoordToScreen(float x, float y, float w = 1.0f);
	ImVec2 ScreenToCoord(float x, float y, float w = 1.0f);
	// void Move(float x, float y);
	void Rotate(int count);

	// Sets the center of the screen to (x,y) in board space
	void SetTarget(float x, float y);

	// Returns true if the part is shown on the currently displayed side of the
	// board.
	bool ComponentIsVisible(const Component *part);
	bool IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io);
	// Returns true if the circle described by screen coordinates x, y, and radius
	// is visible in the
	// ImGuiIO screen rect.
	// bool IsVisibleScreen(float x, float y, float radius = 0.0f);

	bool PartIsHighlighted(const Component &component);
	void SetNetFilter(const char *net);
	void FindComponent(const char *name);
  void FindComponentNoClear(const char *name );
	void SetLastFileOpenName(char *name);
	void FlipBoard();
};
