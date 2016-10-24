#include "FZFile.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <algorithm>
#include <zlib.h>

// Decoding an .fz file. You still need the key of course.
// https://en.wikipedia.org/wiki/RC6 here you can read it all up.

// Looking at the paper the algo is straight forward, not sure about endianness.
// This will put out some .bin file you would decompress using zlib.
//constexpr uint32_t FZFile::key[44];
uint32_t FZFile::key[44];

static inline uint32_t rotl32(uint32_t a, int32_t b) {
	return (a << b) | (a >> (32 - b));
}

/*
 * Decrypt an RC6 encrypted buffer using key
 */
void FZFile::decode(char *source, size_t size) {
	// Along the lines of http://people.csail.mit.edu/rivest/pubs/RRSY98.pdf
	// (page 3, 2.2)
	int32_t logw = 5;
	uint32_t r = 20;

	uint32_t A = 0;
	uint32_t B = 0;
	uint32_t C = 0;
	uint32_t D = 0;

	uint8_t currentByte;
	uint8_t ibuf[16] = {0}; // you'll see

	// RC6 algo from the paper, basically 1:1
	for (size_t pos = 0; pos < size; ++pos) {
		B = B + key[0];
		D = D + key[1];
		for (uint32_t i = 1; i < (r + 1); ++i) { // loop offset by 1
			uint32_t t = rotl32(B * (2 * B + 1), logw);
			uint32_t u = rotl32(D * (2 * D + 1), logw);
			A = rotl32(A ^ t, u) + key[2 * i];
			C = rotl32(C ^ u, t) + key[2 * i + 1];

			uint32_t tmp = A;
			A = B;
			B = C;
			C = D;
			D = tmp;
		}
		A = A + key[2 * r + 2];
		C = C + key[2 * r + 3]; // not used I guess

		// rolling over the the uint8_t string
		// buf[pos] xor A -> is our resulting byte
		currentByte = source[pos];
		source[pos] = ((uint8_t)(currentByte ^ (A & 0xFF)));

		// pushing in a stream of buf[pos] chars 'from the right'
		// and shift whole array 8 bits to the left
		for (uint32_t i = 0; i < 15; ++i) {
			ibuf[i] = ibuf[i + 1];
		}
		ibuf[15] = currentByte;

		// align 4 consequent int32s to that buffer
		// (A, B, C, D) = (buf[0], buf[1], buf[2], buf[3])
		// byte order?!
		A = ibuf[0] | ibuf[1] << 8 | ibuf[2] << 16 | ibuf[3] << 24;
		B = ibuf[4] | ibuf[5] << 8 | ibuf[6] << 16 | ibuf[7] << 24;
		C = ibuf[8] | ibuf[9] << 8 | ibuf[10] << 16 | ibuf[11] << 24;
		D = ibuf[12] | ibuf[13] << 8 | ibuf[14] << 16 | ibuf[15] << 24;
	}
}

/*
 * Sets content_size to the length of the compressed content from the decoded fz
 * file
 */
char *FZFile::split(char *file_buf, size_t buffer_size, size_t &content_size, char *&descr, size_t &descr_size) {
	unsigned char *ufile_buf = reinterpret_cast<unsigned char*>(file_buf);
	auto len = (ufile_buf[buffer_size - 1] << 24) + (ufile_buf[buffer_size - 2] << 16) + (ufile_buf[buffer_size - 3] << 8) +
	                ufile_buf[buffer_size - 4]; // read last 4 bytes as little endian 32-bit int.
	if (len < 0 || static_cast<unsigned>(len) > buffer_size) return nullptr;
	descr_size = static_cast<unsigned>(len);
	content_size = buffer_size - descr_size + 4;
	descr = file_buf + content_size;
	return file_buf + 4;
}

/*
 * Inflates the zlib compressed data from buffer
 */
char *FZFile::decompress(char *file_buf, size_t buffer_size, size_t &output_size) {
	output_size = buffer_size;
	if (buffer_size == 0) return nullptr;

	char *output = (char *)calloc(output_size, sizeof(char));

	z_stream zst;
	zst.next_in = (Bytef *)file_buf;
	zst.avail_in = buffer_size;
	zst.total_out = 0;
	zst.zalloc = Z_NULL;
	zst.zfree = Z_NULL;

	if (inflateInit(&zst) != Z_OK) {
		free(output);
		return nullptr;
	}

	int ret;
	do {
		// If our output buffer is too small
		if (zst.total_out >= output_size) {
			// Increase size of output buffer
			char *buf = (char *)calloc(output_size + buffer_size/2, sizeof(char));
			memcpy(buf, output, output_size);
			output_size += buffer_size/2;
			free(output);
			output = buf;
		}

		zst.next_out = (Bytef *)(output + zst.total_out);
		zst.avail_out = output_size - zst.total_out;
	} while ((ret = inflate(&zst, Z_SYNC_FLUSH)) == Z_OK);

	if (ret != Z_STREAM_END) printf("Error %d: %s\n", ret, zst.msg);

	if (inflateEnd(&zst) != Z_OK) {
		free(output);
		return nullptr;
	}

	return output;
}

/*
 * Creates fake points for outline using outermost pins plus some margin.
 */
#define OUTLINE_MARGIN 20
void FZFile::gen_outline() {
	// Determine board outline
	int minx =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x - OUTLINE_MARGIN;
	int maxx =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x + OUTLINE_MARGIN;
	int miny =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y - OUTLINE_MARGIN;
	int maxy =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y + OUTLINE_MARGIN;
	format.push_back({minx, miny});
	format.push_back({maxx, miny});
	format.push_back({maxx, maxy});
	format.push_back({minx, maxy});
	format.push_back({minx, miny});
}
#undef OUTLINE_MARGIN

/*
 * Updates element counts
 */
void FZFile::update_counts() {
	num_parts = parts.size();
	num_pins = pins.size();
	num_format = format.size();
	num_nails = nails.size();
}

FZFile::FZFile(std::vector<char> &buf, uint32_t *fzkey) {
	auto buffer_size = buf.size();
	char *saved_locale;
	float multiplier = 1.0f;
	saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	memcpy(key, fzkey, sizeof(key));

	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE(file_buf != nullptr);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end = 0;

	FZFile::decode(file_buf, buffer_size); // first decrypt buffer
	size_t content_size = 0;
	size_t descr_size   = 0;
	char *descr;
	char *content       = FZFile::split(file_buf, buffer_size, content_size, descr, descr_size); // then split it


	ENSURE(content != nullptr);
	ENSURE(content_size > 0);
	content = FZFile::decompress(content, content_size, content_size); // decompress zlib content data
	ENSURE(content != nullptr);
	ENSURE(content_size > 0);

	ENSURE(content != descr);
	ENSURE(descr_size > 0);
	descr = FZFile::decompress(descr, descr_size, descr_size);
	ENSURE(descr != nullptr);
	ENSURE(descr_size > 0);

	int current_block = 0;
	std::unordered_map<std::string, int> parts_id; // map between part name and part number

	char **lines_content = stringfile(content);
	ENSURE(lines_content);

	char **lines_descr = stringfile(descr);
	ENSURE(lines_descr);

	// Parse the content part (parts, pins, nails)
	
	while (*lines_content) {
		char *line = *lines_content;
		++lines_content;

	//	fprintf(stdout,"%s\n", line);

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		/*
		 * If we have a UNIT: line in the data, see if it's requesting millimeters and scale appropriatley.
		 * Default is units are thou (0.001")
		 */
		if (!strcmp(line,"UNIT:millimeters")) {
			multiplier=25.4f;
		}

		if (line[0] == 'A') { // New block
			line+=2; // skip "A!"
			if (!strncmp(line, "REFDES", 6)) {
				current_block = 1;
				continue;
			} else if (!strncmp(line, "NET_NAME", 8)) {
				current_block = 2;
				continue;
			} else if (!strncmp(line, "TESTVIA", 7)) {
				current_block = 3;
				continue;
			}
		} else if (line[0] != 'S') // Unknown line type
			continue; // jump to next line
		else
			p+=2; // Skip "S!"

		switch (current_block) {
		case 1: { // Parts
			BRDPart part;
				part.name = READ_STR();
				/*char *cic =*/ READ_STR();
				/*char *sname =*/ READ_STR();
				char *smirror = READ_STR();
				/*char *srotate =*/ READ_STR();
			if(!strcmp(smirror, "YES"))
				part.type = 10; // SMD part on top
			else
				part.type = 5; // SMD part on bottom
			part.end_of_pins = 0;
			parts.push_back(part);
			parts_id[part.name] = parts.size();
		} break;
		case 2: { // Pins
			BRDPin pin;
				pin.net = READ_STR();
				char *part = READ_STR();
			pin.part = parts_id.at(part);
				pin.snum = READ_STR();
				/*char *name =*/ READ_STR();
				double posx = READ_DOUBLE();
				pin.pos.x = posx *multiplier;
				double posy = READ_DOUBLE();
				pin.pos.y = posy *multiplier;
				pin.probe = READ_UINT();
				double radius = READ_DOUBLE();
				radius /= 100;
				if (radius < 0.5f) radius = 0.5f;
				pin.radius                = radius *multiplier;
			pins.push_back(pin);
		} break;
		case 3: { // Nails
			p+=2; // Skip "Y!"
			BRDNail nail;
				nail.net = READ_STR();
				/*char *refdes =*/ READ_STR();
				/*int pinnumber =*/ READ_INT(); // uint
				/*char *pinname =*/ READ_STR();

				double posx = READ_DOUBLE();
				nail.pos.x = posx *multiplier;
				double posy = READ_DOUBLE();
				nail.pos.y = posy *multiplier;
				char *loc = READ_STR();
			if(!strcmp(loc, "T"))
				nail.side = 1; // on top
			else
				nail.side = 2; // on bottom
				/*double radius =*/ READ_DOUBLE();
			nails.push_back(nail);
		} break;
		}
	}


	// Parse the descr part (parts info)
	lines_descr += 2; // Discard first 2 lines (board description, currently unused and table columns name)
	while (*lines_descr) {
		char *line = *lines_descr;
		++lines_descr;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (line[0] == 's') continue; // PARTNUMBER starting with 's' seems unused

		FZPartDesc pdesc;
		pdesc.partno = READ_DESCR_STR();
		pdesc.description = READ_DESCR_STR();
		pdesc.quantity = READ_DESCR_UINT();
		pdesc.locations = split_string(READ_DESCR_STR());
		pdesc.partno2 = READ_DESCR_STR();
		partsDesc.push_back(pdesc);
	}

	for (auto &pdesc: partsDesc) {
		for (auto &partname: pdesc.locations) {
			auto iter = parts_id.find(partname);
			if(iter != parts_id.end()) { // Sometimes the desc part references stuff not in content part, ignore it
				parts.at(iter->second-1).mfgcode = pdesc.description;
		}
	}
		}


	std::sort(pins.begin(), pins.end()); // sort vector by part num then pin num
	for (std::vector<int>::size_type i = 0; i < pins.size(); i++) {
	  	// update end_of_pins field
		if (pins[i].part > 0) parts[pins[i].part-1].end_of_pins = i;
	}

	gen_outline();

	update_counts();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;
}
