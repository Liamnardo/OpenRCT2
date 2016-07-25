#pragma region Copyright (c) 2014-2016 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#include <set>
#include "../core/Console.hpp"
#include "../core/Guard.hpp"
#include "../core/Json.hpp"
#include "../core/Memory.hpp"
#include "../core/String.hpp"
#include "Network2.h"
#include "NetworkChat.h"
#include "NetworkClient.h"
#include "NetworkConnection.h"
#include "NetworkGroupManager.h"
#include "NetworkKey.h"
#include "NetworkPlayerManager.h"
#include "NetworkTypes.h"
#include "TcpSocket.h"

extern "C"
{
    #include "../config.h"
    #include "../game.h"
    #include "../interface/window.h"
    #include "../localisation/localisation.h"
    #include "../platform/platform.h"
    #include "../util/util.h"
}

class NetworkClient : public INetworkClient
{
private:
    NETWORK_CLIENT_STATUS   _status = NETWORK_CLIENT_STATUS_NONE;
    SOCKET_STATUS           _lastConnectStatus = SOCKET_STATUS_CLOSED;
    NetworkConnection *     _serverConnection = nullptr;
    NetworkKey              _key;
    std::vector<uint8>      _challenge;
    std::vector<uint8>      _mapBuffer;

    uint32                      _serverTick = 0;
    uint32                      _serverSrand0 = 0;
    uint32                      _serverSrand0Tick = 0;
    bool                        _desynchronised = false;
    std::multiset<GameCommand>  _gameCommandQueue;

    INetworkChat *          _chat           = nullptr;
    INetworkGroupManager *  _groupManager   = nullptr;
    INetworkPlayerList *    _playerList     = nullptr;

    NetworkServerInfo       _serverInfo = { nullptr };

    uint8                   _playerId = 255;

public:
    NetworkClient()
    {
        _chat = CreateChat();
        _groupManager = CreateGroupManager();
        _playerList = CreatePlayerList(_groupManager);
    }

    virtual ~NetworkClient()
    {
        delete _chat;
    }

    NETWORK_AUTH GetAuthStatus() const override
    {
        return _serverConnection->AuthStatus;
    }

    NETWORK_CLIENT_STATUS GetConnectionStatus() const override
    {
        return _status;
    }

    uint32 GetServerTick() const override
    {
        return _serverTick;
    }

    INetworkChat * GetNetworkChat() const override
    {
        return _chat;
    }

    INetworkGroupManager * GetGroupManager() const override
    {
        return _groupManager;
    }

    INetworkPlayerList * GetPlayerList() const override
    {
        return _playerList;
    }

    NetworkServerInfo GetServerInfo() const override
    {
        return _serverInfo;
    }

    uint8 GetPlayerId() const override
    {
        return _playerId;
    }

    bool Begin(const char * host, uint16 port) override
    {
        Close();

        Guard::Assert(Network2::GetMode() == NETWORK_MODE_NONE, GUARD_LINE);
        Network2::SetMode(NETWORK_MODE_CLIENT);

        Guard::Assert(_serverConnection == nullptr);
        _serverConnection = new NetworkConnection();
        _serverConnection->Socket = CreateTcpSocket();
        _serverConnection->Socket->ConnectAsync(host, port);
        _status = NETWORK_CLIENT_STATUS_CONNECTING;
        _lastConnectStatus = SOCKET_STATUS_CLOSED;

        _chat->StartLogging();

        if (!SetupUserKey())
        {
            return false;
        }
        return true;
    }

    void Close() override
    {
        Network2::SetMode(NETWORK_MODE_NONE);
    }

    void Update() override
    {

    }

    void HandleChallenge(const char * challenge, size_t challengeSize) override
    {
        _challenge.resize(challengeSize);
        memcpy(_challenge.data(), challenge, challengeSize);

        std::string pubkey = _key.PublicKeyString();
        if (!LoadPrivateKey())
        {
            _serverConnection->SetLastDisconnectReason(STR_MULTIPLAYER_VERIFICATION_FAILURE);
            _serverConnection->Socket->Disconnect();
            return;
        }

        char * signature;
        size_t sigsize;
        bool result = _key.Sign(_challenge.data(), _challenge.size(), &signature, &sigsize);
        if (!result)
        {
            Console::Error::WriteLine("Failed to sign server's challenge.");
            _serverConnection->SetLastDisconnectReason(STR_MULTIPLAYER_VERIFICATION_FAILURE);
            _serverConnection->Socket->Disconnect();
            return;
        }

        // Don't keep private key in memory. There's no need and it may get leaked
        // when process dump gets collected at some point in future.
        _key.Unload();

        SendAuthentication(gConfigNetwork.player_name, "", pubkey.c_str(), signature, sigsize);
        delete[] signature;
    }

    void SendPassword(const utf8 * password) override
    {
        std::string pubkey = _key.PublicKeyString();
        if (!LoadPrivateKey())
        {
            return;
        }

        char * signature;
        size_t sigsize;
        _key.Sign(_challenge.data(), _challenge.size(), &signature, &sigsize);

        // Don't keep private key in memory. There's no need and it may get leaked
        // when process dump gets collected at some point in future.
        _key.Unload();

        SendAuthentication(gConfigNetwork.player_name, password, pubkey.c_str(), signature, sigsize);
        delete[] signature;
    }

    void RequestGameInfo() override
    {
        log_verbose("requesting gameinfo");
        std::unique_ptr<NetworkPacket> packet = std::move(NetworkPacket::Allocate());
        *packet << (uint32)NETWORK_COMMAND_GAMEINFO;
        _serverConnection->QueuePacket(std::move(packet));
    }

    void ReceiveMap(size_t totalDataSize, size_t offset, void * dataChunk, size_t dataChunkSize) override
    {
        char str_downloading_map[256];
        unsigned int downloading_map_args[2] = { (offset + dataChunkSize) / 1024, totalDataSize / 1024 };
        format_string(str_downloading_map, STR_MULTIPLAYER_DOWNLOADING_MAP, downloading_map_args);
        window_network_status_open(str_downloading_map, []() -> void
        {
            INetworkContext * context = Network2::GetContext();
            if (context != nullptr)
            {
                context->Close();
            }
        });

        if (totalDataSize > _mapBuffer.size())
        {
            _mapBuffer.resize(totalDataSize);
        }

        Memory::Copy<void>(_mapBuffer.data() + offset, dataChunk, dataChunkSize);

        if (offset + dataChunkSize == totalDataSize)
        {
            window_network_status_close();
            ProcessMap(_mapBuffer.data(), _mapBuffer.size());

            // Discard map buffer data (free the memory)
            _mapBuffer.resize(0);
        }
    }

    void ReceiveChatMessage(const utf8 * message) override
    {
        _chat->ShowMessage(message);
    }
    
    void RecieveGameCommand(const GameCommand * gameCommand) override
    {
        _gameCommandQueue.insert(*gameCommand);
    }

    void RecieveTick(uint32 tick, uint32 srand0) override
    {
        _serverTick = tick;
        if (_serverSrand0Tick == 0)
        {
            _serverSrand0 = srand0;
            _serverSrand0Tick = tick;
        }
    }

    void RecieveServerInfo(const char * json) override
    {
        json_error_t error;
        json_t * root = json_loads(json, 0, &error);
        if (root == nullptr)
        {
            Console::Error::WriteLine("Recieved invalid ServerInfo json.");
        }
        else
        {
            _serverInfo.Name = json_string_value(json_object_get(root, "name"));
            _serverInfo.Description = json_string_value(json_object_get(root, "description"));

            json_t * jsonProvider = json_object_get(root, "provider");
            if (jsonProvider != nullptr)
            {
                _serverInfo.Provider.Name = json_string_value(json_object_get(jsonProvider, "name"));
                _serverInfo.Provider.Email = json_string_value(json_object_get(jsonProvider, "email"));
                _serverInfo.Provider.Website = json_string_value(json_object_get(jsonProvider, "website"));
            }

            json_decref(root);
        }
    }

    void SendPing() override
    {
        std::unique_ptr<NetworkPacket> packet = std::move(NetworkPacket::Allocate());
        *packet << (uint32)NETWORK_COMMAND_PING;
        _serverConnection->QueuePacket(std::move(packet));
    }

    void SendChatMessage(const utf8 * text) override
    {

    }

    void SendGameCommand(uint32 eax, uint32 ebx, uint32 ecx, uint32 edx, uint32 esi, uint32 edi, uint32 ebp, uint8 callbackId) override
    {

    }

private:
    bool SetupUserKey()
    {
        utf8 keyPath[MAX_PATH];
        NetworkKey::GetPrivateKeyPath(keyPath, sizeof(keyPath), gConfigNetwork.player_name);
        if (!platform_file_exists(keyPath))
        {
            Console::WriteLine("Generating key... This may take a while");
            Console::WriteLine("Need to collect enough entropy from the system");
            _key.Generate();
            Console::WriteLine("Key generated, saving private bits as %s", keyPath);

            utf8 keysDirectory[MAX_PATH];
            NetworkKey::GetKeysDirectory(keysDirectory, sizeof(keysDirectory));
            if (!platform_ensure_directory_exists(keysDirectory))
            {
                log_error("Unable to create directory %s.", keysDirectory);
                return false;
            }

            SDL_RWops * privkey = SDL_RWFromFile(keyPath, "wb+");
            if (privkey == nullptr)
            {
                log_error("Unable to save private key at %s.", keyPath);
                return false;
            }
            _key.SavePrivate(privkey);
            SDL_RWclose(privkey);

            const std::string &hash = _key.PublicKeyHash();
            const utf8 * publicKeyHash = hash.c_str();
            NetworkKey::GetPublicKeyPath(keyPath, sizeof(keyPath), gConfigNetwork.player_name, publicKeyHash);
            Console::WriteLine("Key generated, saving public bits as %s", keyPath);
            SDL_RWops * pubkey = SDL_RWFromFile(keyPath, "wb+");
            if (pubkey == nullptr)
            {
                log_error("Unable to save public key at %s.", keyPath);
                return false;
            }
            _key.SavePublic(pubkey);
            SDL_RWclose(pubkey);
            return true;
        }
        else
        {
            log_verbose("Loading key from %s", keyPath);
            SDL_RWops * privkey = SDL_RWFromFile(keyPath, "rb");
            if (privkey == nullptr)
            {
                log_error("Unable to read private key from %s.", keyPath);
                return false;
            }

            // LoadPrivate returns validity of loaded key
            bool ok = _key.LoadPrivate(privkey);
            SDL_RWclose(privkey);
            // Don't store private key in memory when it's not in use.
            _key.Unload();
            return ok;
        }
    }

    void SendAuthentication(const utf8 * playerName,
                            const utf8 * password,
                            const char * pubkey,
                            const char * signature,
                            size_t sigsize)
    {
        Guard::Assert(sigsize <= (size_t)UINT32_MAX, GUARD_LINE);

        std::unique_ptr<NetworkPacket> packet = std::move(NetworkPacket::Allocate());
        *packet << (uint32)NETWORK_COMMAND_AUTH;
        packet->WriteString(NETWORK_STREAM_ID);
        packet->WriteString(playerName);
        packet->WriteString(password);
        packet->WriteString(pubkey);
        *packet << (uint32)sigsize;
        packet->Write((const uint8 *)signature, sigsize);
        _serverConnection->AuthStatus = NETWORK_AUTH_REQUESTED;
        _serverConnection->QueuePacket(std::move(packet));
    }

    bool LoadPrivateKey()
    {
        bool result;
        utf8 keyPath[MAX_PATH];
        NetworkKey::GetPrivateKeyPath(keyPath, sizeof(keyPath), gConfigNetwork.player_name);
        if (platform_file_exists(keyPath))
        {
            SDL_RWops * privkey = SDL_RWFromFile(keyPath, "rb");
            result = _key.LoadPrivate(privkey);
            SDL_RWclose(privkey);
            if (!result)
            {
                Console::Error::WriteLine("Failed to load key '%s'", keyPath);
            }
        }
        else
        {
            Console::Error::WriteLine("Key file '%s' was not found. Restart client to re-generate it.", keyPath);
            result = false;
        }
        return result;
    }

    void ProcessMap(const void * mapData, size_t mapDataSize)
    {
        // Check if zlib compressed
        if (String::Equals((utf8 *)mapData, SV6_HEADER_ZLIB_COMPRESSED))
        {
            log_verbose("Received zlib-compressed sv6 map");

            size_t headerSize = String::SizeOf(SV6_HEADER_ZLIB_COMPRESSED) + 1;
            const void * bufferBody = (const void *)((uintptr_t)mapData + headerSize);
            size_t bufferBodySize = mapDataSize - headerSize;

            size_t sv6DataSize;
            uint8 * sv6Data = util_zlib_inflate((unsigned char *)bufferBody, bufferBodySize, &sv6DataSize);
            if (sv6Data == nullptr)
            {
                Console::Error::WriteLine("Failed to decompress map data sent from server.");
                Close();
            }
            else
            {
                LoadMap(sv6Data, sv6DataSize);
                Memory::Free(sv6Data);
            }
        }
        else
        {
            log_verbose("Assuming received map is in plain sv6 format");
            LoadMap(mapData, mapDataSize);
        }
    }

    void LoadMap(const void * sv6Data, size_t sv6DataSize)
    {
        SDL_RWops * rw = SDL_RWFromMem((void *)sv6Data, sv6DataSize);
        if (game_load_network(rw))
        {
            game_load_init();
            _gameCommandQueue.clear();
            _serverTick = gCurrentTicks;
            _serverSrand0Tick = 0;
            _desynchronised = false;

            // Notify user that they are now online and which shortcut key enables chat
            _chat->ShowChatHelp();
        }
        else
        {
            // Something went wrong, game is not loaded. Return to main screen.
            game_do_command(0, GAME_COMMAND_FLAG_APPLY, 0, 0, GAME_COMMAND_LOAD_OR_QUIT, 1, 0);
        }
        SDL_RWclose(rw);
    }
};

INetworkClient * CreateClient()
{
    return new NetworkClient();
}
