/*-----------------------------------------------------------------------

Matt Marchant 2021 - 2022
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

#include <crogine/detail/Assert.hpp>
#include <crogine/detail/glm/gtx/matrix_interpolation.hpp>
#include <crogine/ecs/components/Skeleton.hpp>
#include <crogine/ecs/components/Transform.hpp>

using namespace cro;

Skeleton::Skeleton()
    : m_playbackRate        (1.f),
    m_currentAnimation      (0),
    m_nextAnimation         (-1),
    m_state                 (Stopped),
    m_blendTime             (1.f),
    m_currentBlendTime      (0.f),
    m_frameTime             (1.f),
    m_currentFrameTime      (0.f),
    m_useInterpolation      (true),
    m_interpolationDistance (2500.f),
    m_frameSize             (0),
    m_frameCount            (0)
{

}

void Skeleton::play(std::size_t idx, float rate, float blendingTime)
{
    CRO_ASSERT(idx < m_animations.size(), "Index out of range");
    CRO_ASSERT(rate >= 0, "");

    if (idx >= m_animations.size())
    {
        return;
    }

    if (idx != m_currentAnimation)
    {
        //blend if we're already playing
        m_nextAnimation = static_cast<std::int32_t>(idx);
        m_blendTime = blendingTime;
        m_currentBlendTime = 0.f;
    }
    else
    {
        m_animations[idx].playbackRate = rate;
        m_animations[idx].currentFrame = m_animations[idx].startFrame;
    }
    m_playbackRate = rate;
    m_state = Playing;
}

void Skeleton::prevFrame()

{
    CRO_ASSERT(!m_animations.empty(), "No animations loaded");

    auto& anim = m_animations[m_currentAnimation];
    auto frame = anim.currentFrame - anim.startFrame;
    frame = (frame + (anim.frameCount - 1)) % anim.frameCount;
    anim.currentFrame = frame + anim.startFrame;
    m_currentFrameTime = 0.f;
    buildKeyframe(anim.currentFrame);
}

void Skeleton::nextFrame()
{
    CRO_ASSERT(!m_animations.empty(), "No animations loaded");
    auto& anim = m_animations[m_currentAnimation];
    auto frame = anim.currentFrame - anim.startFrame;
    frame = (frame + 1) % anim.frameCount;
    anim.currentFrame = frame + anim.startFrame;
    m_currentFrameTime = 0.f;
    buildKeyframe(anim.currentFrame);
}

void Skeleton::gotoFrame(std::uint32_t frame)
{
    CRO_ASSERT(!m_animations.empty(), "No animations loaded");
    auto& anim = m_animations[m_currentAnimation];
    if (frame < anim.frameCount)
    {
        anim.currentFrame = frame + anim.startFrame;
        m_currentFrameTime = 0.f;
        buildKeyframe(anim.currentFrame);
    }
}

void Skeleton::stop()
{
    CRO_ASSERT(!m_animations.empty(), "");
    m_animations[m_currentAnimation].playbackRate = 0.f;
    m_state = Stopped;
}

Skeleton::State Skeleton::getState() const
{
    CRO_ASSERT(!m_animations.empty(), "");
    return m_state;
}

void Skeleton::addAnimation(const SkeletalAnim& anim)
{
    CRO_ASSERT(m_frameCount >= (anim.startFrame + anim.frameCount), "animation is out of frame range");
    m_animations.push_back(anim);
}

void Skeleton::addFrame(const std::vector<Joint>& frame)
{
    if (m_frameSize == 0)
    {
        m_frameSize = frame.size();
        m_currentFrame.resize(m_frameSize);
    }

    CRO_ASSERT(frame.size() == m_frameSize, "Incorrect frame size");
    m_frames.insert(m_frames.end(), frame.begin(), frame.end());
    m_notifications.emplace_back();
    m_frameCount++;
}

std::size_t Skeleton::getCurrentFrame() const
{
    CRO_ASSERT(!m_animations.empty(), "");
    return m_animations[m_currentAnimation].currentFrame;
}

void Skeleton::addNotification(std::size_t frameID, Notification n)
{
    CRO_ASSERT(frameID < m_frameCount, "Out of range");
    CRO_ASSERT(n.jointID < m_frameSize, "Out of range");
    m_notifications[frameID].push_back(n);
}

std::int32_t Skeleton::addAttachment(const Attachment& ap)
{
    m_attachments.push_back(ap);
    return static_cast<std::int32_t>(m_attachments.size() - 1);
}

std::int32_t Skeleton::getAttachmentIndex(const std::string& name) const
{
    if (auto result = std::find_if(m_attachments.begin(), m_attachments.end(),
        [&name](const Attachment& a)
        {
            return name == a.getName();
        }); result != m_attachments.end())
    {
        return static_cast<std::int32_t>(std::distance(m_attachments.begin(), result));
    }

    return -1;
}

glm::mat4 Skeleton::getAttachmentTransform(std::int32_t id) const
{
    CRO_ASSERT(id > -1 && id < m_attachments.size(), "");

    const auto& ap = m_attachments[id];
    return  m_currentFrame[ap.getParent()] * m_bindPose[ap.getParent()] * ap.getLocalTransform();
}

void Skeleton::setInverseBindPose(const std::vector<glm::mat4>& invBindPose)
{
    m_invBindPose = invBindPose; 
    m_bindPose.resize(invBindPose.size());

    for (auto i = 0u; i < invBindPose.size(); ++i)
    {
        m_bindPose[i] = glm::inverse(invBindPose[i]);
    }
}

void Skeleton::setRootTransform(const glm::mat4& transform)
{
    auto undoTx = glm::inverse(m_rootTransform);
    m_rootTransform = transform;

    for(auto i = 0u; i < m_frameCount; ++i)
    {
        //make sure to rebuild our bounding volumes
        buildKeyframe(i);

        if (i < m_keyFrameBounds.size())
        {
            auto bounds = undoTx * m_keyFrameBounds[i];
            m_keyFrameBounds[i] = m_rootTransform * bounds;
        }
    }
}

//private
void Skeleton::buildKeyframe(std::size_t frame)
{
    auto offset = m_frameSize * frame;
    for (auto i = 0u; i < m_frameSize; ++i)
    {
       m_currentFrame[i] = m_rootTransform * m_frames[offset + i].worldMatrix * m_invBindPose[i];
    }
}


//----attachment struct-----//
void Attachment::setParent(std::int32_t parent)
{
    m_parent = parent;
}

void Attachment::setModel(cro::Entity model)
{
    //reset any existing transform
    if (m_model != model &&
        m_model.isValid() &&
        m_model.hasComponent<cro::Transform>())
    {
        m_model.getComponent<cro::Transform>().m_attachmentTransform = glm::mat4(1.f);
    }

    m_model = model;
}

void Attachment::setPosition(glm::vec3 position)
{
    m_position = position;
    updateLocalTransform();
}

void Attachment::setRotation(glm::quat rotation)
{
    m_rotation = rotation;
    updateLocalTransform();
}

void Attachment::setScale(glm::vec3 scale)
{
    m_scale = scale;
    updateLocalTransform();
}

void Attachment::setName(const std::string& name)
{
    m_name = name;
    if (name.size() > MaxNameLength)
    {
        LogW << name << ": max character length is " << MaxNameLength << ", this name will be truncated when the model is saved." << std::endl;
    }
}

void Attachment::updateLocalTransform()
{
    m_transform = glm::translate(glm::mat4(1.f), m_position);
    m_transform *= glm::toMat4(m_rotation);
    m_transform = glm::scale(m_transform, m_scale);
}