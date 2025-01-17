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

#include <crogine/detail/glm/vec2.hpp>
#include <crogine/detail/glm/vec3.hpp>
#include <crogine/graphics/Colour.hpp>

namespace cro
{
    /*!
    \brief 2D vertex data struct
    Used to describe the layout of 2D drawable vertices
    vertex data contains position, colour and UV coordinates.
    Vertex2D is used in conjunction with Drawable2D components
    providing a fixed layout. Custom Drawable2D shaders should
    provide these attributes in the vertex shader.

    Vertex2D can also be used with SimpleVertexArray

    Texture coords should be provided in the normalised (0 - 1) range.
    \see Drawable2D
    \see SimpleVertexArray
    */
    struct CRO_EXPORT_API Vertex2D final
    {
        Vertex2D() = default;
        Vertex2D(glm::vec2 pos) : position(pos) {}
        Vertex2D(glm::vec2 pos, glm::vec2 coord) : position(pos), UV(coord) {}
        Vertex2D(glm::vec2 pos, Colour c) : position(pos), colour(c) {}
        Vertex2D(glm::vec2 pos, glm::vec2 coord, Colour c) : position(pos), UV(coord), colour(c) {}

        glm::vec2 position = glm::vec2(0.f);
        glm::vec2 UV = glm::vec2(0.f);
        cro::Colour colour = cro::Colour::White;

        static constexpr std::size_t Size = (sizeof(float) * 4) + sizeof(cro::Colour);
    };
}