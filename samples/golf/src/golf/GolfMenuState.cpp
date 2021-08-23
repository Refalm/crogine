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

#include "GolfMenuState.hpp"
#include "SharedStateData.hpp"
#include "PacketIDs.hpp"
#include "MenuConsts.hpp"
#include "Utility.hpp"
#include "CommandIDs.hpp"
#include "GameConsts.hpp"
#include "PoissonDisk.hpp"
#include "GolfCartSystem.hpp"

#include <crogine/core/App.hpp>
#include <crogine/gui/Gui.hpp>
#include <crogine/detail/GlobalConsts.hpp>
#include <crogine/graphics/SpriteSheet.hpp>
#include <crogine/util/String.hpp>
#include <crogine/util/Random.hpp>
#include <crogine/util/Wavetable.hpp>

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Text.hpp>
#include <crogine/ecs/components/Camera.hpp>
#include <crogine/ecs/components/CommandTarget.hpp>
#include <crogine/ecs/components/Sprite.hpp>
#include <crogine/ecs/components/Drawable2D.hpp>
#include <crogine/ecs/components/Callback.hpp>

#include <crogine/ecs/systems/TextSystem.hpp>
#include <crogine/ecs/systems/CameraSystem.hpp>
#include <crogine/ecs/systems/CommandSystem.hpp>
#include <crogine/ecs/systems/SpriteSystem2D.hpp>
#include <crogine/ecs/systems/RenderSystem2D.hpp>
#include <crogine/ecs/systems/UISystem.hpp>
#include <crogine/ecs/systems/CallbackSystem.hpp>
#include <crogine/ecs/systems/ModelRenderer.hpp>
#include <crogine/ecs/systems/BillboardSystem.hpp>

#include <cstring>

namespace
{
#include "TerrainShader.inl"

    constexpr glm::vec3 CameraBasePosition(-22.f, 4.9f, 22.2f);
}

GolfMenuState::GolfMenuState(cro::StateStack& stack, cro::State::Context context, SharedStateData& sd)
    : cro::State        (stack, context),
    m_sharedData        (sd),
    m_uiScene           (context.appInstance.getMessageBus()),
    m_backgroundScene   (context.appInstance.getMessageBus()),
    m_playerAvatar      ("assets/golf/images/player.png"),
    m_currentMenu       (MenuID::Main),
    m_viewScale         (2.f)
{
    //launches a loading screen (registered in MyApp.cpp)
    context.mainWindow.loadResources([this]() {
        loadAvatars();

        //add systems to scene
        addSystems();
        //load assets (textures, shaders, models etc)
        loadAssets();
        //create some entities
        createScene();
    });

    context.mainWindow.setMouseCaptured(false);
    context.mainWindow.setTitle("Lotec Golf");
    //context.appInstance.setClearColour(cro::Colour(0.2f, 0.2f, 0.26f));

    sd.clientConnection.ready = false;
    std::fill(m_readyState.begin(), m_readyState.end(), false);
        
    //we returned from a previous game
    if (sd.clientConnection.connected)
    {
        updateLobbyAvatars();

        //switch to lobby view
        cro::Command cmd;
        cmd.targetFlags = CommandID::Menu::RootNode;
        cmd.action = [&](cro::Entity e, float)
        {
            e.getComponent<cro::Transform>().setPosition(m_menuPositions[MenuID::Lobby] * m_viewScale);
            m_uiScene.getSystem<cro::UISystem>().setActiveGroup(GroupID::Lobby);
            m_currentMenu = MenuID::Lobby;
        };
        m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);

        cro::String buttonString;
        cro::String connectionString;

        if (m_sharedData.hosting)
        {
            buttonString = "Start";
            connectionString = "Hosting on: localhost:" + std::to_string(ConstVal::GamePort);

            //auto ready up if host
            m_sharedData.clientConnection.netClient.sendPacket(
                PacketID::LobbyReady, std::uint16_t(m_sharedData.clientConnection.connectionID << 8 | std::uint8_t(1)),
                cro::NetFlag::Reliable, ConstVal::NetChannelReliable);
        }
        else
        {
            buttonString = "Ready";
            connectionString = "Connected to: " + m_sharedData.targetIP + ":" + std::to_string(ConstVal::GamePort);
        }


        cmd.targetFlags = CommandID::Menu::ReadyButton;
        cmd.action = [buttonString](cro::Entity e, float)
        {
            e.getComponent<cro::Text>().setString(buttonString);
        };
        m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::Menu::ServerInfo;
        cmd.action = [connectionString](cro::Entity e, float)
        {
            e.getComponent<cro::Text>().setString(connectionString);
        };
        m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);
    }
    else
    {
        //we ought to be resetting previous data here?
        for (auto& cd : m_sharedData.connectionData)
        {
            cd.playerCount = 0;
        }
        m_sharedData.hosting = false;
    }

#ifdef CRO_DEBUG_
    /*registerWindow([&]() 
        {
            if (ImGui::Begin("Debug"))
            {
                auto camPos = m_backgroundScene.getActiveCamera().getComponent<cro::Transform>().getPosition();
                ImGui::Text("Cam Pos %3.3f, %3.3f, %3.3f", camPos.x, camPos.y, camPos.z);
            }
            ImGui::End();
        
        });*/
#endif
}

//public
bool GolfMenuState::handleEvent(const cro::Event& evt)
{
    if(cro::ui::wantsMouse() || cro::ui::wantsKeyboard())
    {
        return true;
    }

    if (evt.type == SDL_KEYUP)
    {
        switch (evt.key.keysym.sym)
        {
        default: break;
#ifdef CRO_DEBUG_
        case SDLK_F2:
            showPlayerConfig(true, 0);
            break;
        case SDLK_F3:
            showPlayerConfig(false, 0);
            break;
#endif
        case SDLK_1:

            break;
        case SDLK_2:
            
            break;
        case SDLK_3:
            
            break;
        case SDLK_4:

            break;
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
            if (m_textEdit.string)
            {
                applyTextEdit();
            }
            break;
        }
    }
    else if (evt.type == SDL_KEYDOWN)
    {
        handleTextEdit(evt);
    }
    else if (evt.type == SDL_TEXTINPUT)
    {
        handleTextEdit(evt);
    }

    m_uiScene.getSystem<cro::UISystem>().handleEvent(evt);

    m_uiScene.forwardEvent(evt);
    return true;
}

void GolfMenuState::handleMessage(const cro::Message& msg)
{
    m_backgroundScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool GolfMenuState::simulate(float dt)
{
    if (m_sharedData.clientConnection.connected)
    {
        cro::NetEvent evt;
        while (m_sharedData.clientConnection.netClient.pollEvent(evt))
        {
            //handle events
            handleNetEvent(evt);
        }
    }

    m_backgroundScene.simulate(dt);
    m_uiScene.simulate(dt);
    return true;
}

void GolfMenuState::render()
{
//draw any renderable systems
m_backgroundTexture.clear();
m_backgroundScene.render(m_backgroundTexture);
m_backgroundTexture.display();

m_uiScene.render(cro::App::getWindow());
}

//private
void GolfMenuState::addSystems()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_backgroundScene.addSystem<GolfCartSystem>(mb);
    m_backgroundScene.addSystem<cro::CallbackSystem>(mb);
    m_backgroundScene.addSystem<cro::BillboardSystem>(mb);
    m_backgroundScene.addSystem<cro::CameraSystem>(mb);
    m_backgroundScene.addSystem<cro::ModelRenderer>(mb);

    m_uiScene.addSystem<cro::CommandSystem>(mb);
    m_uiScene.addSystem<cro::CallbackSystem>(mb);
    m_uiScene.addSystem<cro::UISystem>(mb);
    m_uiScene.addSystem<cro::CameraSystem>(mb);
    m_uiScene.addSystem<cro::SpriteSystem2D>(mb);
    m_uiScene.addSystem<cro::TextSystem>(mb);
    m_uiScene.addSystem<cro::RenderSystem2D>(mb);
}

void GolfMenuState::loadAssets()
{
    m_font.loadFromFile("assets/golf/fonts/IBM_CGA.ttf");

    m_backgroundScene.setCubemap("assets/golf/images/skybox/spring/sky.ccm");

    m_resources.shaders.loadFromString(ShaderID::Cel, CelVertexShader, CelFragmentShader, "#define VERTEX_COLOURED\n");
    m_resources.shaders.loadFromString(ShaderID::CelTextured, CelVertexShader, CelFragmentShader, "#define TEXTURED\n");

    m_materialIDs[MaterialID::Cel] = m_resources.materials.add(m_resources.shaders.get(ShaderID::Cel));
    m_materialIDs[MaterialID::CelTextured] = m_resources.materials.add(m_resources.shaders.get(ShaderID::CelTextured));

    //load the billboard rects from a sprite sheet and convert to templates
    cro::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/golf/sprites/shrubbery.spt", m_resources.textures);
    m_billboardTemplates[BillboardID::Grass01] = spriteToBillboard(spriteSheet.getSprite("grass01"));
    m_billboardTemplates[BillboardID::Grass02] = spriteToBillboard(spriteSheet.getSprite("grass02"));
    m_billboardTemplates[BillboardID::Pine] = spriteToBillboard(spriteSheet.getSprite("pine"));
    m_billboardTemplates[BillboardID::Willow] = spriteToBillboard(spriteSheet.getSprite("willow"));
    m_billboardTemplates[BillboardID::Birch] = spriteToBillboard(spriteSheet.getSprite("birch"));
}

void GolfMenuState::createScene()
{
    auto texturedMat = m_resources.materials.get(m_materialIDs[MaterialID::CelTextured]);

    cro::ModelDefinition md(m_resources);
    md.loadFromFile("assets/golf/models/menu_pavilion.cmt");
    setTexture(md, texturedMat);

    auto entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>();
    md.createModel(entity);
    entity.getComponent<cro::Model>().setMaterial(0, texturedMat);

    md.loadFromFile("assets/golf/models/menu_ground.cmt");
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>();
    md.createModel(entity);
    texturedMat = m_resources.materials.get(m_materialIDs[MaterialID::CelTextured]);
    setTexture(md, texturedMat);
    entity.getComponent<cro::Model>().setMaterial(0, texturedMat);

    md.loadFromFile("assets/golf/models/phone_box.cmt");
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 8.2f, 0.f, 13.8f });
    md.createModel(entity);

    texturedMat = m_resources.materials.get(m_materialIDs[MaterialID::CelTextured]);
    setTexture(md, texturedMat);
    entity.getComponent<cro::Model>().setMaterial(0, texturedMat);


    //billboards
    md.loadFromFile("assets/golf/models/shrubbery.cmt");
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>();
    md.createModel(entity);

    if (entity.hasComponent<cro::BillboardCollection>())
    {
        std::array minBounds = { 30.f, 0.f };
        std::array maxBounds = { 80.f, 10.f };

        auto& collection = entity.getComponent<cro::BillboardCollection>();

        auto trees = pd::PoissonDiskSampling(4.8f, minBounds, maxBounds);
        for (auto [x, y] : trees)
        {
            float scale = static_cast<float>(cro::Util::Random::value(8, 12)) / 10.f;

            auto bb = m_billboardTemplates[cro::Util::Random::value(BillboardID::Pine, BillboardID::Willow)];
            bb.position = { x, 0.f, -y };
            bb.size *= scale;
            collection.addBillboard(bb);
        }

        //repeat for grass
        minBounds = { 10.5f, 12.f };
        maxBounds = { 26.f, 30.f };

        auto grass = pd::PoissonDiskSampling(1.f, minBounds, maxBounds);
        for (auto [x, y] : grass)
        {
            float scale = static_cast<float>(cro::Util::Random::value(8, 11)) / 10.f;

            auto bb = m_billboardTemplates[cro::Util::Random::value(BillboardID::Grass01, BillboardID::Grass02)];
            bb.position = { -x, 0.1f, y };
            bb.size *= scale;
            collection.addBillboard(bb);
        }
    }

    //golf carts
    md.loadFromFile("assets/golf/models/cart.cmt");
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.f, 0.01f, 1.8f });
    entity.getComponent<cro::Transform>().rotate(cro::Transform::Y_AXIS, 87.f * cro::Util::Const::degToRad);
    md.createModel(entity);

    texturedMat = m_resources.materials.get(m_materialIDs[MaterialID::CelTextured]);
    setTexture(md, texturedMat);
    entity.getComponent<cro::Model>().setMaterial(0, texturedMat);

    //these ones move :)
    for (auto i = 0u; i < 2u; ++i)
    {
        entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>();
        entity.addComponent<GolfCart>();
        md.createModel(entity);
        entity.getComponent<cro::Model>().setMaterial(0, texturedMat);
    }


    //update the 3D view
    auto updateView = [&](cro::Camera& cam)
    {
        auto vpSize = calcVPSize();
        m_backgroundTexture.create(static_cast<std::uint32_t>(vpSize.x), static_cast<std::uint32_t>(vpSize.y));

        //the resize actually extends the target vertically so we need to maintain a
        //horizontal FOV, not the vertical one expected by default.
        cam.setPerspective(FOV * (vpSize.y / ViewportHeight), vpSize.x / vpSize.y, 0.1f, vpSize.x);
        cam.viewport = { 0.f, 0.f, 1.f, 1.f };
    };

    auto camEnt = m_backgroundScene.getActiveCamera();
    auto& cam = camEnt.getComponent<cro::Camera>();
    cam.resizeCallback = updateView;
    updateView(cam);

    camEnt.getComponent<cro::Transform>().setPosition(CameraBasePosition);
    camEnt.getComponent<cro::Transform>().rotate(cro::Transform::Y_AXIS, -34.f * cro::Util::Const::degToRad);
    //camEnt.getComponent<cro::Transform>().rotate(cro::Transform::Z_AXIS, -0.84f * cro::Util::Const::degToRad);
    camEnt.getComponent<cro::Transform>().rotate(cro::Transform::X_AXIS, -8.f * cro::Util::Const::degToRad);

    struct CameraMotion final
    {
        std::size_t vertIndex = 0;
        std::size_t horIndex = 0;
    };
    camEnt.addComponent<cro::Callback>().active = true;
    camEnt.getComponent<cro::Callback>().setUserData<CameraMotion>();
    camEnt.getComponent<cro::Callback>().function =
        [](cro::Entity e, float dt)
    {
        static const std::vector<float> table = cro::Util::Wavetable::sine(0.04f, 0.04f);
        static constexpr float ShakeStrength = 0.5f;

        auto& [vertIdx, horIdx] = e.getComponent<cro::Callback>().getUserData<CameraMotion>();

        auto pos = CameraBasePosition;
        pos.x += table[horIdx];
        pos.y += table[vertIdx];

        e.getComponent<cro::Transform>().setPosition(pos);

        vertIdx = (vertIdx + 1) % table.size();
        horIdx = (horIdx + 4) % table.size();
    };

    auto sunEnt = m_backgroundScene.getSunlight();
    sunEnt.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, /*-0.967f*/-45.f * cro::Util::Const::degToRad);
    sunEnt.getComponent<cro::Transform>().rotate(cro::Transform::X_AXIS, -80.f * cro::Util::Const::degToRad);

    createUI();
}

void GolfMenuState::handleNetEvent(const cro::NetEvent& evt)
{
    if (evt.type == cro::NetEvent::PacketReceived)
    {
        switch (evt.packet.getID())
        {
        default: break;
        case PacketID::StateChange:
            if (evt.packet.as<std::uint8_t>() == sv::StateID::Game)
            {
                requestStackClear();
                requestStackPush(States::Golf::Game);
            }
            break;
        case PacketID::ConnectionAccepted:
            {
                //update local player data
                m_sharedData.clientConnection.connectionID = evt.packet.as<std::uint8_t>();
                m_sharedData.localConnectionData.connectionID = evt.packet.as<std::uint8_t>();
                m_sharedData.connectionData[m_sharedData.clientConnection.connectionID] = m_sharedData.localConnectionData;

                //send player details to server (name, skin)
                auto buffer = m_sharedData.localConnectionData.serialise();
                m_sharedData.clientConnection.netClient.sendPacket(PacketID::PlayerInfo, buffer.data(), buffer.size(), cro::NetFlag::Reliable, ConstVal::NetChannelStrings);

                //switch to lobby view
                cro::Command cmd;
                cmd.targetFlags = CommandID::Menu::RootNode;
                cmd.action = [&](cro::Entity e, float)
                {
                    e.getComponent<cro::Transform>().setPosition(m_menuPositions[MenuID::Lobby] * m_viewScale);
                    m_uiScene.getSystem<cro::UISystem>().setActiveGroup(GroupID::Lobby);
                    m_currentMenu = MenuID::Lobby;
                };
                m_uiScene.getSystem<cro::CommandSystem>().sendCommand(cmd);

                if (m_sharedData.serverInstance.running())
                {
                    //auto ready up if host
                    m_sharedData.clientConnection.netClient.sendPacket(
                        PacketID::LobbyReady, std::uint16_t(m_sharedData.clientConnection.connectionID << 8 | std::uint8_t(1)),
                        cro::NetFlag::Reliable, ConstVal::NetChannelReliable);
                }

                LOG("Successfully connected to server", cro::Logger::Type::Info);
            }
            break;
        case PacketID::ConnectionRefused:
        {
            std::string err = evt.packet.as<std::uint8_t>() == 0 ? "Server full" : "Game in progress";
            cro::Logger::log("Connection refused: " + err, cro::Logger::Type::Error);

            m_sharedData.clientConnection.netClient.disconnect();
            m_sharedData.clientConnection.connected = false;
        }
            break;
        case PacketID::LobbyUpdate:
            updateLobbyData(evt);
            break;
        case PacketID::ClientDisconnected:
            m_sharedData.connectionData[evt.packet.as<std::uint8_t>()].playerCount = 0;
            updateLobbyAvatars();
            break;
        case PacketID::LobbyReady:
        {
            std::uint16_t data = evt.packet.as<std::uint16_t>();
            m_readyState[((data & 0xff00) >> 8)] = (data & 0x00ff) ? true : false;
        }
            break;
        case PacketID::MapInfo:
            m_sharedData.mapDirectory = deserialiseString(evt.packet);
            break;
        break;
        }
    }
    else if (evt.type == cro::NetEvent::ClientDisconnect)
    {
        m_sharedData.errorMessage = "Lost Connection To Host";
        requestStackPush(States::Golf::Error);
    }
}

void GolfMenuState::handleTextEdit(const cro::Event& evt)
{
    if (!m_textEdit.string)
    {
        return;
    }

    if (evt.type == SDL_KEYDOWN)
    {
        switch (evt.key.keysym.sym)
        {
        default: break;
        case SDLK_BACKSPACE:
            if (!m_textEdit.string->empty())
            {
                m_textEdit.string->erase(m_textEdit.string->size() - 1);
            }
            break;
        //case SDLK_RETURN:
        //case SDLK_RETURN2:
            //applyTextEdit();
            //return;
        }
        
    }
    else if (evt.type == SDL_TEXTINPUT)
    {
        if (m_textEdit.string->size() < ConstVal::MaxStringChars
            && m_textEdit.string->size() < m_textEdit.maxLen)
        {
            auto codePoints = cro::Util::String::getCodepoints(evt.text.text);

            *m_textEdit.string += cro::String::fromUtf32(codePoints.begin(), codePoints.end());
        }
    }

    //update string origin
    if (!m_textEdit.string->empty())
    {
        auto bounds = cro::Text::getLocalBounds(m_textEdit.entity);
       // m_textEdit.entity.getComponent<cro::Transform>().setOrigin({ bounds.width / 2.f, -bounds.height / 2.f });
        //TODO make this scroll when we hist the edge of the input
    }
}

void GolfMenuState::applyTextEdit()
{
    if (m_textEdit.string && m_textEdit.entity.isValid())
    {
        if (m_textEdit.string->empty())
        {
            *m_textEdit.string = "INVALID";
        }

        m_textEdit.entity.getComponent<cro::Text>().setFillColour(TextNormalColour);
        m_textEdit.entity.getComponent<cro::Text>().setString(*m_textEdit.string);
        //auto bounds = cro::Text::getLocalBounds(m_textEdit.entity);
        //m_textEdit.entity.getComponent<cro::Transform>().setOrigin({ bounds.width / 2.f, -bounds.height / 2.f });
        m_textEdit.entity.getComponent<cro::Callback>().active = false;
        SDL_StopTextInput();
    }
    m_textEdit = {};
}