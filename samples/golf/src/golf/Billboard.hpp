/*-----------------------------------------------------------------------

Matt Marchant 2021
http://trederia.blogspot.com

Super Video Golf - zlib licence.

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

#include <crogine/ecs/components/BillboardCollection.hpp>
#include <crogine/ecs/components/Sprite.hpp>
#include <crogine/graphics/Texture.hpp>

static constexpr float PixelPerMetre = 32.f;// 64.f; //used for scaling clouds and 3D sprites

struct BillboardID final
{
    enum
    {
        Grass01,
        Grass02,

        Flowers01,
        Flowers02,
        Flowers03,
        Bush01,
        Bush02,
        Bush03,

        Tree01,
        Tree02,
        Tree03,
        Tree04,

        Count
    };
};

static inline cro::Billboard spriteToBillboard(const cro::Sprite& sprite)
{
    //pixels per metre in a billboard - not the same as above
    static constexpr float PPM = 64.f;

    if (sprite.getTexture())
    {
        auto bounds = sprite.getTextureRect();
        auto texSize = glm::vec2(sprite.getTexture()->getSize());

        cro::Billboard bb;
        bb.size = { bounds.width / PPM, bounds.height / PPM };
        bb.textureRect = { bounds.left / texSize.x, bounds.bottom / texSize.y, bounds.width / texSize.x, bounds.height / texSize.y };
        bb.origin = { bb.size.x / 2.f, 0.f };
        return bb;
    }
    return {};
};