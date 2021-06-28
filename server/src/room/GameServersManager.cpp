#include "GameServersManager.hpp"

#include <stdio.h>

namespace room
{

void GameServersManager::start()
{
    m_running = true;
    m_cleanupThread = std::thread{&GameServersManager::cleaningProcedure, this};
}
void GameServersManager::stop()
{
    // Try to stop all game servers
    {
        std::unique_lock<std::mutex> entryListLock{m_entryListMutex};

        // Stop all game servers
        for (auto& [id, gameServerEntry] : m_gameServerEntryList)
        {
            gameServerEntry.stopGameServer();
        }

        // Wait for all game servers to be removed
        m_cvGameServerRemovedEvent.wait(entryListLock, [this]() { return this->m_gameServerEntryList.size() == 0; });
    }

    // Try to stop cleaning thread
    m_running = false;
    // Cleaning thread might be waiting for game server ended event.
    // Send notification so it can exit
    m_cvGameServerEndedEvent.notify_all();
}

void GameServersManager::wait()
{
    if (m_cleanupThread.joinable())
    {
        m_cleanupThread.join();
    }
}

void GameServersManager::cleaningProcedure()
{
    std::unique_lock<std::mutex> idQueueLock{m_endedIDQueueMutex};

    while (m_running)
    {
        // Wait for either stop is requested or
        // any game server has ended.
        m_cvGameServerEndedEvent.wait(idQueueLock, [this]() 
        {
            return this->m_running == false || this->m_endedGameServerIDQueue.size() > 0; 
        });

        if (!m_endedGameServerIDQueue.empty())
        {
            // Try to remove ended game servers
            do
            {
                uint gameServerID = m_endedGameServerIDQueue.front();
                m_endedGameServerIDQueue.pop();

                game::GameServer& gameServer = m_gameServerEntryList.get(gameServerID)->gameServer();
                gameServer.wait();

                {
                    std::scoped_lock entryListLock{m_entryListMutex};
                    bool removeResult = m_gameServerEntryList.remove(gameServerID);
                    assert(removeResult);
                    printf("[GameServersManager] GameServer #%d removed\n", gameServerID);
                }
            } while (!m_endedGameServerIDQueue.empty());

            m_cvGameServerRemovedEvent.notify_all();
        }
    }
}

bool GameServersManager::dispatchGameServer(std::unique_ptr<game::GameServer> gameServer)
{
    auto iterator = m_gameServerEntryList.end();

    {
        std::scoped_lock entryListLock{m_entryListMutex};
        auto [tempIterator, result] = m_gameServerEntryList.add(GameServerEntry{std::move(gameServer), this});

        if (!result)
        {
            return false;
        }

        iterator = tempIterator;
    }

    uint gameServerID = iterator->first;
    GameServerEntry& gameServerEntry = iterator->second;

    gameServerEntry.setID(gameServerID);
    gameServerEntry.startGameServer();

    return true;
}

void GameServersManager::onGameServerEnded(game::GameServer& gameServer, uint gameServerID)
{
    {
        std::scoped_lock idQueueLock{m_endedIDQueueMutex};
        m_endedGameServerIDQueue.push(gameServerID);

        if (m_gameServerEndedEventCallback)
        {
            m_gameServerEndedEventCallback(gameServer);
        }

        // Notify cleaning procedure
        m_cvGameServerEndedEvent.notify_all();
    }
}

} // namespace room
