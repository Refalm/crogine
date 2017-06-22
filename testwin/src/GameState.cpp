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

#include "GameState.hpp"
#include "ResourceIDs.hpp"
#include "BackgroundShader.hpp"
#include "BackgroundSystem.hpp"
#include "PostRadial.hpp"
#include "RotateSystem.hpp"
#include "TerrainChunk.hpp"
#include "ChunkBuilder.hpp"
#include "Messages.hpp"
#include "RockFallSystem.hpp"
#include "RandomTranslation.hpp"
#include "VelocitySystem.hpp"
#include "PlayerDirector.hpp"
#include "BackgroundDirector.hpp"
#include "ItemSystem.hpp"
#include "ItemsDirector.hpp"
#include "NPCSystem.hpp"
#include "NpcDirector.hpp"
#include "PlayerSystem.hpp"
#include "PlayerWeaponsSystem.hpp"

#include <crogine/core/App.hpp>
#include <crogine/core/Clock.hpp>

#include <crogine/graphics/QuadBuilder.hpp>
#include <crogine/graphics/StaticMeshBuilder.hpp>
#include <crogine/graphics/IqmBuilder.hpp>
#include <crogine/graphics/SpriteSheet.hpp>

#include <crogine/ecs/systems/SceneGraph.hpp>
#include <crogine/ecs/systems/ModelRenderer.hpp>
#include <crogine/ecs/systems/ParticleSystem.hpp>
#include <crogine/ecs/systems/CommandSystem.hpp>
#include <crogine/ecs/systems/SkeletalAnimator.hpp>
#include <crogine/ecs/systems/CollisionSystem.hpp>
#include <crogine/ecs/systems/SpriteRenderer.hpp>

#include <crogine/ecs/components/Transform.hpp>
#include <crogine/ecs/components/Model.hpp>
#include <crogine/ecs/components/Camera.hpp>
#include <crogine/ecs/components/ParticleEmitter.hpp>
#include <crogine/ecs/components/CommandID.hpp>
#include <crogine/ecs/components/PhysicsObject.hpp>

#include <crogine/util/Random.hpp>
#include <crogine/util/Constants.hpp>
#include <crogine/graphics/postprocess/PostChromeAB.hpp>

namespace
{
    const glm::vec2 backgroundSize(21.3f, 7.2f);
    std::size_t rockfallCount = 2;
}

GameState::GameState(cro::StateStack& stack, cro::State::Context context)
    : cro::State        (stack, context),
    m_scene             (context.appInstance.getMessageBus())
{
    context.mainWindow.loadResources([this]() {
        addSystems();
        loadAssets();
        createScene();
    });
    //context.appInstance.setClearColour(cro::Colour::White());
    //context.mainWindow.setVsyncEnabled(false);

    updateView();

    auto* msg = getContext().appInstance.getMessageBus().post<GameEvent>(MessageID::GameMessage);
    msg->type = GameEvent::RoundStart;

    context.appInstance.resetFrameTime();
}

//public
bool GameState::handleEvent(const cro::Event& evt)
{
    if (evt.type == SDL_KEYUP)
    {

    }

    m_scene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const cro::Message& msg)
{
    m_scene.forwardMessage(msg);

    if (msg.id == cro::Message::WindowMessage)
    {
        const auto& data = msg.getData<cro::Message::WindowEvent>();
        if (data.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            updateView();
        }
    }
}

bool GameState::simulate(cro::Time dt)
{
    m_scene.simulate(dt);
    return true;
}

void GameState::render()
{
    m_scene.render();
}

//private
void GameState::addSystems()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<BackgroundSystem>(mb);
    m_scene.addSystem<ChunkSystem>(mb);
    m_scene.addSystem<RockFallSystem>(mb);
    m_scene.addSystem<RotateSystem>(mb);   
    m_scene.addSystem<Translator>(mb);
    m_scene.addSystem<cro::CommandSystem>(mb);
    m_scene.addSystem<VelocitySystem>(mb);
    m_scene.addSystem<cro::SkeletalAnimator>(mb);
    m_scene.addSystem<ItemSystem>(mb);
    m_scene.addSystem<NpcSystem>(mb);
    m_scene.addSystem<PlayerSystem>(mb);
    m_scene.addSystem<PlayerWeaponSystem>(mb);
    m_scene.addSystem<cro::SceneGraph>(mb);
    m_scene.addSystem<cro::CollisionSystem>(mb);
    m_scene.addSystem<cro::ModelRenderer>(mb);
    m_scene.addSystem<cro::ParticleSystem>(mb);
    m_scene.addSystem<cro::SpriteRenderer>(mb);

    m_scene.addDirector<PlayerDirector>();
    m_scene.addDirector<BackgroundDirector>();
    m_scene.addDirector<ItemDirector>();
    m_scene.addDirector<NpcDirector>();
#ifdef PLATFORM_DESKTOP
    //m_scene.addPostProcess<cro::PostChromeAB>();
    m_scene.addPostProcess<PostRadial>();
#endif
}

void GameState::loadAssets()
{
    m_resources.shaders.preloadFromString(Shaders::Background::Vertex, Shaders::Background::Fragment, ShaderID::Background);
    auto& farTexture = m_resources.textures.get("assets/materials/background_far.png");
    farTexture.setRepeated(true);
    farTexture.setSmooth(true);
    auto& farMaterial = m_resources.materials.add(MaterialID::GameBackgroundFar, m_resources.shaders.get(ShaderID::Background));
    farMaterial.setProperty("u_diffuseMap", farTexture);

    auto& midTexture = m_resources.textures.get("assets/materials/background_mid.png");
    midTexture.setRepeated(true);
    midTexture.setSmooth(true);
    auto& midMaterial = m_resources.materials.add(MaterialID::GameBackgroundMid, m_resources.shaders.get(ShaderID::Background));
    midMaterial.setProperty("u_diffuseMap", midTexture);
    midMaterial.blendMode = cro::Material::BlendMode::Alpha;

    auto& nearTexture = m_resources.textures.get("assets/materials/background_near.png");
    nearTexture.setRepeated(true);
    nearTexture.setSmooth(true);
    auto& nearMaterial = m_resources.materials.add(MaterialID::GameBackgroundNear, m_resources.shaders.get(ShaderID::Background));
    nearMaterial.setProperty("u_diffuseMap", nearTexture);
    nearMaterial.blendMode = cro::Material::BlendMode::Alpha;

    cro::QuadBuilder qb(backgroundSize);
    m_resources.meshes.loadMesh(MeshID::GameBackground, qb);


    m_modelDefs[GameModelID::Player].loadFromFile("assets/models/player.cmt", m_resources);
    m_modelDefs[GameModelID::CollectableBatt].loadFromFile("assets/models/collectable_batt.cmt", m_resources);
    m_modelDefs[GameModelID::CollectableBomb].loadFromFile("assets/models/collectable_bomb.cmt", m_resources);
    m_modelDefs[GameModelID::CollectableBot].loadFromFile("assets/models/collectable_bot.cmt", m_resources);
    m_modelDefs[GameModelID::CollectableHeart].loadFromFile("assets/models/collectable_heart.cmt", m_resources);
    m_modelDefs[GameModelID::CollectableShield].loadFromFile("assets/models/collectable_shield.cmt", m_resources);
    m_modelDefs[GameModelID::Elite].loadFromFile("assets/models/elite.cmt", m_resources);
    m_modelDefs[GameModelID::TurretBase].loadFromFile("assets/models/turret_base.cmt", m_resources);
    m_modelDefs[GameModelID::TurretCannon].loadFromFile("assets/models/turret_cannon.cmt", m_resources);
    m_modelDefs[GameModelID::Choppa].loadFromFile("assets/models/choppa.cmt", m_resources);
    m_modelDefs[GameModelID::Speedray].loadFromFile("assets/models/speed.cmt", m_resources);
    m_modelDefs[GameModelID::WeaverHead].loadFromFile("assets/models/weaver_head.cmt", m_resources);
    m_modelDefs[GameModelID::WeaverBody].loadFromFile("assets/models/weaver_body.cmt", m_resources);


    auto shaderID = m_resources.shaders.preloadBuiltIn(cro::ShaderResource::BuiltIn::Unlit, cro::ShaderResource::VertexColour);
    m_resources.materials.add(MaterialID::TerrainChunk, m_resources.shaders.get(shaderID));

    ChunkBuilder chunkBuilder;
    m_resources.meshes.loadMesh(MeshID::TerrainChunkA, chunkBuilder);
    m_resources.meshes.loadMesh(MeshID::TerrainChunkB, chunkBuilder);


    shaderID = m_resources.shaders.preloadBuiltIn(cro::ShaderResource::Unlit, cro::ShaderResource::DiffuseMap | cro::ShaderResource::Subrects);
    for (auto i = 0u; i < rockfallCount; ++i)
    {
        auto& rockMat = m_resources.materials.add(MaterialID::Rockfall + i, m_resources.shaders.get(shaderID));
        rockMat.setProperty("u_diffuseMap", m_resources.textures.get("assets/materials/npc/tites.png"));
        rockMat.setProperty("u_subrect", glm::vec4(0.f, 0.f, 0.25f, 1.f));
        rockMat.blendMode = cro::Material::BlendMode::Alpha;
    }
    cro::QuadBuilder rockQuad({ 1.f, 1.f });
    m_resources.meshes.loadMesh(MeshID::RockQuad, rockQuad);
}

void GameState::createScene()
{
    //background layers
    auto entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 0.f, 0.f, -18.f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::GameBackground), m_resources.materials.get(MaterialID::GameBackgroundFar));

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 0.f, 0.f, -14.f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::GameBackground), m_resources.materials.get(MaterialID::GameBackgroundMid));

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 0.f, 0.f, -11.f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::GameBackground), m_resources.materials.get(MaterialID::GameBackgroundNear));
    entity.addComponent<BackgroundComponent>();

    //terrain chunks
    cro::PhysicsShape boundsShape;
    boundsShape.type = cro::PhysicsShape::Type::Box;
    boundsShape.extent = { 10.65f, 0.6f, 0.2f };
    boundsShape.position = { 0.f, 2.85f, 0.f };

    entity = m_scene.createEntity();
    auto& chunkTxA = entity.addComponent<cro::Transform>();
    chunkTxA.setPosition({ 0.f, 0.f, -8.8f });
    //chunkTxA.setScale({ 0.5f, 0.5f, 1.f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::TerrainChunkA), m_resources.materials.get(MaterialID::TerrainChunk));
    entity.addComponent<TerrainChunk>();
    entity.addComponent<cro::PhysicsObject>().addShape(boundsShape);
    boundsShape.position.y = -2.85f;
    entity.getComponent<cro::PhysicsObject>().addShape(boundsShape);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Environment);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    auto chunkEntityA = entity; //keep this so we can attach turrets to it

    entity = m_scene.createEntity();
    auto& chunkTxB = entity.addComponent<cro::Transform>();
    chunkTxB.setPosition({ backgroundSize.x, 0.f, -8.8f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::TerrainChunkB), m_resources.materials.get(MaterialID::TerrainChunk));
    entity.addComponent<TerrainChunk>();
    entity.addComponent<cro::PhysicsObject>().addShape(boundsShape);
    boundsShape.position.y = 2.85f;
    entity.getComponent<cro::PhysicsObject>().addShape(boundsShape);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Environment);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    auto chunkEntityB = entity;

    //left/right bounds
    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 0.f, 0.f, -9.3f });
    entity.addComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Bounds);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player | CollisionID::PlayerLaser);
    boundsShape.extent = { 1.25f, 4.f, 0.5f };
    boundsShape.position.y = 0.f;
    boundsShape.position.x = -6.25f;
    entity.getComponent<cro::PhysicsObject>().addShape(boundsShape);
    boundsShape.position.x = 6.25f;
    entity.getComponent<cro::PhysicsObject>().addShape(boundsShape);

    //some rockfall parts
    glm::vec3 rockscale(0.6f, 1.2f, 1.f);
    auto bb = m_resources.meshes.getMesh(m_modelDefs[MeshID::RockQuad].meshID).boundingBox;
    cro::PhysicsShape ps;
    ps.type = cro::PhysicsShape::Type::Box;
    ps.extent = (bb[1] - bb[0]) * rockscale / 2.f;
    ps.extent.z = 0.1f;
    
    for (auto i = 0u; i < rockfallCount; ++i)
    {
        entity = m_scene.createEntity();
        entity.addComponent<RockFall>();
        auto& tx = entity.addComponent<cro::Transform>();
        tx.setScale(rockscale);
        tx.setPosition({ 0.f, 3.4f, -9.1f });
        entity.addComponent<cro::PhysicsObject>().addShape(ps);
        entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Environment);
        entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);
        entity.addComponent<cro::Model>(m_resources.meshes.getMesh(MeshID::RockQuad), m_resources.materials.get(MaterialID::Rockfall + i));
    }

    //player ship
    entity = m_scene.createEntity();
    auto& playerTx = entity.addComponent<cro::Transform>();
    playerTx.setPosition({ -35.4f, 0.f, -9.3f });
    playerTx.setScale({ 0.5f, 0.5f, 0.5f });
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::Player].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::Player].materialIDs[0]));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Player;
    entity.addComponent<Velocity>().friction = 2.5f;
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::Player].meshID).boundingBox;
    ps.extent = (bb[1] - bb[0]) * playerTx.getScale() / 2.f;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Player);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Collectable | CollisionID::Environment | CollisionID::NPC | CollisionID::Bounds);
    entity.addComponent<PlayerInfo>();
    auto playerEntity = entity;

    //collectables
    static const glm::vec3 coinScale(0.15f);
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableBatt].meshID).boundingBox;
    ps.extent = (bb[1] - bb[0]) * coinScale / 2.f;
    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.41f, 1.4f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(coinScale);
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableBatt].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::CollectableBatt].materialIDs[0]));
    auto& battSpin = entity.addComponent<Rotator>();
    battSpin.axis.y = 1.f;
    battSpin.speed = 3.2f;
    entity.addComponent<CollectableItem>().type = CollectableItem::EMP;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Collectable;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Collectable);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.42f, 0.6f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(coinScale);
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableBomb].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::CollectableBomb].materialIDs[0]));
    auto& bombSpin = entity.addComponent<Rotator>();
    bombSpin.axis.y = 1.f;
    bombSpin.speed = 2.9f;
    entity.addComponent<CollectableItem>().type = CollectableItem::Bomb;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Collectable;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Collectable);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.6f, -0.2f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(coinScale);
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableBot].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::CollectableBot].materialIDs[0]));
    auto& botSpin = entity.addComponent<Rotator>();
    botSpin.axis.y = 1.f;
    botSpin.speed = 2.994f;
    entity.addComponent<CollectableItem>().type = CollectableItem::Buddy;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Collectable;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Collectable);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.38f, -1.f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(coinScale);
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableHeart].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::CollectableHeart].materialIDs[0]));
    auto& heartSpin = entity.addComponent<Rotator>();
    heartSpin.axis.y = 1.f;
    heartSpin.speed = 2.873f;
    entity.addComponent<CollectableItem>().type = CollectableItem::Life;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Collectable;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Collectable);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.4f, -1.7f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(coinScale);
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::CollectableShield].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::CollectableShield].materialIDs[0]));
    auto& shieldSpin = entity.addComponent<Rotator>();
    shieldSpin.axis.y = 1.f;
    shieldSpin.speed = 3.028f;
    entity.addComponent<CollectableItem>().type = CollectableItem::Shield;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Collectable;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::Collectable);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);


    //----NPCs----//
    //elite
    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 5.9f, 1.5f, -9.3f });
    entity.getComponent<cro::Transform>().setScale(glm::vec3(0.8f));
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::Elite].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::Elite].materialIDs[0]));
    entity.addComponent<Npc>().type = Npc::Elite;
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Elite;
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::Elite].meshID).boundingBox;
    ps.extent = (bb[1] - bb[0]) * entity.getComponent<cro::Transform>().getScale() / 2.f;
    entity.addComponent<cro::PhysicsObject>().addShape(ps);
    entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::NPC);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);
    
    //choppa
    const float choppaSpacing = ChoppaNavigator::spacing;
    glm::vec3 choppaScale(0.8f);
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::Choppa].meshID).boundingBox;
    ps.extent = (bb[1] - bb[0]) * choppaScale / 2.f;
    for (auto i = 0; i < 3; ++i)
    {
        entity = m_scene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 5.9f, -choppaSpacing + (i * choppaSpacing), -9.3f });
        entity.getComponent<cro::Transform>().setRotation({ -cro::Util::Const::PI / 2.f, 0.f, 0.f });
        entity.getComponent<cro::Transform>().setScale(choppaScale);
        entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::Choppa].meshID),
                                        m_resources.materials.get(m_modelDefs[GameModelID::Choppa].materialIDs[0]));
        CRO_ASSERT(m_modelDefs[GameModelID::Choppa].skeleton, "Skeleton missing from choppa!");
        entity.addComponent<cro::Skeleton>() = *m_modelDefs[GameModelID::Choppa].skeleton;
        entity.getComponent<cro::Skeleton>().play(0);
        entity.addComponent<Npc>().type = Npc::Choppa;
        entity.getComponent<Npc>().choppa.ident = i;
        entity.addComponent<cro::CommandTarget>().ID = CommandID::Choppa;
        entity.addComponent<cro::PhysicsObject>().addShape(ps);
        entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::NPC);
        entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);
    }

    //speedray
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::Speedray].meshID).boundingBox;
    ps.extent = (bb[1] - bb[0]) / 2.f;
    for (auto i = 0u; i < SpeedrayNavigator::count; ++i)
    {
        const float rotateOffset = (cro::Util::Const::PI / SpeedrayNavigator::count) * i;
        
        entity = m_scene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ 6.f, 0.f, -9.3f });
        entity.getComponent<cro::Transform>().rotate(glm::vec3(1.f, 0.f, 0.f), cro::Util::Const::PI - rotateOffset);
        entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::Speedray].meshID),
            m_resources.materials.get(m_modelDefs[GameModelID::Speedray].materialIDs[0]));
        entity.getComponent<cro::Model>().setMaterial(1, m_resources.materials.get(m_modelDefs[GameModelID::Speedray].materialIDs[1]));
        auto& speedRot = entity.addComponent<Rotator>();
        speedRot.axis.x = 1.f;
        speedRot.speed = -cro::Util::Const::TAU / 2.f;
        entity.addComponent<Npc>().type = Npc::Speedray;
        entity.getComponent<Npc>().speedray.ident = i;
        entity.addComponent<cro::CommandTarget>().ID = CommandID::Speedray;
        entity.addComponent<cro::PhysicsObject>().addShape(ps);
        entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::NPC);
        entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);
    }

    //attach turret to each of the terrain chunks
    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 10.f, 0.f, 0.f }); //places off screen to start
    entity.getComponent<cro::Transform>().setScale({ 0.3f, 0.3f, 0.3f });
    entity.getComponent<cro::Transform>().setParent(chunkEntityA); //attach to scenery
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::TurretBase].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::TurretBase].materialIDs[0]));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Turret;
    
    auto canEnt = m_scene.createEntity();
    canEnt.addComponent<cro::Transform>().setParent(entity);
    canEnt.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::TurretCannon].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::TurretCannon].materialIDs[0]));
    canEnt.addComponent<Npc>().type = Npc::Turret;

    entity = m_scene.createEntity();
    entity.addComponent<cro::Transform>().setPosition({ 10.f, 0.f, 0.f });
    entity.getComponent<cro::Transform>().setScale({ 0.3f, 0.3f, 0.3f });
    entity.getComponent<cro::Transform>().setParent(chunkEntityB); //attach to scenery
    entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::TurretBase].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::TurretBase].materialIDs[0]));
    entity.addComponent<cro::CommandTarget>().ID = CommandID::Turret;

    canEnt = m_scene.createEntity();
    canEnt.addComponent<cro::Transform>().setParent(entity);
    canEnt.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[GameModelID::TurretCannon].meshID),
                                    m_resources.materials.get(m_modelDefs[GameModelID::TurretCannon].materialIDs[0]));
    canEnt.addComponent<Npc>().type = Npc::Turret;


    //weaver
    glm::vec3 weaverScale(0.5f);
    bb = m_resources.meshes.getMesh(m_modelDefs[GameModelID::WeaverBody].meshID).boundingBox;
    for (auto i = 0u; i < WeaverNavigator::count; ++i)
    {
        cro::int32 modelID = (i == 0) ? GameModelID::WeaverHead : GameModelID::WeaverBody;
        
        entity = m_scene.createEntity();
        entity.addComponent<cro::Transform>().setPosition({ (static_cast<float>(i) * (WeaverNavigator::spacing * weaverScale.x)) + 10.f, 0.f, -9.3f });
        entity.getComponent<cro::Transform>().setScale(weaverScale);
        entity.getComponent<cro::Transform>().rotate({ 1.f, 0.f, 0.f }, static_cast<float>(i) * 0.3f);
        entity.getComponent<cro::Transform>().rotate({ 0.f, 1.f, 0.f }, cro::Util::Const::PI / 2.f);
        entity.addComponent<cro::Model>(m_resources.meshes.getMesh(m_modelDefs[modelID].meshID),
                                        m_resources.materials.get(m_modelDefs[modelID].materialIDs[0]));
        entity.addComponent<Npc>().type = Npc::Weaver;
        entity.getComponent<Npc>().weaver.ident = WeaverNavigator::count - i;
        entity.addComponent<cro::CommandTarget>().ID = CommandID::Weaver;
        auto& rot = entity.addComponent<Rotator>();
        rot.axis.z = 1.f;
        rot.speed = 2.f;

        ps.extent = ((bb[1] - bb[0]) / 2.f) * weaverScale;
        entity.addComponent<cro::PhysicsObject>().addShape(ps);
        entity.getComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::NPC);
        entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Player);

        weaverScale *= 0.92f;
    }

    //particle systems
    entity = m_scene.createEntity();
    auto& snowEmitter = entity.addComponent<cro::ParticleEmitter>();
    auto& settings = snowEmitter.emitterSettings;
    settings.emitRate = 30.f;
    settings.initialVelocity = { -2.4f, -0.1f, 0.f };
    settings.gravity = { 0.f, -1.f, 0.f };
    settings.colour = cro::Colour::White();
    settings.lifetime = 5.f;
    settings.rotationSpeed = 5.f;
    settings.size = 0.03f;
    settings.spawnRadius = 0.8f;
    settings.textureID = m_resources.textures.get("assets/particles/snowflake.png").getGLHandle();
    settings.blendmode = cro::EmitterSettings::Add;

    snowEmitter.start();
    entity.addComponent<cro::Transform>();
    auto& translator = entity.addComponent<RandomTranslation>();
    for (auto& p : translator.translations)
    {
        p.x = cro::Util::Random::value(-3.5f, 12.1f);
        p.y = 3.1f;
        p.z = -9.2f;
    }
    entity.addComponent<cro::CommandTarget>().ID = CommandID::SnowParticles;

    //rock fragments from ceiling
    entity = m_scene.createEntity();
    auto& rockEmitter = entity.addComponent<cro::ParticleEmitter>();
    auto& rockSettings = rockEmitter.emitterSettings;
    rockSettings.emitRate = 4.f;
    rockSettings.initialVelocity = {};
    rockSettings.lifetime = 2.f;
    rockSettings.rotationSpeed = 8.f;
    rockSettings.size = 0.06f;
    rockSettings.textureID = m_resources.textures.get("assets/particles/rock_fragment.png").getGLHandle();
    rockSettings.blendmode = cro::EmitterSettings::Alpha;
    rockSettings.gravity = { 0.f, -9.f, 0.f };

    entity.addComponent<cro::Transform>();
    auto& rockTrans = entity.addComponent<RandomTranslation>();
    for (auto& p : rockTrans.translations)
    {
        p.x = cro::Util::Random::value(-4.2f, 4.2f);
        p.y = 3.f;
        p.z = -8.6f;
    }
    entity.addComponent<cro::CommandTarget>().ID = CommandID::RockParticles;

    //weapons sprites
    cro::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/lasers.spt", m_resources.textures);
    glm::vec3 pulseScale(0.006f);
    const std::size_t maxPulses = 20;
    for (auto i = 0u; i < maxPulses; ++i)
    {
        entity = m_scene.createEntity();
        entity.addComponent<cro::Sprite>() = spriteSheet.getSprite("player_pulse");
        auto size = entity.getComponent<cro::Sprite>().getSize();

        entity.addComponent<cro::Transform>().setScale(pulseScale);
        entity.getComponent<cro::Transform>().setPosition(glm::vec3(-10.f));
        entity.getComponent<cro::Transform>().setOrigin({ size.x / 2.f, size.y / 2.f, 0.f });

        cro::PhysicsShape ps;
        ps.type = cro::PhysicsShape::Type::Box;
        ps.extent = { size.x * pulseScale.x, size.y * pulseScale.y, 0.2f };
        ps.extent /= 2.f;
        ps.extent *= glm::vec3(0.85f);

        entity.addComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::PlayerLaser);
        entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Bounds | CollisionID::Collectable | CollisionID::NPC);
        entity.getComponent<cro::PhysicsObject>().addShape(ps);

        entity.addComponent<PlayerWeapon>();
    }

    //use a single laser we'll scale it to make it look bigger or smaller
    glm::vec3 laserScale(0.1f, 0.006f, 1.f);
    entity = m_scene.createEntity();
    entity.addComponent<cro::Sprite>() = spriteSheet.getSprite("player_laser");
    auto size = entity.getComponent<cro::Sprite>().getSize();

    entity.addComponent<cro::Transform>().setScale(laserScale);
    entity.getComponent<cro::Transform>().setPosition(glm::vec3(-9.3f));
    //entity.getComponent<cro::Transform>().setOrigin({ 0.f, size.y / 2.f, 0.f });
    entity.getComponent<cro::Transform>().setParent(playerEntity);

    cro::PhysicsShape laserPhys;
    laserPhys.type = cro::PhysicsShape::Type::Box;
    laserPhys.extent = { size.x * laserScale.x, size.y * laserScale.y, 0.2f };
    laserPhys.extent /= 2.f;
    laserPhys.extent *= glm::vec3(0.85f);

    entity.addComponent<cro::PhysicsObject>().setCollisionGroups(CollisionID::PlayerLaser);
    entity.getComponent<cro::PhysicsObject>().setCollisionFlags(CollisionID::Collectable | CollisionID::NPC);
    entity.getComponent<cro::PhysicsObject>().addShape(laserPhys);

    entity.addComponent<PlayerWeapon>().type = PlayerWeapon::Type::Laser;


    //3D camera
    auto ent = m_scene.createEntity();
    ent.addComponent<cro::Transform>();
    ent.addComponent<cro::Camera>();
    m_scene.setActiveCamera(ent);
}

void GameState::updateView()
{
    glm::vec2 size(cro::App::getWindow().getSize());
    size.y = ((size.x / 16.f) * 9.f) / size.y;
    size.x = 1.f;

    auto& cam3D = m_scene.getActiveCamera().getComponent<cro::Camera>();
    cam3D.projection = glm::perspective(0.6f, 16.f / 9.f, 0.1f, 100.f);
    cam3D.viewport.bottom = (1.f - size.y) / 2.f;
    cam3D.viewport.height = size.y;

    /*auto& cam2D = m_menuScene.getActiveCamera().getComponent<cro::Camera>();
    cam2D.viewport = cam3D.viewport;*/
}