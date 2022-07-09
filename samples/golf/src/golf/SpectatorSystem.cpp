/*-----------------------------------------------------------------------

Matt Marchant 2022
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

#include "SpectatorSystem.hpp"
#include "Path.hpp"

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Skeleton.hpp>

#include <crogine/util/Constants.hpp>
#include <crogine/util/Maths.hpp>
#include <crogine/util/Random.hpp>

namespace
{
    constexpr float BaseSeparation = 1.8f;
    constexpr float NextTargetRadius = (0.5f * 0.5f);
}

SpectatorSystem::SpectatorSystem(cro::MessageBus& mb)
    : cro::System(mb, typeid(SpectatorSystem))
{
    requireComponent<cro::Transform>();
    requireComponent<cro::Skeleton>();
    requireComponent<Spectator>();
}

//public
void SpectatorSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& spectator = entity.getComponent<Spectator>();
        if (spectator.path)
        {
            switch (spectator.state)
            {
            default: break;
            case Spectator::State::Pause:
                spectator.stateTime += dt;
                if (spectator.stateTime > spectator.pauseTime)
                {
                    spectator.stateTime = 0.f;
                    spectator.state = Spectator::State::Walk;
                    entity.getComponent<cro::Skeleton>().play(spectator.anims[Spectator::AnimID::Walk], /*spectator.walkSpeed*/1.f, 0.15f);
                }
                break;

            case Spectator::State::Walk:
            {
                auto targetPos = spectator.path->getPoint(spectator.target);
                auto segmentIndex = spectator.target - std::max(0, spectator.direction);
                float speed = spectator.path->getSpeedMultiplier(segmentIndex) * (spectator.walkSpeed / spectator.path->getLength());

                //separate paths in opposition directions
                auto offset = glm::normalize(spectator.path->getPoints().back() - spectator.path->getPoints().front()) * static_cast<float>(spectator.direction);
                auto temp = offset.z;
                offset.z = -offset.x;
                offset.x = temp;
                targetPos -= (offset * BaseSeparation);

                //steer away from other spectators, and adjust target if it overlaps another spectator
                auto currPos = entity.getComponent<cro::Transform>().getPosition();
                for (auto e : m_spectatorGroups[spectator.path])
                {
                    if (e != entity)
                    {
                        static constexpr float Rad = 1.15f;
                        static constexpr float MinRadius = Rad * Rad;
                        auto diff = e.getComponent<cro::Transform>().getPosition() - currPos;
                        if (auto len2 = glm::length2(diff); len2 < MinRadius)
                        {
                            spectator.velocity -= diff * dt;

                            //makes 2 spectators disappear :S
                            /*auto len = std::sqrt(len2);
                            diff /= len;
                            len = Rad - len;
                            entity.getComponent<cro::Transform>().move(-diff * (len / 2.f));
                            e.getComponent<cro::Transform>().move(diff * (len / 2.f));*/
                        }

                        static constexpr float TargetRad = 2.5f;
                        static constexpr float MinTargetRadius = TargetRad * TargetRad;
                        diff = e.getComponent<cro::Transform>().getPosition() - targetPos;
                        if (auto len2 = glm::length2(diff); len2 < MinTargetRadius)
                        {
                            //TODO this should only be done in the x/z plane
                            auto len = std::sqrt(len2);
                            diff /= len;
                            len = TargetRad - len;
                            targetPos += diff * len;
                        }
                    }
                }



                auto& tx = entity.getComponent<cro::Transform>();

                auto dir = targetPos - tx.getPosition();
                spectator.velocity += glm::normalize(dir) * speed;
                spectator.velocity /= 2.f;
                tx.move(spectator.velocity);

                spectator.targetRotation = std::atan2(-spectator.velocity.z, spectator.velocity.x) + ((cro::Util::Const::PI / 2.f));
                spectator.rotation += cro::Util::Maths::shortestRotation(spectator.rotation, spectator.targetRotation) * (dt * 6.f);
                tx.setRotation(cro::Transform::Y_AXIS, spectator.rotation);



                //move to next target if < 0.5m
                if (glm::length2(dir) < NextTargetRadius)
                {
                    if (spectator.target == 0
                        || spectator.target == spectator.path->getPoints().size() - 1)
                    {
                        spectator.direction *= -1;
                        spectator.state = Spectator::State::Pause;
                        spectator.velocity = glm::vec3(0.f);
                        entity.getComponent<cro::Skeleton>().play(spectator.anims[Spectator::AnimID::Idle], 1.f, 0.25f);
                    }

                    auto pointCount = spectator.path->getPoints().size();
                    spectator.target = (spectator.target + ((pointCount + spectator.direction)) % pointCount) % pointCount;
                }
            }
                break;
            }
        }
    }
}

void SpectatorSystem::updateSpectatorGroups()
{
    m_spectatorGroups.clear();

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto* path = entity.getComponent<Spectator>().path;
        if (path)
        {
            m_spectatorGroups[path].push_back(entity);
        }
    }
}

//private
void SpectatorSystem::onEntityAdded(cro::Entity e)
{
    e.getComponent<Spectator>().pauseTime = 3.f + cro::Util::Random::value(0.f, 1.f);
}