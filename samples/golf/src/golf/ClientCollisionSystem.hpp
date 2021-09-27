/*-----------------------------------------------------------------------

Matt Marchant 2021
http://trederia.blogspot.com

crogine application - Zlib license.

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

#include "HoleData.hpp"

#include <crogine/ecs/System.hpp>

#include <crogine/graphics/Image.hpp>

#include <crogine/detail/glm/vec3.hpp>

struct ClientCollider final
{
    glm::vec3 previousPosition = glm::vec3(0.f);
    std::int32_t previousDirection = 0;
};

class ClientCollisionSystem final : public cro::System 
{
public:
    ClientCollisionSystem(cro::MessageBus&, const std::vector<HoleData>&);

    void process(float) override;

    void setMap(std::uint32_t);

    void setActiveClub(std::int32_t club) { m_club = club; } //hacky.

private:
    const std::vector<HoleData>& m_holeData;
    std::uint32_t m_holeIndex;
    cro::Image m_currentMap;
    std::int32_t m_club;
};
