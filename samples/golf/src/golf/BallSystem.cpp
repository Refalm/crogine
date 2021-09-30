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

#include "BallSystem.hpp"
#include "Terrain.hpp"
#include "HoleData.hpp"
#include "GameConsts.hpp"
#include "server/ServerMessages.hpp"

#include <crogine/ecs/components/Transform.hpp>

#include <crogine/graphics/Image.hpp>

#include <crogine/util/Random.hpp>
#include <crogine/util/Easings.hpp>

#include <crogine/detail/glm/gtx/norm.hpp>

namespace
{
    constexpr glm::vec3 Gravity(0.f, -9.8f, 0.f);

    static constexpr float MinBallDistance = HoleRadius * HoleRadius;
    static constexpr float BallTurnDelay = 2.5f; //how long to delay before stating turn ended
}

BallSystem::BallSystem(cro::MessageBus& mb, const cro::Image& mapData)
    : cro::System           (mb, typeid(BallSystem)),
    m_mapData               (mapData),
    m_windDirTime           (cro::seconds(0.f)),
    m_windStrengthTime      (cro::seconds(1.f)),
    m_windDirection         (-1.f, 0.f, 0.f),
    m_windDirSrc            (m_windDirection),
    m_windDirTarget         (1.f, 0.f, 0.f),
    m_windStrength          (0.f),
    m_windStrengthSrc       (m_windStrength),
    m_windStrengthTarget    (0.1f),
    m_windInterpTime        (1.f),
    m_currentWindInterpTime (0.f),
    m_holeData              (nullptr)
{
    requireComponent<cro::Transform>();
    requireComponent<Ball>();

    m_windDirTarget.x = static_cast<float>(cro::Util::Random::value(-10, 10)) / 10.f;
    m_windDirTarget.z = static_cast<float>(cro::Util::Random::value(-10, 10)) / 10.f;

    m_windDirTarget = glm::normalize(m_windDirTarget);

    m_windStrengthTarget = static_cast<float>(cro::Util::Random::value(1, 10)) / 10.f;

    updateWind();
}

//public
void BallSystem::process(float dt)
{
    //interpolate current strength/direction
    m_currentWindInterpTime = std::min(m_windInterpTime, m_currentWindInterpTime + dt);
    float interp = m_currentWindInterpTime / m_windInterpTime;
    m_windDirection = interpolate(m_windDirSrc, m_windDirTarget, interp);
    m_windStrength = interpolate(m_windStrengthSrc, m_windStrengthTarget, interp);

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& ball = entity.getComponent<Ball>();
        switch (ball.state)
        {
        default: break;
        case Ball::State::Idle:
            ball.hadAir = false;
            break;
        case Ball::State::Flight:
        {
            ball.hadAir = false;
            ball.delay -= dt;
            if (ball.delay < 0)
            {
                //add gravity
                ball.velocity += Gravity * dt;

                //add wind
                ball.velocity += m_windDirection * m_windStrength * dt;

                //TODO add air friction?

                //move by velocity
                auto& tx = entity.getComponent<cro::Transform>();
                tx.move(ball.velocity * dt);

                //test collision
                doCollision(entity);
            }
        }
        break;
        case Ball::State::Putt:
            
            ball.delay -= dt;
            if (ball.delay < 0)
            {
                auto& tx = entity.getComponent<cro::Transform>();
                auto position = tx.getPosition();

                auto [terrain, normal] = getTerrain(tx.getPosition());

                //test distance to pin
                auto len2 = glm::length2(glm::vec2(position.x, position.z) - glm::vec2(m_holeData->pin.x, m_holeData->pin.z));
                if (len2 < MinBallDistance)
                {
                    //over hole or in the air
                    static constexpr float MinFallVelocity = 2.1f;
                    float gravityAmount = 1.f - std::min(1.f, glm::length2(ball.velocity) / MinFallVelocity);

                    //this is some fudgy non-physics.
                    //if the ball falls low enough when
                    //over the hole we'll put it in.
                    ball.velocity += (gravityAmount * Gravity) * dt;

                    //TODO we could also test to see which side of the hole the ball
                    //currently is and add some 'side spin' to the velocity.

                    ball.hadAir = true;
                }
                else //we're on the green so roll
                {

                    //if the ball has registered some air but is not over
                    //the hole reset the depth and slow it down as if it
                    //bumped the far edge
                    if (ball.hadAir)
                    {
                        //these are all just a wild stab
                        //destined for some tweaking - basically puts the ball back along its vector
                        //towards the hole while maintaining gravity. As if it bounced off the inside of the hole
                        if (position.y < -(Ball::Radius * 0.5f))
                        {
                            ball.velocity *= -1.f;
                            ball.velocity.y *= -1.f;
                        }
                        else
                        {
                            //lets the ball continue travelling, ie overshoot
                            ball.velocity *= 0.7f;
                            ball.velocity.y = glm::length2(ball.velocity) * 0.4f;
                            position.y = 0.f;
                            tx.setPosition(position);
                        }
                    }
                    else
                    {
                        if (position.y > 0)
                        {
                            //we had a bump so add gravity
                            ball.velocity += Gravity * dt;
                        }
                        else if (position.y < 0)
                        {
                            ball.velocity.y = 0.f;
                            position.y = 0.f;
                            tx.setPosition(position);
                        }
                    }
                    ball.hadAir = false;

                    //move by slope from normal map
                    auto velLength = glm::length(ball.velocity);
                    glm::vec3 slope = glm::vec3(normal.x, 0.f, normal.z) * 0.16f * smoothstep(0.5f, 4.5f, velLength);
                    ball.velocity += slope;

                    //add wind - adding less wind the more the ball travels in the
                    //wind direction means we don't get blown forever
                    float windAmount = 1.f - glm::dot(m_windDirection, ball.velocity / velLength);
                    ball.velocity += m_windDirection * m_windStrength * 0.06f * windAmount * dt;

                    //add friction
                    ball.velocity *= 0.985f;
                }


                //move by velocity
                tx.move(ball.velocity * dt);
                ball.terrain = terrain; //TODO this will be wrong if the above movement changed the terrain

                //if we've slowed down or fallen more than the
                //ball's diameter (radius??) stop the ball
                if (glm::length2(ball.velocity) < 0.01f
                    || (position.y < -(Ball::Radius * 2.5f)))
                {
                    ball.velocity = glm::vec3(0.f);
                    
                    if (ball.terrain == TerrainID::Water
                        || ball.terrain == TerrainID::Scrub)
                    {
                        ball.state = Ball::State::Reset;
                    }
                    else
                    {
                        ball.state = Ball::State::Paused;
                    }

                    ball.delay = BallTurnDelay;

                    auto* msg = postMessage<BallEvent>(sv::MessageID::BallMessage);
                    msg->type = BallEvent::Landed;
                    msg->terrain = position.y < -Ball::Radius ? TerrainID::Hole : ball.terrain;
                    msg->position = position;
                }
            }
            break;
        case Ball::State::Reset:
        {
            ball.delay -= dt;
            if (ball.delay < 0)
            {
                //move towards hole util we find non-water
                auto& tx = entity.getComponent<cro::Transform>();

                std::uint8_t terrain = TerrainID::Water;
                auto ballPos = tx.getPosition();
                auto dir = glm::normalize(m_holeData->pin - ballPos);
                for (auto i = 0; i < 100; ++i) //max 100m
                {
                    ballPos += dir;
                    terrain = getTerrain(ballPos).first;

                    if (terrain != TerrainID::Water
                        && terrain != TerrainID::Scrub)
                    {
                        tx.setPosition(ballPos);
                        break;
                    }
                }

                //if for some reason we never got out the water, put the ball back at the start
                if (terrain == TerrainID::Water
                    || terrain == TerrainID::Scrub)
                {
                    terrain = TerrainID::Fairway;
                    tx.setPosition(m_holeData->tee);
                }

                //raise message to say player should be penalised
                auto* msg = postMessage<BallEvent>(sv::MessageID::BallMessage);
                msg->type = BallEvent::Foul;
                msg->terrain = terrain;
                msg->position = tx.getPosition();

                //set ball to reset / correct terrain
                ball.delay = 0.5f;
                ball.terrain = terrain;
                ball.state = Ball::State::Paused;
            }
        }
            break;
        case Ball::State::Paused:
        {
            ball.delay -= dt;
            if (ball.delay < 0)
            {
                //send message to report status
                auto* msg = postMessage<BallEvent>(sv::MessageID::BallMessage);
                msg->terrain = ball.terrain;
                msg->position = entity.getComponent<cro::Transform>().getPosition();

                if (msg->position.y < -Ball::Radius)
                {
                    //we're in the hole
                    msg->type = BallEvent::Holed;
                }
                else
                {
                    msg->type = BallEvent::TurnEnded;
                }


                ball.state = Ball::State::Idle;
                updateWind(); //is a bit less random but at least stops the wind
                //changing direction mid-stroke which is just annoying.
            }
        }
            break;
        }
    }
}

glm::vec3 BallSystem::getWindDirection() const
{
    //the Y value is unused so we pack the strength in here
    //(it's only for vis on the client anyhoo)
    return { m_windDirection.x, m_windStrength, m_windDirection.z };
}

void BallSystem::setHoleData(const HoleData& holeData)
{
    m_holeData = &holeData;
}

//private
void BallSystem::doCollision(cro::Entity entity)
{
    //check height
    auto& tx = entity.getComponent<cro::Transform>();
    if (auto pos = tx.getPosition(); pos.y < 0)
    {
        pos.y = 0.f;
        tx.setPosition(pos);

        auto& ball = entity.getComponent<Ball>();

        auto [terrain, normal] = getTerrain(pos);

        //apply dampening based on terrain (or splash)
        switch (terrain)
        {
        default: break;
        case TerrainID::Bunker:
        case TerrainID::Water:
        case TerrainID::Scrub:
            ball.velocity = glm::vec3(0.f);
            break;
        case TerrainID::Fairway:
            ball.velocity *= 0.33f;
            ball.velocity = glm::reflect(ball.velocity, normal);
            break;
        case TerrainID::Green:
            ball.velocity *= 0.28f;

            //if low bounce start rolling
            if (ball.velocity.y > -0.05f)
            {
                float momentum = 1.f - glm::dot(-cro::Transform::Y_AXIS, glm::normalize(ball.velocity));
                static constexpr float MaxMomentum = 20.f;
                momentum *= MaxMomentum;


                auto len = glm::length(ball.velocity);
                ball.velocity.y = 0.f;
                ball.velocity = glm::normalize(ball.velocity) * len * momentum; //fake physics to simulate momentum
                ball.state = Ball::State::Putt;
                ball.delay = 0.f;

                return;
            }
            else //bounce
            {
                ball.velocity = glm::reflect(ball.velocity, normal);
            }
            break;
        case TerrainID::Rough:
            ball.velocity *= 0.23f;
            ball.velocity = glm::reflect(ball.velocity, normal);
            break;
        }

        //stop the ball if velocity low enough
        if (glm::length2(ball.velocity) < 0.01f)
        {
            if (terrain == TerrainID::Water
                || terrain == TerrainID::Scrub)
            {
                ball.state = Ball::State::Reset;
            }
            else
            {
                ball.state = Ball::State::Paused;
            }
            ball.delay = BallTurnDelay;
            ball.terrain = terrain;
            ball.velocity = glm::vec3(0.f);

            auto* msg = postMessage<BallEvent>(sv::MessageID::BallMessage);
            msg->type = BallEvent::Landed;
            msg->terrain = ball.terrain;
            msg->position = tx.getPosition();
        }
    }
}

void BallSystem::updateWind()
{
    auto resetInterp =
        [&]()
    {
        m_windDirSrc = m_windDirection;
        m_windStrengthSrc = m_windStrength;

        m_currentWindInterpTime = 0.f;
        m_windInterpTime = static_cast<float>(cro::Util::Random::value(50, 75)) / 10.f;
    };

    //update wind direction
    if (m_windDirClock.elapsed() > m_windDirTime)
    {
        m_windDirClock.restart();
        m_windDirTime = cro::seconds(static_cast<float>(cro::Util::Random::value(100, 220)) / 10.f);

        //create new direction
        m_windDirTarget.x = static_cast<float>(cro::Util::Random::value(-10, 10)) / 10.f;
        m_windDirTarget.z = static_cast<float>(cro::Util::Random::value(-10, 10)) / 10.f;

        m_windDirTarget = glm::normalize(m_windDirTarget);

        resetInterp();
    }

    //update wind strength
    if (m_windStrengthClock.elapsed() > m_windStrengthTime)
    {
        m_windStrengthClock.restart();
        m_windStrengthTime = cro::seconds(static_cast<float>(cro::Util::Random::value(80, 180)) / 10.f);

        m_windStrengthTarget = static_cast<float>(cro::Util::Random::value(1, 10)) / 10.f;

        resetInterp();
    }
}

std::pair<std::uint8_t, glm::vec3> BallSystem::getTerrain(glm::vec3 pos) const
{
    auto mapSize = m_mapData.getSize();
    auto size = glm::vec2(mapSize);
    std::uint32_t x = static_cast<std::uint32_t>(std::max(0.f, std::min(size.x - 1.f, std::floor(pos.x))));
    std::uint32_t y = static_cast<std::uint32_t>(std::max(0.f, std::min(size.y - 1.f, std::floor(-pos.z))));

    CRO_ASSERT(m_mapData.getFormat() == cro::ImageFormat::RGBA, "expected RGBA format");

    auto index = ((y * mapSize.x) + x);
    CRO_ASSERT(index < mapSize.x * mapSize.y, "");

    //R is terrain * 10
    std::uint8_t terrain = m_mapData.getPixelData()[index * 4] / 10;
    terrain = std::min(static_cast<std::uint8_t>(TerrainID::Scrub), terrain);

    return std::make_pair(terrain, m_holeData->normalMap[index]);
}