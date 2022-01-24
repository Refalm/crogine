/*-----------------------------------------------------------------------

Matt Marchant 2021 - 2022
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

#include "MenuState.hpp"
#include "GameConsts.hpp"
#include "MenuConsts.hpp"
#include "spooky2.hpp"

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Model.hpp>
#include <crogine/ecs/components/Callback.hpp>
#include <crogine/ecs/components/Camera.hpp>

#include <crogine/util/Random.hpp>

namespace
{
#include "RandNames.hpp"
}

void MenuState::createBallScene()
{
    static constexpr float RootPoint = 100.f;
    static constexpr float BallSpacing = 0.09f;

    auto ballTexCallback = [&](cro::Camera&)
    {
        auto vpSize = calcVPSize().y;
        auto windowSize = static_cast<float>(cro::App::getWindow().getSize().y);

        float windowScale = std::floor(windowSize / vpSize);
        float scale = m_sharedData.pixelScale ? windowScale : 1.f;
        auto size = BallPreviewSize * static_cast<std::uint32_t>((windowScale + 1.f) - scale);
        m_ballTexture.create(size, size);
    };

    m_ballCam = m_backgroundScene.createEntity();
    m_ballCam.addComponent<cro::Transform>().setPosition({ RootPoint, 0.045f, 0.095f });
    m_ballCam.getComponent<cro::Transform>().rotate(cro::Transform::X_AXIS, -0.03f);
    m_ballCam.addComponent<cro::Camera>().setPerspective(1.f, 1.f, 0.001f, 2.f);
    m_ballCam.getComponent<cro::Camera>().resizeCallback = ballTexCallback;
    m_ballCam.addComponent<cro::Callback>().active = true;
    m_ballCam.getComponent<cro::Callback>().setUserData<std::int32_t>(0);
    m_ballCam.getComponent<cro::Callback>().function =
        [](cro::Entity e, float dt)
    {
        auto id = e.getComponent<cro::Callback>().getUserData<std::int32_t>();
        float target = RootPoint + (BallSpacing * id);

        auto pos = e.getComponent<cro::Transform>().getPosition();
        auto diff = target - pos.x;
        pos.x += diff * (dt * 10.f);

        e.getComponent<cro::Transform>().setPosition(pos);
    };

    ballTexCallback(m_ballCam.getComponent<cro::Camera>());


    auto ballFiles = cro::FileSystem::listFiles(cro::FileSystem::getResourcePath() + "assets/golf/balls");
    if (ballFiles.empty())
    {
        LogE << "No ball files were found" << std::endl;
    }

    m_sharedData.ballModels.clear();

    for (const auto& file : ballFiles)
    {
        cro::ConfigFile cfg;
        if (cro::FileSystem::getFileExtension(file) == ".ball"
            && cfg.loadFromFile("assets/golf/balls/" + file))
        {
            std::uint32_t uid = SpookyHash::Hash32(file.data(), file.size(), 0);
            std::string modelPath;
            cro::Colour colour = cro::Colour::White;

            const auto& props = cfg.getProperties();
            for (const auto& p : props)
            {
                const auto& name = p.getName();
                if (name == "model")
                {
                    modelPath = p.getValue<std::string>();
                }
                /*else if (name == "uid")
                {
                    uid = p.getValue<std::int32_t>();
                }*/
                else if (name == "tint")
                {
                    colour = p.getValue<cro::Colour>();
                }
            }

            if (/*uid > -1
                &&*/ (!modelPath.empty() && cro::FileSystem::fileExists(cro::FileSystem::getResourcePath() + modelPath)))
            {
                auto ball = std::find_if(m_sharedData.ballModels.begin(), m_sharedData.ballModels.end(),
                    [uid](const SharedStateData::BallInfo& ballPair)
                    {
                        return ballPair.uid == uid;
                    });

                if (ball == m_sharedData.ballModels.end())
                {
                    m_sharedData.ballModels.emplace_back(colour, uid, modelPath);
                }
                else
                {
                    LogE << file << ": a ball already exists with UID " << uid << std::endl;
                }
            }
        }
    }

    cro::ModelDefinition ballDef(m_resources);

    cro::ModelDefinition shadowDef(m_resources);
    auto shadow = shadowDef.loadFromFile("assets/golf/models/ball_shadow.cmt");

    for (auto i = 0u; i < m_sharedData.ballModels.size(); ++i)
    {
        if (ballDef.loadFromFile(m_sharedData.ballModels[i].modelPath))
        {
            auto entity = m_backgroundScene.createEntity();
            entity.addComponent<cro::Transform>().setPosition({ (i * BallSpacing) + RootPoint, 0.f, 0.f });
            ballDef.createModel(entity);
            entity.getComponent<cro::Model>().setMaterial(0, m_resources.materials.get(m_materialIDs[MaterialID::Cel]));

            entity.addComponent<cro::Callback>().active = true;
            entity.getComponent<cro::Callback>().function =
                [](cro::Entity e, float dt)
            {
                e.getComponent<cro::Transform>().rotate(cro::Transform::Y_AXIS, /*0.3f **/ dt);
            };

            if (shadow)
            {
                auto ballEnt = entity;
                entity = m_backgroundScene.createEntity();
                entity.addComponent<cro::Transform>();
                shadowDef.createModel(entity);
                ballEnt.getComponent<cro::Transform>().addChild(entity.getComponent<cro::Transform>());
            }
        }
    }
}

std::int32_t MenuState::indexFromBallID(std::uint32_t ballID)
{
    auto ball = std::find_if(m_sharedData.ballModels.begin(), m_sharedData.ballModels.end(),
        [ballID](const SharedStateData::BallInfo& ballPair)
        {
            return ballPair.uid == ballID;
        });

    if (ball != m_sharedData.ballModels.end())
    {
        return static_cast<std::int32_t>(std::distance(m_sharedData.ballModels.begin(), ball));
    }

    return -1;
}

void MenuState::parseAvatarDirectory()
{
    m_sharedData.avatarInfo.clear();

    const std::string AvatarPath = "assets/golf/avatars/";

    auto files = cro::FileSystem::listFiles(cro::FileSystem::getResourcePath() + AvatarPath);
    m_playerAvatars.reserve(files.size());

    for (const auto& file : files)
    {
        if (cro::FileSystem::getFileExtension(file) == ".avt")
        {
            cro::ConfigFile cfg;
            if (cfg.loadFromFile(AvatarPath + file))
            {
                SharedStateData::AvatarInfo info;
                std::string texturePath;

                const auto& props = cfg.getProperties();
                for (const auto& prop : props)
                {
                    const auto& name = prop.getName();
                    if (name == "model")
                    {
                        //info.uid = prop.getValue<std::int32_t>();

                        //create the UID based on a the string hash.
                        //uses Bob Jenkins' spooky hash, untested on M1 processors
                        info.modelPath = prop.getValue<std::string>();
                        if (!info.modelPath.empty())
                        {
                            info.uid = SpookyHash::Hash32(file.data(), file.size(), 0);
                            //LogI << "Got hash of " << info.uid << std::endl;

                            cro::ConfigFile modelData;
                            modelData.loadFromFile(info.modelPath);
                            for (const auto& o : modelData.getObjects())
                            {
                                if (o.getName() == "material")
                                {
                                    for (const auto& p : o.getProperties())
                                    {
                                        if (p.getName() == "diffuse")
                                        {
                                            texturePath = p.getValue<std::string>();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (name == "audio")
                    {
                        info.audioscape = prop.getValue<std::string>();
                    }
                }

                if (info.uid != 0
                    && !info.modelPath.empty())
                {
                    //check uid doesn't exist
                    auto result = std::find_if(m_sharedData.avatarInfo.begin(), m_sharedData.avatarInfo.end(),
                        [&info](const SharedStateData::AvatarInfo& i)
                        {
                            return info.uid == i.uid;
                        });

                    if (result == m_sharedData.avatarInfo.end())
                    {
                        m_sharedData.avatarInfo.push_back(info);
                        m_playerAvatars.emplace_back(texturePath);
                    }
                    else
                    {
                        LogW << "Avatar with UID " << info.uid << " already exists. " << info.modelPath << " will be skipped." << std::endl;
                    }
                }
                else
                {
                    LogW << "Skipping " << file << ": missing or corrupt data, or not an avatar." << std::endl;
                }
            }
        }
    }

    if (!m_playerAvatars.empty())
    {
        for (auto i = 0u; i < ConnectionData::MaxPlayers; ++i)
        {
            m_avatarIndices[i] = indexFromAvatarID(m_sharedData.localConnectionData.playerData[i].skinID);
        }
    }

    createAvatarScene();
}

void MenuState::createAvatarScene()
{
    auto avatarTexCallback = [&](cro::Camera&)
    {
        auto vpSize = calcVPSize().y;
        auto windowSize = static_cast<float>(cro::App::getWindow().getSize().y);

        float windowScale = std::floor(windowSize / vpSize);
        float scale = m_sharedData.pixelScale ? windowScale : 1.f;
        auto size = AvatarPreviewSize * static_cast<std::uint32_t>((windowScale + 1.f) - scale);
        m_avatarTexture.create(size.x, size.y);
    };

    auto avatarCam = m_avatarScene.createEntity();
    avatarCam.addComponent<cro::Transform>().setPosition({ 0.f, 0.649f, 1.3f });
    //avatarCam.addComponent<cro::Camera>().setPerspective(75.f * cro::Util::Const::degToRad, static_cast<float>(AvatarPreviewSize.x) / AvatarPreviewSize.y, 0.001f, 10.f);

    constexpr float ratio = static_cast<float>(AvatarPreviewSize.y) / AvatarPreviewSize.x;
    constexpr float orthoWidth = 0.7f;
    auto orthoSize = glm::vec2(orthoWidth, orthoWidth * ratio);
    avatarCam.addComponent<cro::Camera>().setOrthographic(-orthoSize.x, orthoSize.x, -orthoSize.y, orthoSize.y, 0.001f, 10.f);
    avatarCam.getComponent<cro::Camera>().resizeCallback = avatarTexCallback;
    avatarTexCallback(avatarCam.getComponent<cro::Camera>());

    m_avatarScene.setActiveCamera(avatarCam);

    //load the preview models
    cro::ModelDefinition clubDef(m_resources);
    clubDef.loadFromFile("assets/golf/models/club_iron.cmt");

    cro::ModelDefinition md(m_resources);
    for (auto i = 0u; i < m_sharedData.avatarInfo.size(); ++i)
    {
        if (md.loadFromFile(m_sharedData.avatarInfo[i].modelPath))
        {
            auto entity = m_avatarScene.createEntity();
            entity.addComponent<cro::Transform>().setOrigin(glm::vec2(-0.34f, 0.f));
            entity.getComponent<cro::Transform>().setScale(glm::vec3(0.f));
            md.createModel(entity);
            entity.getComponent<cro::Model>().setHidden(true);

            //TODO account for multiple materials? Avatar is set to
            //only update a single image though, so 1 material should
            //be a hard limit.
            auto material = m_resources.materials.get(m_materialIDs[MaterialID::CelTexturedSkinned]);
            applyMaterialData(md, material);
            entity.getComponent<cro::Model>().setMaterial(0, material);

            if (entity.hasComponent<cro::Skeleton>())
            {
                auto id = entity.getComponent<cro::Skeleton>().getAttachmentIndex("hands");
                if (id > -1)
                {
                    auto e = m_avatarScene.createEntity();
                    e.addComponent<cro::Transform>();
                    clubDef.createModel(e);

                    material = m_resources.materials.get(m_materialIDs[MaterialID::Cel]);
                    applyMaterialData(clubDef, material);
                    material.doubleSided = true; //we could update front face with parent model but meh
                    e.getComponent<cro::Model>().setMaterial(0, material);

                    entity.getComponent<cro::Skeleton>().getAttachments()[id].setModel(e);
                }
                else
                {
                    //no hands, no good
                    LogE << cro::FileSystem::getFileName(m_sharedData.avatarInfo[i].modelPath) << ": no hands attachment found, avatar not loaded" << std::endl;
                    m_avatarScene.destroyEntity(entity);
                    m_sharedData.avatarInfo[i].modelPath.clear();
                }

                //TODO fail to load if there's no animations? This shouldn't
                //be game breaking if there are none, it'll just look wrong.
            }
            else
            {
                //no skeleton, no good (see why this works, below)
                LogE << cro::FileSystem::getFileName(m_sharedData.avatarInfo[i].modelPath) << ": no skeleton found, avatar not loaded" << std::endl;
                m_avatarScene.destroyEntity(entity);
                m_sharedData.avatarInfo[i].modelPath.clear();
            }

            m_playerAvatars[i].previewModel = entity;
        }
        else
        {
            //we'll use this to signify failure by clearing
            //the string and using that to erase the entry
            //from the avatar list (below). An empty avatar 
            //list will display an error on the menu and stop
            //the game from being playable.
            m_sharedData.avatarInfo[i].modelPath.clear();
            LogE << m_sharedData.avatarInfo[i].modelPath << ": model not loaded!" << std::endl;
        }
    }

    //remove all info with no valid path
    m_sharedData.avatarInfo.erase(std::remove_if(
        m_sharedData.avatarInfo.begin(),
        m_sharedData.avatarInfo.end(),
        [](const SharedStateData::AvatarInfo& ai)
        {
            return ai.modelPath.empty();
        }),
        m_sharedData.avatarInfo.end());

    //remove all models with no valid preview
    m_playerAvatars.erase(std::remove_if(m_playerAvatars.begin(), m_playerAvatars.end(),
        [](const PlayerAvatar& a)
        {
            return !a.previewModel.isValid();
        }),
        m_playerAvatars.end());
}

std::int32_t MenuState::indexFromAvatarID(std::uint32_t id)
{
    auto avatar = std::find_if(m_sharedData.avatarInfo.begin(), m_sharedData.avatarInfo.end(),
        [id](const SharedStateData::AvatarInfo& info)
        {
            return info.uid == id;
        });

    if (avatar != m_sharedData.avatarInfo.end())
    {
        return static_cast<std::int32_t>(std::distance(m_sharedData.avatarInfo.begin(), avatar));
    }

    return 0;
}

void MenuState::applyAvatarColours(std::size_t playerIndex)
{
    auto avatarIndex = m_avatarIndices[playerIndex];

    const auto& flags = m_sharedData.localConnectionData.playerData[playerIndex].avatarFlags;
    m_playerAvatars[avatarIndex].setColour(pc::ColourKey::Bottom, flags[0]);
    m_playerAvatars[avatarIndex].setColour(pc::ColourKey::Top, flags[1]);
    m_playerAvatars[avatarIndex].setColour(pc::ColourKey::Skin, flags[2]);
    m_playerAvatars[avatarIndex].setColour(pc::ColourKey::Hair, flags[3]);

    m_playerAvatars[avatarIndex].setTarget(m_sharedData.avatarTextures[0][playerIndex]);
    m_playerAvatars[avatarIndex].apply();
}

void MenuState::updateThumb(std::size_t index)
{
    setPreviewModel(index);

    //we have to make sure model data is updated correctly before each
    //draw call as this func might be called in a loop to update multiple
    //avatars outside of the main update function.
    m_avatarScene.simulate(0.f);

    m_avatarThumbs[index].clear(cro::Colour::Transparent);
    //m_avatarThumbs[index].clear(cro::Colour::Magenta);
    m_avatarScene.render();
    m_avatarThumbs[index].display();
}

void MenuState::setPreviewModel(std::size_t playerIndex)
{
    auto index = m_avatarIndices[playerIndex];
    auto flipped = m_sharedData.localConnectionData.playerData[playerIndex].flipped;

    //hmm this would be quicker if we just tracked the active model...
    //in fact it might be contributing to the slow down when entering main lobby.
    for (auto i = 0u; i < m_playerAvatars.size(); ++i)
    {
        if (m_playerAvatars[i].previewModel.isValid()
            && m_playerAvatars[i].previewModel.hasComponent<cro::Model>())
        {
            m_playerAvatars[i].previewModel.getComponent<cro::Model>().setHidden(i != index);
            m_playerAvatars[i].previewModel.getComponent<cro::Transform>().setScale(glm::vec3(0.f));

            if (i == index)
            {
                if (flipped)
                {
                    m_playerAvatars[i].previewModel.getComponent<cro::Transform>().setScale({ -1.f, 1.f, 1.f });
                    m_playerAvatars[i].previewModel.getComponent<cro::Model>().setFacing(cro::Model::Facing::Back);
                }
                else
                {
                    m_playerAvatars[i].previewModel.getComponent<cro::Transform>().setScale({ 1.f, 1.f, 1.f });
                    m_playerAvatars[i].previewModel.getComponent<cro::Model>().setFacing(cro::Model::Facing::Front);
                }
                auto texID = cro::TextureID(m_sharedData.avatarTextures[0][playerIndex].getGLHandle());
                m_playerAvatars[i].previewModel.getComponent<cro::Model>().setMaterialProperty(0, "u_diffuseMap", texID);
            }
        }
    }
}

void MenuState::saveAvatars()
{
    cro::ConfigFile cfg("avatars");
    for (const auto& player : m_sharedData.localConnectionData.playerData)
    {
        auto* avatar = cfg.addObject("avatar");
        avatar->addProperty("name", player.name.empty() ? "Player" : player.name.toAnsiString()); //hmmm shame we can't save the encoding here
        avatar->addProperty("skin_id").setValue(player.skinID);
        avatar->addProperty("flipped").setValue(player.flipped);
        avatar->addProperty("ball_id").setValue(player.ballID);
        avatar->addProperty("flags0").setValue(player.avatarFlags[0]);
        avatar->addProperty("flags1").setValue(player.avatarFlags[1]);
        avatar->addProperty("flags2").setValue(player.avatarFlags[2]);
        avatar->addProperty("flags3").setValue(player.avatarFlags[3]);
    }

    auto path = cro::App::getPreferencePath() + "avatars.cfg";
    cfg.save(path);
}

void MenuState::loadAvatars()
{
    auto path = cro::App::getPreferencePath() + "avatars.cfg";
    cro::ConfigFile cfg;
    if (cfg.loadFromFile(path, false))
    {
        std::uint32_t i = 0;

        const auto& objects = cfg.getObjects();
        for (const auto& obj : objects)
        {
            if (obj.getName() == "avatar"
                && i < m_sharedData.localConnectionData.MaxPlayers)
            {
                const auto& props = obj.getProperties();
                for (const auto& prop : props)
                {
                    const auto& name = prop.getName();
                    if (name == "name")
                    {
                        //TODO try running this through unicode parser
                        m_sharedData.localConnectionData.playerData[i].name = prop.getValue<std::string>();
                    }
                    else if (name == "skin_id")
                    {
                        auto id = prop.getValue<std::uint32_t>();
                        m_sharedData.localConnectionData.playerData[i].skinID = id;
                    }
                    else if (name == "flipped")
                    {
                        m_sharedData.localConnectionData.playerData[i].flipped = prop.getValue<bool>();
                    }
                    else if (name == "ball_id")
                    {
                        auto id = prop.getValue<std::uint32_t>();
                        m_sharedData.localConnectionData.playerData[i].ballID = id;
                    }
                    else if (name == "flags0")
                    {
                        auto flag = prop.getValue<std::int32_t>();
                        flag = std::min(pc::ColourID::Count - 1, std::max(0, flag));
                        m_sharedData.localConnectionData.playerData[i].avatarFlags[0] = static_cast<std::uint8_t>(flag);
                    }
                    else if (name == "flags1")
                    {
                        auto flag = prop.getValue<std::int32_t>();
                        flag = std::min(pc::ColourID::Count - 1, std::max(0, flag));
                        m_sharedData.localConnectionData.playerData[i].avatarFlags[1] = static_cast<std::uint8_t>(flag);
                    }
                    else if (name == "flags2")
                    {
                        auto flag = prop.getValue<std::int32_t>();
                        flag = std::min(pc::ColourID::Count - 1, std::max(0, flag));
                        m_sharedData.localConnectionData.playerData[i].avatarFlags[2] = static_cast<std::uint8_t>(flag);
                    }
                    else if (name == "flags3")
                    {
                        auto flag = prop.getValue<std::int32_t>();
                        flag = std::min(pc::ColourID::Count - 1, std::max(0, flag));
                        m_sharedData.localConnectionData.playerData[i].avatarFlags[3] = static_cast<std::uint8_t>(flag);
                    }
                }

                i++;
            }
        }
    }

    if (m_sharedData.localConnectionData.playerData[0].name.empty())
    {
        m_sharedData.localConnectionData.playerData[0].name = RandomNames[cro::Util::Random::value(0u, RandomNames.size() - 1)];
    }
}

std::int32_t MenuState::indexFromHairID(std::uint32_t id)
{


    return 0;
}