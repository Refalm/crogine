/*-----------------------------------------------------------------------

Matt Marchant 2017 - 2020
http://trederia.blogspot.com

crogine - Zlib license.

This software is provided 'as-is', without any express or
implied warranty.In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.

-----------------------------------------------------------------------*/

#pragma once

#include <crogine/Config.hpp>
#include <crogine/detail/SDLResource.hpp>
#include <crogine/graphics/Texture.hpp>

#include <map>
#include <vector>
#include <any>

namespace cro
{
    class Image;

    struct Glyph final
    {
        float advance = 0.f;
        FloatRect bounds;
        FloatRect textureBounds;
    };

    /*!
    \brief Font class.
    Fonts are created from a given texture atlas which are a standard
    greyscale image. Atlases use only the first (red)
    colour channel to multiply the colour of the given text instance when drawn.
    */
    class CRO_EXPORT_API Font final : public Detail::SDLResource
    {
    public:

        Font();
        ~Font();
        Font(const Font&) = delete;
        const Font& operator = (const Font&) = delete;
        Font(Font&&) = delete;
        Font& operator = (Font&&) = delete;

        /*!
        \brief Attempts to load a font from a ttf file on disk.
        \param path Path to font file
        \returns true if successful else false
        */
        bool loadFromFile(const std::string& path);

        /*!
        \brief Attempts to return a float rect representing the sub rectangle of the atlas
        for the given codepoint.
        */
        FloatRect getGlyph(uint32 codepoint, uint32 charSize) const;

        /*!
        \brief Returns a reference to the texture used by the font
        */
        const Texture& getTexture(uint32 charSize) const;

        /*!
        \brief Returns the lineheight 
        */
        float getLineHeight(uint32 charSize) const;

    private:

        std::string m_path;

        struct Row final
        {
            Row(std::uint32_t t, std::uint32_t h) : width(0), top(t), height(h) {}
            std::uint32_t width = 0;
            std::uint32_t top = 0;
            std::uint32_t height = 0;
        };

        struct Page final
        {
            Texture texture;
            std::map<uint32, Glyph> glyphs;
            std::uint32_t nextRow = 0;
            std::vector<Row> rows;
        };

        mutable std::map<uint32, Page> m_pages;
        mutable std::vector<std::uint8_t> m_pixelBuffer;


        //use std::any so we don't expose freetype pointers to public API
        std::any m_library;
        std::any m_face;
        std::int32_t* m_refCount;

        void loadGlyph(std::uint32_t cp, std::uint32_t charSize);
        FloatRect getGlyphRect(Page&, std::uint32_t w, std::uint32_t h) const;
        bool setCurrentCharacterSize(std::uint32_t) const;

        void cleanup();
    };
}