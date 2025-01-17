/*-----------------------------------------------------------------------

Matt Marchant 2017 - 2020
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

#include "../detail/enet/enet/enet.h"

#include "NetConf.hpp"

#include <crogine/network/NetClient.hpp>
#include <crogine/core/Log.hpp>
#include <crogine/detail/Assert.hpp>

#include <cstring>

using namespace cro;

NetClient::NetClient()
    : m_client      (nullptr),
    m_threadRunning (false)
{
    if (!NetConf::instance)
    {
        NetConf::instance = std::make_unique<NetConf>();
    }
}

NetClient::~NetClient()
{
    if (m_peer.m_peer)
    {
        disconnect();
    }
    
    if (m_client)
    {
        enet_host_destroy(m_client);
    }
}


//public
bool NetClient::create(std::size_t maxChannels, std::size_t maxClients, std::uint32_t incoming, std::uint32_t outgoing)
{
    if (m_client)
    {
        disconnect();
        enet_host_destroy(m_client);
    }

    if (!NetConf::instance->m_initOK)
    {
        Logger::log("Failed creating client host, Network not initialised", Logger::Type::Error);
        return false;
    }

    CRO_ASSERT(maxChannels > 0, "Invalid channel count");
    CRO_ASSERT(maxClients > 0, "Invalid client count");

    m_client = enet_host_create(nullptr, maxClients, maxChannels, incoming, outgoing);
    if (!m_client)
    {
        Logger::log("Creating net client failed.", Logger::Type::Error);
        return false;
    }

    enet_host_compress_with_range_coder(m_client);

    LOG("Created client host", Logger::Type::Info);
    return true;
}

bool NetClient::connect(const std::string& address, std::uint16_t port, std::uint32_t timeout)
{
    CRO_ASSERT(timeout > 0, "Timeout should probably be at least 1000ms");
    CRO_ASSERT(port > 0, "Invalid port number");
    CRO_ASSERT(!address.empty(), "Invalid address string");

    if (m_peer.m_peer)
    {
        disconnect();
    }

    if (!m_client)
    {
        //must call create() successfully first!
        Logger::log("Unable to connect, call NetClient::create() first!", Logger::Type::Error);
        return false;
    }

    ENetAddress add;
    if (enet_address_set_host(&add, address.c_str()) != 0)
    {
        Logger::log("Failed to parse address from " + address + ", client failed to connect", Logger::Type::Error);
        return false;
    }
    add.port = port;

    m_peer.m_peer = enet_host_connect(m_client, &add, m_client->channelLimit, 0);
    if (!m_peer.m_peer)
    {
        Logger::log("Failed assigning peer connection to host", Logger::Type::Error);
        return false;
    }

    //wait for a success event from server - this part is blocking
    ENetEvent evt;
    if (enet_host_service(m_client, &evt, timeout) > 0 && evt.type == ENET_EVENT_TYPE_CONNECT)
    {
        //this is a hack to allow for long(er) loading times when the main
        //thread is unable to poll a connection to keep it alive
        enet_peer_timeout(m_peer.m_peer, ENET_PEER_TIMEOUT_LIMIT * 2, ENET_PEER_TIMEOUT_MINIMUM * 2, ENET_PEER_TIMEOUT_MAXIMUM * 2);


        //ehhhhh... it actually works, but ofc introduces latency.
        //m_threadRunning = true;
        //m_thread = std::make_unique<std::thread>(&NetClient::threadFunc, this);

        LOG("Connected to " + address, Logger::Type::Info);
        return true;
    }

    enet_peer_reset(m_peer.m_peer);
    m_peer.m_peer = nullptr;
    Logger::log("Connection attempt timed out after " + std::to_string(timeout) + " milliseconds.", Logger::Type::Error);
    return false;
}

void NetClient::disconnect()
{
    /*m_threadRunning = false;
    m_thread->join();
    m_thread.reset();*/

    if (m_peer.m_peer)
    {
        ENetEvent evt;
        enet_peer_disconnect(m_peer.m_peer, 0);

        //wait 3 seconds for a response
        while (enet_host_service(m_client, &evt, 3000) > 0)
        {
            switch (evt.type)
            {
            default:break;
            case ENET_EVENT_TYPE_RECEIVE:
                //clear rx'd packets from buffer by destroying them
                enet_packet_destroy(evt.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT: //um what if this is another peer disconnecting at the same time?
                m_peer.m_peer = nullptr;
                LOG("Disconnected from server", Logger::Type::Info);
                return;
            }
        }

        //timed out so force disconnect
        LOG("Disconnect timed out", Logger::Type::Info);
        enet_peer_reset(m_peer.m_peer);
        m_peer.m_peer = nullptr;
    }
}

bool NetClient::pollEvent(NetEvent& evt)
{
    if (!m_client) return false;

    ENetEvent hostEvt;
    if (enet_host_service(m_client, &hostEvt, 0) > 0)
    //if (!m_activeBuffer.empty())
    {
        //auto hostEvt = std::any_cast<ENetEvent>(m_activeBuffer.front());
        //m_activeBuffer.pop_front();

        switch (hostEvt.type)
        {
        default:
            evt.type = NetEvent::None;
            break;
        case ENET_EVENT_TYPE_CONNECT:
            evt.type = NetEvent::ClientConnect;
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            evt.type = NetEvent::ClientDisconnect;            
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            evt.type = NetEvent::PacketReceived;
            evt.packet.setPacketData(hostEvt.packet);
            //our event takes ownership
            //enet_packet_destroy(hostEvt.packet);
            break;
        }
        evt.peer.m_peer = hostEvt.peer;
        return true;
    }

    {
        //std::lock_guard<std::mutex> lock(m_mutex);
        //m_activeBuffer.swap(m_evtBuffer);
    }
    return false;
}

void NetClient::sendPacket(std::uint8_t id, const void* data, std::size_t size, NetFlag flags, std::uint8_t channel)
{
    if (m_peer.m_peer)
    {
        std::int32_t packetFlags = 0;
        if (flags == NetFlag::Reliable)
        {
            packetFlags |= ENET_PACKET_FLAG_RELIABLE;
        }
        else if (flags == NetFlag::Unreliable)
        {
            packetFlags |= ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
        }
        else if (flags == NetFlag::Unsequenced)
        {
            packetFlags |= ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT | ENET_PACKET_FLAG_UNSEQUENCED;
        }
        
        ENetPacket* packet = enet_packet_create(&id, sizeof(std::uint8_t), packetFlags);
        enet_packet_resize(packet, sizeof(std::uint8_t) + size);
        std::memcpy(&packet->data[sizeof(std::uint8_t)], data, size);

        enet_peer_send(m_peer.m_peer, channel, packet);
    }
}

//private
void NetClient::threadFunc()
{
    while (m_threadRunning)
    {
        ENetEvent hostEvt;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (enet_host_service(m_client, &hostEvt, 0) != 0)
            {
                m_evtBuffer.push_back(std::make_any<ENetEvent>(hostEvt));
            }
        }

        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(50));
    }
}