#pragma once

#include "Board.h"
#include <array>
#include <stdlib.h>
#include <string>
#include <vector>

#define ENSURE(X) assert(X);
#define READ_INT() strtol(p, &p, 10);
#define READ_UINT [&]() {                                    \
		int t = strtol(p, &p, 10);                   \
		ENSURE(t >=0);                               \
		return static_cast<unsigned int>(t);         \
	}
#define READ_DOUBLE() strtod(p, &p);
#define READ_STR [&]() {                                     \
		while ((*p) && (isspace((uint8_t)*p))) ++p;  \
		s = p;                                       \
		while ((*p) && (!isspace((uint8_t)*p))) ++p; \
		*p = 0;                                      \
		p++;                                         \
		return fix_to_utf8(s, &arena, arena_end);    \
	}

static constexpr std::array<uint8_t, 4> signature = {0x23, 0xe2, 0x63, 0x28};

struct BRDPoint {
	int x;
	int y;
};

struct BRDPart {
	const char *name;
	unsigned int type;
	unsigned int end_of_pins;
	BRDPoint p1{0, 0};
	BRDPoint p2{0, 0};
};

struct BRDPin {
	BRDPoint pos;
	unsigned int probe;
	unsigned int part;
	unsigned int side = 0;
	const char *net;
	double radius = 0.5f;
	const char *snum = nullptr;

	bool operator<(const BRDPin &p) const // For sorting the vector
	{
		return part == p.part ? (std::string(snum) < std::string(p.snum)) : (part < p.part); // sort by part number then pin number
	}
};

struct BRDNail {
	unsigned int probe;
	BRDPoint pos;
	unsigned int side;
	const char *net;
};

class BRDFile {
  public:
	unsigned int num_format = 0;
	unsigned int num_parts = 0;
	unsigned int num_pins = 0;
	unsigned int num_nails = 0;

	std::vector<BRDPoint> format;
	std::vector<BRDPart> parts;
	std::vector<BRDPin> pins;
	std::vector<BRDNail> nails;

	char *file_buf = nullptr;

	bool valid = false;

	BRDFile(const char *buf, size_t buffer_size);
	BRDFile(){};
	~BRDFile() {
		free(file_buf);
	}

	static bool verifyFormat(const char *buf, size_t buffer_size);

  private:
	static constexpr std::array<uint8_t, 4> signature = {0x23, 0xe2, 0x63, 0x28};
};

char **stringfile(char *buffer);
char *fix_to_utf8(char *s, char **arena, char *arena_end);
