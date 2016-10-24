#ifndef _WIN32

#define _CRT_SECURE_NO_WARNINGS 1
#include "imgui/imgui.h"
#include "platform.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdint.h>

#ifndef __APPLE__
#include <gtk/gtk.h>
#endif

char *file_as_buffer(size_t *buffer_size, const char *utf8_filename) {
	std::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Error opening " << utf8_filename << ": " << strerror(errno) << std::endl;
		*buffer_size = 0;
		return nullptr;
	}

	std::streampos sz = file.tellg();
	*buffer_size      = sz;
	char *buf         = (char *)malloc(sz);
	file.seekg(0, std::ios::beg);
	file.read(buf, sz);
	assert(file.gcount() == sz);
	file.close();

	return buf;
}

#ifndef __APPLE__
char *show_file_picker() {
	char *path = nullptr;
	GtkWidget *parent, *dialog;
	GtkFileFilter *filter            = gtk_file_filter_new();
	GtkFileFilter *filter_everything = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Boards");
	gtk_file_filter_add_pattern(filter, "*.brd");
	gtk_file_filter_add_pattern(filter, "*.bdv");
	gtk_file_filter_add_pattern(filter, "*.bvr");
	gtk_file_filter_add_pattern(filter, "*.fz");

	gtk_file_filter_set_name(filter_everything, "All");
	gtk_file_filter_add_pattern(filter_everything, "*");

	if (!gtk_init_check(NULL, NULL)) return nullptr;

	parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	dialog = gtk_file_chooser_dialog_new("Open File",
	                                     GTK_WINDOW(parent),
	                                     GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "_Cancel",
	                                     GTK_RESPONSE_CANCEL,
	                                     "_Open",
	                                     GTK_RESPONSE_ACCEPT,
	                                     NULL);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_everything);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename   = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		size_t len = strlen(filename);
		path       = (char *)malloc(len + 1);
		memcpy(path, filename, len + 1);
		if (!path) {
			g_free(filename);
			gtk_widget_destroy(dialog);
			return nullptr;
		}
		g_free(filename);
	}

	while (gtk_events_pending()) gtk_main_iteration();

	gtk_widget_destroy(dialog);

	while (gtk_events_pending()) gtk_main_iteration();

	return path;
}

std::string get_asset_path(const char *asset) {
	std::string path = "asset";
	path += "/";
	path += asset;
	return path;
}
#endif

ImTextureID TextureIDs[NUM_GLOBAL_TEXTURES];

#endif
