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

#include "GolfState.hpp"
#include "GameConsts.hpp"
#include "CommandIDs.hpp"
#include "SharedStateData.hpp"
#include "Clubs.hpp"

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Sprite.hpp>
#include <crogine/ecs/components/SpriteAnimation.hpp>
#include <crogine/ecs/components/Text.hpp>
#include <crogine/ecs/components/Drawable2D.hpp>
#include <crogine/ecs/components/Camera.hpp>
#include <crogine/ecs/components/CommandTarget.hpp>
#include <crogine/ecs/components/Callback.hpp>


namespace
{
    glm::vec2 viewScale(1.f); //tracks the current scale of the output

}

void GolfState::buildUI()
{
    if (m_holeData.empty())
    {
        return;
    }

    //draws the background using the render texture
    auto entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>();
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>(m_renderTexture.getTexture());
    auto bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.getComponent<cro::Transform>().setOrigin(glm::vec2(bounds.width / 2.f, bounds.height / 2.f));
    auto courseEnt = entity;


    auto& camera = m_gameScene.getActiveCamera().getComponent<cro::Camera>();
    camera.updateMatrices(m_gameScene.getActiveCamera().getComponent<cro::Transform>());
    auto pos = camera.coordsToPixel(m_holeData[0].tee, m_renderTexture.getSize());

    //player sprite - TODO apply avatar customisation
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(pos);
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::PlayerSprite;
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::Player01];
    entity.addComponent<cro::SpriteAnimation>();
    bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.getComponent<cro::Transform>().setOrigin(glm::vec2(bounds.width * 0.78f, 0.f));
    courseEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());
    auto playerEnt = entity;
    m_currentPlayer.position = m_holeData[0].tee;

    //flag sprite. TODO make this less crap
    //this is updated by a command sent when the 3D camera is positioned
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>();
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::Flag01];
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::FlagSprite;
    courseEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());




    //info panel background
    auto windowSize = glm::vec2(cro::App::getWindow().getSize());
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(HoleInfoPosition * windowSize);
    entity.addComponent<UIElement>().position = HoleInfoPosition;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::UIElement;
    auto colour = cro::Colour(0.f, 0.f, 0.f, 0.5f);
    entity.addComponent<cro::Drawable2D>().getVertexData() =
    {
        cro::Vertex2D(glm::vec2(0.f, 160.f), colour), //TODO remove all these magic numbers
        cro::Vertex2D(glm::vec2(0.f), colour),
        cro::Vertex2D(glm::vec2(134.f, 160.f), colour),
        cro::Vertex2D(glm::vec2(134.f, 0.f), colour)
    };
    entity.getComponent<cro::Drawable2D>().updateLocalBounds();
    auto infoEnt = entity;



    auto& font = m_resources.fonts.get(FontID::UI);

    //player's name
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 150.f));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::PlayerName;
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //hole number
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 132.f));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::HoleNumber;
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //hole distance
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 120.f));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::PinDistance;
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //club info
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 108.f));
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().function =
        [&](cro::Entity e, float)
    {
        e.getComponent<cro::Text>().setString(Clubs[getClub()].name);
    };
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //current stroke
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 96.f));
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().function =
        [&](cro::Entity e, float)
    {
        auto stroke = std::to_string(m_sharedData.connectionData[m_currentPlayer.client].playerData[m_currentPlayer.player].holeScores[m_currentHole]);
        e.getComponent<cro::Text>().setString("Stroke: " + stroke);
    };
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());


    //current terrain
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(10.f, 84.f));
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().function =
        [&](cro::Entity e, float)
    {
        e.getComponent<cro::Text>().setString(TerrainStrings[m_currentPlayer.terrain]);
    };
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());


    //wind indicator
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(67.f, 82.f)); //TODO de-magicfy this and make it half the size of the background
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::WindIndicator];
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::WindSock;
    bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.getComponent<cro::Transform>().setOrigin(glm::vec2(bounds.width / 2.f, bounds.height / 2.f));
    entity.getComponent<cro::Transform>().move(glm::vec2(0.f, -bounds.height));
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    auto camDir = m_holeData[0].pin - m_currentPlayer.position;
    m_camRotation = std::atan2(-camDir.z, camDir.y);

    //wind strength
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(67.f, 30.f)); //TODO same as indicator
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::WindString;
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    entity.getComponent<cro::Text>().setAlignment(cro::Text::Alignment::Centre);
    infoEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());







    //root used to show/hide input UI
    auto rootNode = m_uiScene.createEntity();
    rootNode.addComponent<cro::Transform>().setPosition(UIHiddenPosition);
    rootNode.addComponent<cro::CommandTarget>().ID = CommandID::UI::Root;
    infoEnt.getComponent<cro::Transform>().addChild(rootNode.getComponent<cro::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec3(67.f, 10.f, 0.02f)); //TODO demagify - this is 50% panel width
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::PowerBar];
    bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.getComponent<cro::Transform>().setOrigin(glm::vec2(bounds.width / 2.f, bounds.height / 2.f));
    rootNode.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //power bar
    auto barEnt = entity;
    auto barCentre = bounds.width / 2.f;
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec2(5.f, 0.f)); //TODO expel the magic number!!
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::PowerBarInner];
    bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().function =
        [&, bounds](cro::Entity e, float)
    {
        auto crop = bounds;
        crop.width *= m_inputParser.getPower();
        e.getComponent<cro::Drawable2D>().setCroppingArea(crop);
    };
    barEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());

    //hook/slice indicator
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(glm::vec3(barCentre, 8.f, 0.1f)); //TODO expel the magic number!!
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Sprite>() = m_sprites[SpriteID::HookBar];
    bounds = entity.getComponent<cro::Sprite>().getTextureBounds();
    entity.getComponent<cro::Transform>().setOrigin(glm::vec2(bounds.width / 2.f, bounds.height / 2.f));
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().function =
        [&, barCentre](cro::Entity e, float)
    {
        glm::vec3 pos(barCentre + (barCentre * m_inputParser.getHook()), 8.f, 0.1f);
        e.getComponent<cro::Transform>().setPosition(pos);
    };
    barEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());









    //callback for the UI camera when window is resized
    auto updateView = [&, playerEnt, courseEnt, rootNode](cro::Camera& cam) mutable
    {
        auto size = glm::vec2(cro::App::getWindow().getSize());
        cam.setOrthographic(0.f, size.x, 0.f, size.y, -0.5f, 1.f);
        cam.viewport = { 0.f, 0.f, 1.f, 1.f };

        auto vpSize = calcVPSize();

        viewScale = glm::vec2(std::floor(size.y / vpSize.y));
        courseEnt.getComponent<cro::Transform>().setScale(viewScale);
        courseEnt.getComponent<cro::Transform>().setPosition(glm::vec3(size / 2.f, -0.1f));
        courseEnt.getComponent<cro::Transform>().setOrigin(vpSize / 2.f);
        courseEnt.getComponent<cro::Sprite>().setTextureRect({ 0.f, 0.f, vpSize.x, vpSize.y });

        //update avatar position
        const auto& camera = m_gameScene.getActiveCamera().getComponent<cro::Camera>();
        auto pos = camera.coordsToPixel(m_currentPlayer.position, m_renderTexture.getSize());
        playerEnt.getComponent<cro::Transform>().setPosition(pos);


        //send command to UIElements and reposition / rescale
        cro::Command cmd;
        cmd.targetFlags = CommandID::UI::UIElement;
        cmd.action = [size](cro::Entity e, float)
        {
            auto pos = size * e.getComponent<UIElement>().position;
            e.getComponent<cro::Transform>().setPosition(glm::vec3(pos, e.getComponent<UIElement>().depth));
            e.getComponent<cro::Transform>().setScale(viewScale);
        };
        m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);
    };

    auto& cam = m_uiScene.getActiveCamera().getComponent<cro::Camera>();
    cam.resizeCallback = updateView;
    updateView(cam);
}

void GolfState::showCountdown(std::uint8_t seconds)
{
    //hide any input
    cro::Command cmd;
    cmd.targetFlags = CommandID::UI::Root;
    cmd.action = [](cro::Entity e, float)
    {
        e.getComponent<cro::Transform>().setPosition(UIHiddenPosition);
    };
    m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);

    constexpr glm::vec2 position(0.5f, 0.5f);
    constexpr glm::vec2 size(400.f, 300.f);

    glm::vec2 windowSize(cro::App::getWindow().getSize());

    auto entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition(windowSize * position);
    entity.getComponent<cro::Transform>().setScale(viewScale);
    entity.getComponent<cro::Transform>().setOrigin(size / 2.f);
    entity.addComponent<UIElement>().position = position;
    entity.getComponent<UIElement>().depth = 0.21f;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::UI::UIElement;

    //TODO replace this with some sort of graphic?
    //We'll probably be showing scoreboard here.
    cro::Colour c(0.f, 0.f, 0.f, 0.6f);
    entity.addComponent<cro::Drawable2D>().getVertexData() =
    {
        cro::Vertex2D(glm::vec2(0.f, size.y), c),
        cro::Vertex2D(glm::vec2(0.f), c),
        cro::Vertex2D(size, c),
        cro::Vertex2D(glm::vec2(size.x, 0.f), c)
    };
    entity.getComponent<cro::Drawable2D>().updateLocalBounds();

    auto& font = m_resources.fonts.get(FontID::UI);

    auto bgEnt = entity;
    entity = m_uiScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ size.x / 2.f, 20.f, 0.01f });
    entity.addComponent<cro::Drawable2D>();
    entity.addComponent<cro::Text>(font).setCharacterSize(8);
    entity.getComponent<cro::Text>().setAlignment(cro::Text::Alignment::Centre);
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().setUserData<std::pair<float, std::uint8_t>>(1.f, seconds);
    entity.getComponent<cro::Callback>().function =
        [](cro::Entity e, float dt)
    {
        auto& [current, sec] = e.getComponent<cro::Callback>().getUserData<std::pair<float, std::uint8_t>>();
        current -= dt;
        if (current < 0)
        {
            current += 1.f;
            sec--;
        }

        e.getComponent<cro::Text>().setString("Return to lobby in: " + std::to_string(sec));
    };
    bgEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());
}