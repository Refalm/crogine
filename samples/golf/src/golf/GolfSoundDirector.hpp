/*-----------------------------------------------------------------------

Matt Marchant 2021 - 2022
http://trederia.blogspot.com

Super Video Golf - zlib licence.

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

#pragma once

#include "SoundEffectsDirector.hpp"
#include "CommonConsts.hpp"
#include "SharedStateData.hpp"

#include <crogine/audio/AudioScape.hpp>

#include <vector>
#include <array>

namespace cro
{
    class AudioResource;
    class AudioSource;
}

class GolfSoundDirector final : public SoundEffectsDirector
{
public:
    explicit GolfSoundDirector(cro::AudioResource&);

    void handleMessage(const cro::Message&) override;

    void addAudioScape(const std::string& path, cro::AudioResource& resource);
    void setPlayerIndex(std::size_t client, std::size_t player, std::int32_t index);
    void setActivePlayer(std::size_t client, std::size_t player);

private:
    struct AudioID final
    {
        enum
        {
            Putt01,
            Putt02,
            Putt03,

            Swing01,
            Swing02,
            Swing03,

            Wedge,

            Hole,
            NearHole,
            NearMiss,
            Water,
            Ground,
            Scrub,
            Stone,

            ScoreAlbatross,
            ScoreBirdie,
            ScoreBogie,
            ScoreDoubleBogie,
            ScoreTripleBogie,
            ScoreEagle,
            ScoreHole,
            ScorePar,
            ScoreHIO,
            ScoreInOne,

            Draw01,
            Draw02,
            Win,
            Lose,

            Applause,
            ApplausePlus,

            TerrainBunker01,
            TerrainBunker02,
            TerrainBunker03,
            TerrainBunker04,
            TerrainBunker05,
            TerrainFairway01,
            TerrainFairway02,
            TerrainGreen01,
            TerrainGreen02,
            TerrainRough01,
            TerrainRough02,
            TerrainScrub01,
            TerrainScrub02,
            TerrainScrub03,
            TerrainScrub04,
            TerrainWater01,
            TerrainWater02,
            TerrainWater03,

            Honour,
            NiceSwing01,
            NiceSwing02,
            NiceSwing03,

            Hook,
            Slice,

            DriveExcellent,
            DriveGood,
            DriveBad,

            NicePutt01,
            NicePutt02,
            Gimme,

            Burst,
            Airmail,
            Birds,
            BillboardSwing,
            BillboardRewind,
            Foot01,
            Foot02,
            Foot03,
            Foot04,

            Count
        };
    };
    std::array<const cro::AudioSource*, AudioID::Count> m_audioSources = {};

    std::array<std::array<std::int32_t, ConnectionData::MaxPlayers>, ConstVal::MaxClients> m_playerIndices = {};
    std::vector<cro::AudioScape> m_playerVoices;
    std::size_t m_currentClient;
    std::size_t m_currentPlayer;
    std::size_t m_honourID;
    bool m_newHole;

    cro::Entity playSound(std::int32_t, glm::vec3, float = 1.f);
    void playSoundDelayed(std::int32_t, glm::vec3, float, float = 1.f, std::uint8_t = 1/*MixerChannel::Effects*/);
    cro::Entity playAvatarSound(std::int32_t, const std::string&, glm::vec3);
    void playAvatarSoundDelayed(std::int32_t, const std::string&, glm::vec3, float);

    //not really sound, but we want to trigger it in sync (ish) with audio
    void applaud();
};