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
}

void TextRenderer::destroy()
{
	THROW_CHECK(FT_Done_Face(m_face), "Failed to destroy font face");
	THROW_CHECK(FT_Done_FreeType(m_ft), "Failed to destroy freetype library");
}

void TextRenderer::renderText(const std::string & text, int x, int y, int font_size, void * target, int width, int height)
{
	FT_Set_Pixel_Sizes(m_face, font_size, font_size);

	FT_GlyphSlot slot = m_face->glyph;

	int pen_x = x;
	int pen_y = y + font_size;

	for (const char & c : text)
	{
		if (c == '\n')
		{
			pen_x = x;
			pen_y += font_size;
			continue;
		}

		// TODO: cache the glyphs
		if (FT_Load_Char(m_face, c, FT_LOAD_RENDER))
		{
			LOG_WARNING("Failed to load glyph for character: " << c);
			continue;
		}

		for (uint i = 0; i < slot->bitmap.rows; i++)
		{
			for (uint j = 0; j < slot->bitmap.width; j++)
			{
				const int target_x = pen_x + j + slot->bitmap_left;
				const int target_y = pen_y + i - slot->bitmap_top;

				if (target_x >= 0 && target_x < width && target_y >= 0 && target_y < height)
				{
					const int index = (target_y * width + target_x) * 4;
					const uint8_t value = slot->bitmap.buffer[i * slot->bitmap.width + j];

					((unsigned char *)target)[index] = 255;
					((unsigned char *)target)[index + 1] = 255;
					((unsigned char *)target)[index + 2] = 255;
					((unsigned char *)target)[index + 3] = value;

				}
			}
		}

		pen_x += slot->advance.x >> 6;
		// pen_y += slot->advance.y >> 6;
	}
}
