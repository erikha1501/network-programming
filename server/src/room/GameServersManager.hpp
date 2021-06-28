#pragma once

#include "../game/GameServer.hpp"
#include "../util/ActionCallback.hpp"
#include "../util/ReusableIdList.hpp"

#include <queue>

#include <condition_variable>
#include <mutex>
#include <thread>

#include <functional>

namespace room
{

class GameServersManager
{
private:
    class GameServerEntry : private util::ActionCallback
    {
    public:
        GameServerEntry(std::unique_ptr<game::GameServer> gameServer, GameServersManager* manager)
            : m_gameServer(std::move(gameServer)), m_manager(manager)
        {
        }

        GameServerEntry(const GameServerEntry&) = delete;
        GameServerEntry& operator=(const GameServerEntry&) = delete;
        GameServerEntry(GameServerEntry&&) = default;
        GameServerEntry& operator=(GameServerEntry&&) = default;

        const game::GameServer& gameServer() const
        {
            return *m_gameServer;
        }
        game::GameServer& gameServer()
        {
            return *m_gameServer;
        }

        void setID(uint gameServerID)
        {
            m_gameServerID = gameServerID;
        }

        void startGameServer()
        {
            m_gameServer->setServerID(m_gameServerID);
            m_gameServer->registerEndCallback(this);
            m_gameServer->start();
        }

        void stopGameServer()
        {
            m_gameServer->stop();
        }

    private:
        void invoke() override
        {
            m_manager->onGameServerEnded(*m_gameServer, m_gameServerID);
        }

    private:
        std::unique_ptr<game::GameServer> m_gameServer;

        uint m_gameServerID;
        GameServersManager* m_manager;
    };

public:
    GameServersManager(uint maxGameServerCount = -1)
        : m_gameServerEntryList(maxGameServerCount), m_entryListMutex{}, m_endedGameServerIDQueue{},
          m_endedIDQueueMutex{}, m_cvGameServerEndedEvent{}, m_running(false), m_cleanupThread{}
    {
    }

    void start();
    void stop();
    void wait();

    void registerGameServerEndedCallback(std::function<void(const game::GameServer&)> callback)
    {
        m_gameServerEndedEventCallback = std::move(callback);
    }

    bool isFull() const
    {
        return m_gameServerEntryList.isFull();
    }

    bool dispatchGameServer(std::unique_ptr<game::GameServer> gameServer);

private:
    void cleaningProcedure();

    // Called in ending GameServer's context
    void onGameServerEnded(game::GameServer& gameServer, uint gameServerID);

private:
    util::ReusableIdList<GameServerEntry> m_gameServerEntryList;
    std::mutex m_entryListMutex;

    std::queue<uint> m_endedGameServerIDQueue;
    std::mutex m_endedIDQueueMutex;

    // Cleaning procedure waits on this event.
    // GameServers notify.
    std::condition_variable m_cvGameServerEndedEvent;

    // GameServersManager::stop() waits on this event.
    // Cleaning procedure notifies.
    std::condition_variable m_cvGameServerRemovedEvent;

    std::function<void(const game::GameServer&)> m_gameServerEndedEventCallback;

    bool m_running;
    std::thread m_cleanupThread;
};

} // namespace room