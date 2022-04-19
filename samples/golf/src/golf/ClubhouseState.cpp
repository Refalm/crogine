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

#include "ClubhouseState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "MenuConsts.hpp"
#include "PacketIDs.hpp"
#include "CommandIDs.hpp"
#include "Utility.hpp"

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Camera.hpp>

#include <crogine/ecs/systems/TextSystem.hpp>
#include <crogine/ecs/systems/SpriteSystem2D.hpp>
#include <crogine/ecs/systems/CommandSystem.hpp>
#include <crogine/ecs/systems/CallbackSystem.hpp>
#include <crogine/ecs/systems/ShadowMapRenderer.hpp>
#include <crogine/ecs/systems/ModelRenderer.hpp>
#include <crogine/ecs/systems/UISystem.hpp>
#include <crogine/ecs/systems/CameraSystem.hpp>
#include <crogine/ecs/systems/RenderSystem2D.hpp>
#include <crogine/ecs/systems/AudioSystem.hpp>
#include <crogine/ecs/systems/AudioPlayerSystem.hpp>

#include <crogine/util/Constants.hpp>
#include <crogine/gui/Gui.hpp>

namespace
{
#include "TerrainShader.inl"
}

ClubhouseContext::ClubhouseContext(ClubhouseState* state)
{
    uiScene = &state->m_uiScene;
    currentMenu = &state->m_currentMenu;
    menuEntities = state->m_menuEntities.data();

    menuPositions = state->m_menuPositions.data();
    viewScale = &state->m_viewScale;
    sharedData = &state->m_sharedData;
}

constexpr std::array<glm::vec2, ClubhouseState::MenuID::Count> ClubhouseState::m_menuPositions =
{
    glm::vec2(0.f, 0.f),
    glm::vec2(0.f, 0.f),
    glm::vec2(0.f, MenuSpacing.y),
    glm::vec2(-MenuSpacing.x, MenuSpacing.y),
    glm::vec2(-MenuSpacing.x, 0.f)
};

ClubhouseState::ClubhouseState(cro::StateStack& ss, cro::State::Context ctx, SharedStateData& sd)
    : cro::State        (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_scaleBuffer       ("PixelScale", sizeof(float)),
    m_resolutionBuffer  ("ScaledResolution", sizeof(glm::vec2)),
    m_viewScale         (2.f),
    m_currentMenu       (MenuID::Main),
    m_prevMenu          (MenuID::Main)
{
    std::fill(m_readyState.begin(), m_readyState.end(), false);

    ctx.mainWindow.loadResources([this]() {
        addSystems();
        loadResources();
        buildScene();
        });

    ctx.mainWindow.setMouseCaptured(false);

    sd.inputBinding.controllerID = 0;
    sd.baseState = StateID::Clubhouse;
    sd.mapDirectory = "pool";

    //this is actually set asa flag from the pause menu
    //to say we want to quit
    if (sd.tutorial)
    {
        sd.serverInstance.stop();
        sd.hosting = false;

        sd.tutorial = false;
        sd.clientConnection.connected = false;
        sd.clientConnection.connectionID = 4;
        sd.clientConnection.ready = false;
        sd.clientConnection.netClient.disconnect();
    }


    //we returned from a previous game
    if (sd.clientConnection.connected)
    {
        //updateLobbyAvatars();

        //switch to lobby view - send as a command
        //to ensure it's delayed by a frame
        cro::Command cmd;
        cmd.targetFlags = CommandID::Menu::RootNode;
        cmd.action = [&](cro::Entity e, float)
        {
            m_uiScene.getSystem<cro::UISystem>()->setActiveGroup(MenuID::Dummy);
            m_menuEntities[m_currentMenu].getComponent<cro::Callback>().getUserData<MenuData>().targetMenu = MenuID::Lobby;
            m_menuEntities[m_currentMenu].getComponent<cro::Callback>().active = true;
        };
        m_uiScene.getSystem<cro::CommandSystem>()->sendCommand(cmd);

        std::int32_t spriteID = 0;
        cro::String connectionString;

        if (m_sharedData.hosting)
        {
            spriteID = SpriteID::StartGame;
            connectionString = "Hosting on: localhost:" + std::to_string(ConstVal::GamePort);

            //auto ready up if host
            m_sharedData.clientConnection.netClient.sendPacket(
                PacketID::LobbyReady, std::uint16_t(m_sharedData.clientConnection.connectionID << 8 | std::uint8_t(1)),
                cro::NetFlag::Reliable, ConstVal::NetChannelReliable);


            //set the course selection menu
            //addTableSelectButtons();

            //send a UI refresh to correctly place buttons
            glm::vec2 size(GolfGame::getActiveTarget()->getSize());
            cmd.targetFlags = CommandID::Menu::UIElement;
            cmd.action =
                [&, size](cro::Entity e, float)
            {
                const auto& element = e.getComponent<UIElement>();
                auto pos = element.absolutePosition;
                pos += element.relativePosition * size / m_viewScale;

                pos.x = std::floor(pos.x);
                pos.y = std::floor(pos.y);

                e.getComponent<cro::Transform>().setPosition(glm::vec3(pos, element.depth));
            };
            m_uiScene.getSystem<cro::CommandSystem>()->sendCommand(cmd);


            //send the initially selected map/course
            m_sharedData.mapDirectory = "pool"; //TODO read this from a list of parsed directories EG m_tables[m_sharedDirectory.courseIndex]
            auto data = serialiseString(m_sharedData.mapDirectory);
            m_sharedData.clientConnection.netClient.sendPacket(PacketID::MapInfo, data.data(), data.size(), cro::NetFlag::Reliable, ConstVal::NetChannelStrings);
        }
        else
        {
            spriteID = SpriteID::ReadyUp;
            connectionString = "Connected to: " + m_sharedData.targetIP + ":" + std::to_string(ConstVal::GamePort);
        }


        cmd.targetFlags = CommandID::Menu::ReadyButton;
        cmd.action = [&, spriteID](cro::Entity e, float)
        {
            e.getComponent<cro::Sprite>() = m_sprites[spriteID];
        };
        m_uiScene.getSystem<cro::CommandSystem>()->sendCommand(cmd);

        cmd.targetFlags = CommandID::Menu::ServerInfo;
        cmd.action = [connectionString](cro::Entity e, float)
        {
            e.getComponent<cro::Text>().setString(connectionString);
        };
        m_uiScene.getSystem<cro::CommandSystem>()->sendCommand(cmd);
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
}

//public
bool ClubhouseState::handleEvent(const cro::Event& evt)
{
    if (cro::ui::wantsKeyboard() || cro::ui::wantsMouse())
    {
        return false;
    }

    if (evt.type == SDL_KEYDOWN)
    {
        handleTextEdit(evt);
    }
    else if (evt.type == SDL_TEXTINPUT)
    {
        handleTextEdit(evt);
    }

    m_uiScene.getSystem<cro::UISystem>()->handleEvent(evt);

    m_backgroundScene.forwardEvent(evt);
    m_uiScene.forwardEvent(evt);

    return false;
}

void ClubhouseState::handleMessage(const cro::Message& msg)
{
    m_backgroundScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool ClubhouseState::simulate(float dt)
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

    return false;
}

void ClubhouseState::render()
{
    m_scaleBuffer.bind(0);
    m_resolutionBuffer.bind(1);

    m_backgroundTexture.clear(cro::Colour::CornflowerBlue);
    m_backgroundScene.render();
    m_backgroundTexture.display();

    m_uiScene.render();
}

//private
void ClubhouseState::addSystems()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_backgroundScene.addSystem<cro::CallbackSystem>(mb);
    m_backgroundScene.addSystem<cro::ShadowMapRenderer>(mb);
    m_backgroundScene.addSystem<cro::ModelRenderer>(mb);
    m_backgroundScene.addSystem<cro::CameraSystem>(mb);
    m_backgroundScene.addSystem<cro::AudioSystem>(mb);


    m_uiScene.addSystem<cro::CommandSystem>(mb);
    m_uiScene.addSystem<cro::CallbackSystem>(mb);
    m_uiScene.addSystem<cro::UISystem>(mb);
    m_uiScene.addSystem<cro::TextSystem>(mb);
    m_uiScene.addSystem<cro::SpriteSystem2D>(mb);
    m_uiScene.addSystem<cro::CameraSystem>(mb);
    m_uiScene.addSystem<cro::RenderSystem2D>(mb);
    m_uiScene.addSystem<cro::AudioPlayerSystem>(mb);
}

void ClubhouseState::loadResources()
{
    m_reflectionMap.loadFromFile("assets/golf/images/skybox/billiards/trophy.ccm");

    std::fill(m_materialIDs.begin(), m_materialIDs.end(), -1);

    m_resources.shaders.loadFromString(ShaderID::Course, CelVertexShader, CelFragmentShader, "#define TEXTURED\n#define RX_SHADOWS\n");
    auto* shader = &m_resources.shaders.get(ShaderID::Course);
    m_scaleBuffer.addShader(*shader);
    m_resolutionBuffer.addShader(*shader);
    m_materialIDs[MaterialID::Cel] = m_resources.materials.add(*shader);

    m_resources.shaders.loadFromString(ShaderID::Leaderboard, CelVertexShader, CelFragmentShader, "#define VERTEX_COLOURED\n#define TEXTURED\n#define RX_SHADOWS\n");
    shader = &m_resources.shaders.get(ShaderID::Leaderboard);
    m_scaleBuffer.addShader(*shader);
    m_resolutionBuffer.addShader(*shader);
    m_materialIDs[MaterialID::Shelf] = m_resources.materials.add(*shader);

    m_resources.shaders.loadFromString(ShaderID::Trophy, CelVertexShader, CelFragmentShader, "#define VERTEX_COLOURED\n#define RX_SHADOWS\n#define REFLECTIONS\n");
    shader = &m_resources.shaders.get(ShaderID::Trophy);
    m_scaleBuffer.addShader(*shader);
    m_resolutionBuffer.addShader(*shader);
    m_materialIDs[MaterialID::Trophy] = m_resources.materials.add(*shader);
    m_resources.materials.get(m_materialIDs[MaterialID::Trophy]).setProperty("u_reflectMap", cro::CubemapID(m_reflectionMap.getGLHandle()));


    m_menuSounds.loadFromFile("assets/golf/sound/billiards/menu.xas", m_resources.audio);
    m_audioEnts[AudioID::Accept] = m_uiScene.createEntity();
    m_audioEnts[AudioID::Accept].addComponent<cro::AudioEmitter>() = m_menuSounds.getEmitter("accept");
    m_audioEnts[AudioID::Back] = m_uiScene.createEntity();
    m_audioEnts[AudioID::Back].addComponent<cro::AudioEmitter>() = m_menuSounds.getEmitter("back");
}

void ClubhouseState::buildScene()
{
    m_backgroundScene.setCubemap("assets/golf/images/skybox/mountain_spring/sky.ccm");


    cro::ModelDefinition md(m_resources);

    auto applyMaterial = [&](cro::Entity entity, std::int32_t id)
    {
        auto material = m_resources.materials.get(m_materialIDs[id]);
        applyMaterialData(md, material);
        entity.getComponent<cro::Model>().setMaterial(0, material);
    };

    if (md.loadFromFile("assets/golf/models/clubhouse_menu.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>();
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Cel);
    }

    if (md.loadFromFile("assets/golf/models/menu_ground.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>();
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Cel);
    }

    if (md.loadFromFile("assets/golf/models/trophies/cabinet.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({14.3f, 1.2f, -1.6f});
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Cel);
    }

    if (md.loadFromFile("assets/golf/models/trophies/shelf.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.2f, -1.6f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Shelf);

        entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.8f, -1.6f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Shelf);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy05.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.2f, -0.9f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy03.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.2f, -1.2f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy04.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.2f, -2.3f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy07.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.8f, -1.3f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 91.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);

        entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.2f, -2.05f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 89.5f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy06.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.8f, -1.9f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy02.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.8f, -2.15f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/trophies/trophy01.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 14.25f, 1.8f, -1.f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Trophy);
    }

    if (md.loadFromFile("assets/golf/models/hole_19/snooker_model.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 18.6f, 0.5f, -1.8f });
        entity.getComponent<cro::Transform>().setRotation(cro::Transform::Y_AXIS, 90.f * cro::Util::Const::degToRad);
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Cel);
    }

    if (md.loadFromFile("assets/golf/models/table_legs.cmt"))
    {
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 18.6f, 0.f, -1.8f });
        md.createModel(entity);

        applyMaterial(entity, MaterialID::Cel);
    }



    //TODO bar stools
    //TODO sofa



    //ambience 01
    auto entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 19.6f, 2.f, -0.8f });
    entity.addComponent<cro::AudioEmitter>() = m_menuSounds.getEmitter("01");
    entity.getComponent<cro::AudioEmitter>().play();

    //ambience 02
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 17.6f, 1.6f, -2.8f });
    entity.addComponent<cro::AudioEmitter>() = m_menuSounds.getEmitter("02");
    entity.getComponent<cro::AudioEmitter>().play();

    //music
    entity = m_backgroundScene.createEntity();
    entity.addComponent<cro::Transform>();
    entity.addComponent<cro::AudioEmitter>() = m_menuSounds.getEmitter("music");
    entity.getComponent<cro::AudioEmitter>().play();

    auto targetVol = entity.getComponent<cro::AudioEmitter>().getVolume();
    entity.getComponent<cro::AudioEmitter>().setVolume(0.f);
    entity.addComponent<cro::Callback>().active = true;
    entity.getComponent<cro::Callback>().setUserData<std::pair<float, const float>>(0.f, targetVol);
    entity.getComponent<cro::Callback>().function =
        [](cro::Entity e, float dt)
    {
        auto& [currTime, target] = e.getComponent<cro::Callback>().getUserData<std::pair<float, const float>>();
        currTime = std::min(1.f, currTime + dt);

        e.getComponent<cro::AudioEmitter>().setVolume(cro::Util::Easing::easeOutQuint(currTime) * target);

        if (currTime == 1)
        {
            e.getComponent<cro::Callback>().active = false;
        }
    };


    //update the 3D view
    auto updateView = [&](cro::Camera& cam)
    {
        auto vpSize = calcVPSize();

        auto winSize = glm::vec2(cro::App::getWindow().getSize());
        float maxScale = std::floor(winSize.y / vpSize.y);
        float scale = m_sharedData.pixelScale ? maxScale : 1.f;
        auto texSize = winSize / scale;

        auto invScale = (maxScale + 1) - scale;
        m_scaleBuffer.setData(&invScale);

        glm::vec2 scaledRes = texSize / invScale;
        m_resolutionBuffer.setData(&scaledRes);

        m_backgroundTexture.create(static_cast<std::uint32_t>(texSize.x), static_cast<std::uint32_t>(texSize.y));

        cam.setPerspective(m_sharedData.fov * cro::Util::Const::degToRad, texSize.x / texSize.y, 0.1f, 30.f);
        cam.viewport = { 0.f, 0.f, 1.f, 1.f };
    };

    auto camEnt = m_backgroundScene.getActiveCamera();
    camEnt.getComponent<cro::Transform>().setPosition({ 24.f, 1.6f, -4.3f });
    camEnt.getComponent<cro::Transform>().rotate(cro::Transform::Y_AXIS, 127.f * cro::Util::Const::degToRad);
    camEnt.getComponent<cro::Transform>().rotate(cro::Transform::X_AXIS, -0.8f * cro::Util::Const::degToRad);
    camEnt.getComponent<cro::Transform>().move(camEnt.getComponent<cro::Transform>().getForwardVector());
    //camEnt.addComponent<cro::Callback>():
    auto& cam = camEnt.getComponent<cro::Camera>();
    cam.resizeCallback = updateView;
    cam.shadowMapBuffer.create(2048, 2048);
    updateView(cam);

    auto sunEnt = m_backgroundScene.getSunlight();
    sunEnt.getComponent<cro::Transform>().rotate(cro::Transform::Y_AXIS, -90.f * cro::Util::Const::degToRad);
    sunEnt.getComponent<cro::Transform>().rotate(cro::Transform::X_AXIS, -130.56f * cro::Util::Const::degToRad);

    createUI();
}

void ClubhouseState::handleNetEvent(const cro::NetEvent& evt)
{
    if (evt.type == cro::NetEvent::PacketReceived)
    {
        switch (evt.packet.getID())
        {
        default: break;
        case PacketID::StateChange:
            if (evt.packet.as<std::uint8_t>() == sv::StateID::Billiards)
            {
                requestStackClear();
                requestStackPush(StateID::Billiards);
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

            if (m_sharedData.tutorial)
            {
                m_sharedData.clientConnection.netClient.sendPacket(PacketID::RequestGameStart, std::uint8_t(sv::StateID::Golf), cro::NetFlag::Reliable, ConstVal::NetChannelReliable);
            }
            else
            {
                //switch to lobby view
                cro::Command cmd;
                cmd.targetFlags = CommandID::Menu::RootNode;
                cmd.action = [&](cro::Entity e, float)
                {
                    m_uiScene.getSystem<cro::UISystem>()->setActiveGroup(MenuID::Dummy);
                    m_menuEntities[m_currentMenu].getComponent<cro::Callback>().getUserData<MenuData>().targetMenu = MenuID::Lobby;
                    m_menuEntities[m_currentMenu].getComponent<cro::Callback>().active = true;
                };
                m_uiScene.getSystem<cro::CommandSystem>()->sendCommand(cmd);
            }

            if (m_sharedData.serverInstance.running())
            {
                //auto ready up if hosting
                m_sharedData.clientConnection.netClient.sendPacket(
                    PacketID::LobbyReady, std::uint16_t(m_sharedData.clientConnection.connectionID << 8 | std::uint8_t(1)),
                    cro::NetFlag::Reliable, ConstVal::NetChannelReliable);
            }

            LOG("Successfully connected to server", cro::Logger::Type::Info);
        }
        break;
        case PacketID::ConnectionRefused:
        {
            std::string err = "Connection refused: ";
            switch (evt.packet.as<std::uint8_t>())
            {
            default:
                err += "Unknown Error";
                break;
            case MessageType::ServerFull:
                err += "Server full";
                break;
            case MessageType::NotInLobby:
                err += "Game in progress";
                break;
            case MessageType::BadData:
                err += "Bad Data Received";
                break;
            case MessageType::VersionMismatch:
                err += "Client/Server Mismatch";
                break;
            }
            LogE << err << std::endl;
            m_sharedData.errorMessage = err;
            requestStackPush(StateID::Error);

            m_sharedData.clientConnection.netClient.disconnect();
            m_sharedData.clientConnection.connected = false;
        }
        break;
        case PacketID::LobbyUpdate:
            updateLobbyData(evt);
            break;
        case PacketID::ClientDisconnected:
        {
            auto client = evt.packet.as<std::uint8_t>();
            m_sharedData.connectionData[client].playerCount = 0;
            m_readyState[client] = false;
            //updateLobbyAvatars();
        }
        break;
        case PacketID::LobbyReady:
        {
            std::uint16_t data = evt.packet.as<std::uint16_t>();
            m_readyState[((data & 0xff00) >> 8)] = (data & 0x00ff) ? true : false;
        }
        break;
        case PacketID::MapInfo:
        {
            //check we have the local data (or at least something with the same name)
            auto table = deserialiseString(evt.packet);

            /*if (auto data = std::find_if(m_courseData.cbegin(), m_courseData.cend(),
                [&course](const CourseData& cd)
                {
                    return cd.directory == course;
                }); data != m_courseData.cend())*/
            {
                m_sharedData.mapDirectory = table;

                //update UI
                
            }
            //else
            {
                //print to UI course is missing
            }
        }
        break;
        case PacketID::ServerError:
            switch (evt.packet.as<std::uint8_t>())
            {
            default:
                m_sharedData.errorMessage = "Server Error (Unknown)";
                break;
            case MessageType::MapNotFound:
                m_sharedData.errorMessage = "Server Failed To Load Map";
                break;
            }
            requestStackPush(StateID::Error);
            break;
        case PacketID::ClientVersion:
            //server is requesting our client version
            m_sharedData.clientConnection.netClient.sendPacket(PacketID::ClientVersion, CURRENT_VER, cro::NetFlag::Reliable);
            break;
            break;
        }
    }
    else if (evt.type == cro::NetEvent::ClientDisconnect)
    {
        m_sharedData.errorMessage = "Lost Connection To Host";
        requestStackPush(StateID::Error);
    }
}