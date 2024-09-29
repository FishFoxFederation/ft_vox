#include "TextRenderer.hpp"
#include "logger.hpp"

#include <stdexcept>

#define THROW_CHECK(f, msg) if (f) { throw std::runtime_error(msg); }

TextRenderer::TextRenderer()
{
}

TextRenderer::~TextRenderer()
{
}

void TextRenderer::initialize()
{
	THROW_CHECK(FT_Init_FreeType(&m_ft), "Failed to init freetype library");
	THROW_CHECK(FT_New_Face(m_ft, "assets/fonts/Minecraft.ttf", 0, &m_face), "Failed to load font face");

	LOG_INFO("Initialized freetype library");
}

void TextRenderer::destroy()
{
	THROW_CHECK(FT_Done_Face(m_face), "Failed to destroy font face");
	THROW_CHECK(FT_Done_FreeType(m_ft), "Failed to destroy freetype library");
}

void TextRenderer::renderText(const std::string & text, int x, int y, int font_size, void * target, int width, int height)
{
	FT_Set_Pixel_Sizes(m_face, 0, font_size);

	FT_GlyphSlot slot = m_face->glyph;

	int pen_x = x;
	int pen_y = y;

	for (const char & c : text)
	{
		if (FT_Load_Char(m_face, c, FT_LOAD_RENDER))
		{
			continue;
		}

		FT_Bitmap * bitmap = &slot->bitmap;

		int x_max = pen_x + bitmap->width;
		int y_max = pen_y + bitmap->rows;

		if (x_max < 0 || y_max < 0 || pen_x >= width || pen_y >= height)
		{
			pen_x += slot->advance.x >> 6;
			pen_y += slot->advance.y >> 6;
			continue;
		}

		int x0 = std::max(pen_x, 0);
		int y0 = std::max(pen_y, 0);
		int x1 = std::min(x_max, width);
		int y1 = std::min(y_max, height);

		for (int y = y0; y < y1; y++)
		{
			for (int x = x0; x < x1; x++)
			{
				int bitmap_x = x - pen_x;
				int bitmap_y = y - pen_y;

				int target_index = (y * width + x) * 4;
				int bitmap_index = (bitmap_y * bitmap->width + bitmap_x);

				((unsigned char *)target)[target_index + 0] = 255;
				((unsigned char *)target)[target_index + 1] = 255;
				((unsigned char *)target)[target_index + 2] = 255;
				((unsigned char *)target)[target_index + 3] = bitmap->buffer[bitmap_index];
			}
		}

		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6;
	}
}
