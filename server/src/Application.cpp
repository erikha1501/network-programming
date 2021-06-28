#include "room/RoomServer.hpp"

#include <stdio.h>
#include <signal.h>

void sigintHandler(int signum);

struct Context
{
    room::RoomServer* server;
} context;

int main()
{
    // Register SIGINT handler
    struct sigaction new_action{};
    struct sigaction old_action{};
    new_action.sa_flags = SA_RESTART;
    new_action.sa_handler = &sigintHandler;

    sigaction(SIGINT, &new_action, &old_action);

    constexpr uint16_t port = 6969;
    constexpr int maxRoomCount = 4;

    room::RoomServer server(port, maxRoomCount);
    context.server = &server;

    server.start();
    server.stop();
    server.wait();


    // char choice;
    // while (true)
    // {
    //     scanf(" %c", &choice);

    //     if (choice == 'q')
    //     {
    //         // printf("Stopping game instace\n");
    //         // gameServer.stop();
    //         break;
    //     }
    // }

    // game::GameServer gameServer{0, 6970};
    // gameServer.registerPlayer(0, IPEndpoint());
    // gameServer.start();

    // char choice;
    // while (true)
    // {
    //     scanf(" %c", &choice);

    //     if (choice == 'q')
    //     {
    //         // printf("Stopping game instace\n");
    //         // gameServer.stop();
    //         break;
    //     }
    // }

    // gameServer.wait();
    // assert(gameServer.errorCode() == game::GameServer::ErrorCode::InsufficientPlayer);
}

void sigintHandler(int signum)
{
    if (signum == SIGINT)
    {
        context.server->stop();
    }
}