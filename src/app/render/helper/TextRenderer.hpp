#pragma once

#include <array>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

#define THROW_CHECK(f, msg) if (f) { throw std::runtime_error(msg); }

class TextRenderer
{

public:

	TextRenderer();
	~TextRenderer();

	void initialize();
	void destroy();

	void renderText(const std::string & text, int x, int y, int font_size, void * target, int width, int height);

private:

	FT_Library m_ft;
	FT_Face m_face;
};
