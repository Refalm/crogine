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

#pragma once

#include "server/Server.hpp"

#include <crogine/network/NetClient.hpp>
#include <crogine/core/String.hpp>

#include <string>
#include <array>

struct PlayerData final
{
    cro::String name;
    std::uint32_t avatarFlags = 0;
};

struct ConnectionData final
{
    static constexpr std::uint8_t MaxPlayers = 4;
    std::uint8_t connectionID = MaxPlayers;

    std::uint8_t playerCount = 0;
    std::array<PlayerData, MaxPlayers> playerData = {};

    std::vector<std::uint8_t> serialise() const;
    bool deserialise(const cro::NetEvent::Packet&);
};

struct SharedStateData final
{
    Server serverInstance;

    struct ClientConnection final
    {
        cro::NetClient netClient;
        bool connected = false;
        bool ready = false;
        std::uint8_t connectionID = 4;
    }clientConnection;

    //data of all players rx'd from server
    std::array<ConnectionData, 4u> connectionData = {};

    //our local player data
    ConnectionData localPlayer;
    cro::String targetIP;

    //printed by the error state
    std::string errorMessage;
};