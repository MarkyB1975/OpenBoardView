#include "BVRFile.h"

#include "utils.h"
#include <assert.h>
#include <cmath>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>

char *nextfield(char *p) {
	if (p) {
		while ((*p != '\t') && (*p != '\0') && (*p != '\r') && (*p != '\n')) {
			p++;
		}
	}
	if (*p == '\t') p++;
	return p;
}

bool BVRFile::verifyFormat(std::vector<char> &buf) {
	return find_str_in_buf("BVRAW_FORMAT_1", buf);
}

BVRFile::BVRFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	char *saved_locale;
	char ppn[100] = {0};                        // previous part name
	saved_locale  = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE(file_buf != nullptr);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	int current_block = 0;

	char **lines = stringfile(file_buf);
	ENSURE(lines);

	while (*lines) {
		char *line = *lines;
		++lines;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		if (!strcmp(line, "<<Layout>>")) {
			//			fprintf(stderr,"HIT LAYOUT\n");
			current_block = 1;
			lines += 1; // Skip 1 unused lines before 1st layout
			continue;
		}
		if (!strcmp(line, "<<Pin>>")) {
			current_block = 2;
			//			fprintf(stderr,"HIT PIN, block = %d\n", current_block);
			lines += 1; // Skip 1 unused lines before 1st pin
			continue;
		}
		if (!strcmp(line, "<<Nail>>")) {
			//			fprintf(stderr,"HIT NAIL\n");
			current_block = 3;
			lines += 1; // Skip 1 unused lines before 1st nail
			continue;
		}

		char *p = line;
		char *s;

		switch (current_block) {
			case 1: { // Format
				BRDPoint point;
				double x = READ_DOUBLE();
				point.x = trunc(x * 1000); // OBV uses integers
				if (*p == ',') p++;
				double y = READ_DOUBLE();
				point.y = trunc(y * 1000);
				format.push_back(point);
			} break;

			case 2: { // Parts & Pins
				BRDPart part;
				BRDPin pin;

				part.name = READ_STR();

				char *loc = READ_STR();
				if (!strcmp(loc, "(T)"))
					part.type = 10; // SMD part on top
				else
					part.type = 5; // SMD part on bottom

				// If this is the first time we've seen this part
				if ((strcmp(ppn, part.name))) {
					part.end_of_pins = 0;
					parts.push_back(part);
					snprintf(ppn, sizeof(ppn), "%s", part.name);
				}

				pin.part = parts.size(); // the part this pin is associated with, is the last part on the vector

				/*int id =*/ READ_INT(); // uint
				/*char *name =*/ READ_STR();
				double posx = READ_DOUBLE();
				pin.pos.x = trunc(posx * 1000);
				double posy = READ_DOUBLE();
				pin.pos.y = trunc(posy * 1000);
				/*int layer =*/ READ_INT(); // uint
				pin.net = READ_STR();
				// pin.probe = READ_INT();
				//
				pins.push_back(pin);
				parts.back().end_of_pins = pins.size();
			} break;

			case 3: { // Nails
				BRDNail nail;

				p = nextfield(p);
				double posx = READ_DOUBLE();
				nail.pos.x = trunc(posx * 1000);
				double posy = READ_DOUBLE();
				nail.pos.y = trunc(posy * 1000);
				/*int type =*/ READ_INT(); // uint
				/*char *grid =*/ READ_STR();
				char *loc = READ_STR();
				if (!strcmp(loc, "(T)"))
					nail.side = 1;
				else
					nail.side = 2;

				/*char *netid =*/ READ_STR();
				nail.net = READ_STR();
				nail.pos.x = posx * 1000;
				nail.pos.y = posy * 1000;
				nails.push_back(nail);
				// nail.probe = READ_INT();
				//
			} break;

			default: continue;
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;
}
