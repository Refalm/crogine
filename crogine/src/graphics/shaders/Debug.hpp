/*-----------------------------------------------------------------------

Matt Marchant 2017 - 2021
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

#include <string>

namespace cro::Shaders::Debug
{
    static const std::string Vertex = R"(
        ATTRIBUTE vec4 a_position;
        ATTRIBUTE LOW vec4 a_colour;

        uniform mat4 u_projectionMatrix;

        VARYING_OUT MED vec4 v_colour;

        void main()
        {
            gl_Position = u_projectionMatrix * a_position;
            v_colour = a_colour;
        })";

    static const std::string Fragment = R"(
        VARYING_IN LOW vec4 v_colour;
        OUTPUT

        void main()
        {
            FRAG_OUT = v_colour;
        })";
}