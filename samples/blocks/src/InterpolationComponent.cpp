/*-----------------------------------------------------------------------

Matt Marchant 2020
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

#include "InterpolationSystem.hpp"

InterpolationComponent::InterpolationComponent(InterpolationPoint initialPoint)
    : m_enabled         (true),
    m_targetPoint       (initialPoint),
    m_previousPoint     (initialPoint),
    m_timeDifference    (1),
    m_started           (false)
{

}

//public
void InterpolationComponent::setTarget(const InterpolationPoint& target)
{
    if(m_buffer.size() < m_buffer.capacity())
    {
        m_buffer.push_back(target);
    }

    if (!m_started && m_buffer.size() == /*m_buffer.capacity()*/2)
    {
        m_started = true;
    }
    applyNextTarget();
}

void InterpolationComponent::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool InterpolationComponent::getEnabled() const
{
    return m_enabled;
}

void InterpolationComponent::resetPosition(glm::vec3 position)
{
    m_previousPoint.position = m_targetPoint.position = position;
}

void InterpolationComponent::resetRotation(glm::quat rotation)
{
    m_previousPoint.rotation = m_targetPoint.rotation = rotation;
}

//private
void InterpolationComponent::applyNextTarget()
{
    if (m_started && m_buffer.size() > 0)
    {
        InterpolationPoint target;
        while (m_buffer.size() > 0 &&
            target.timestamp <= m_targetPoint.timestamp)
        {
            //skips old/out of date targets
            target = m_buffer.pop_front();
        }

        //before this function is called the entity
        //is usually snapped to the target position/rotation
        //anyway so we can assume the previous point will be
        //assigned the current position/rotation
        m_previousPoint = m_targetPoint;
        m_elapsedTimer.restart();

        m_timeDifference = target.timestamp - m_previousPoint.timestamp;
        m_targetPoint = target;
    }
}