/*-----------------------------------------------------------------------

Matt Marchant 2017
http://trederia.blogspot.com

crogine test application - Zlib license.

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

#ifndef TEST_MENUSTATE_HPP_
#define TEST_MENUSTATE_HPP_

#include "StateIDs.hpp"
#include "CircularBuffer.hpp"

#include <crogine/core/State.hpp>
#include <crogine/ecs/Scene.hpp>
#include <crogine/graphics/TextureResource.hpp>
#include <crogine/gui/GuiClient.hpp>

class MenuState final : public cro::State, public cro::GuiClient
{
public:
    MenuState(cro::StateStack&, cro::State::Context);

    cro::StateID getStateID() const override { return States::MainMenu; }

    bool handleEvent(const cro::Event&) override;
    void handleMessage(const cro::Message&) override;
    bool simulate(float) override;
    void render() override;

private:
    cro::Scene m_scene;
    
    struct AudioSource final
    {
        std::vector<float> wavetable;
        std::size_t tableIndex = 0;

        std::array<float, 6u> levels = {};
    }m_audioSource;

    CircularBuffer<float, 16384> m_leftChannel;
    CircularBuffer<float, 16384> m_rightChannel;
    CircularBuffer<float, 16384> m_centreChannel;
    CircularBuffer<float, 16384> m_leftChannelRear;
    CircularBuffer<float, 16384> m_rightChannelRear;

    cro::Entity m_sourceEnt;

    void addSystems();
    void loadAssets();
    void createScene();
};

#endif //TEST_MENUSTATE_HPP_