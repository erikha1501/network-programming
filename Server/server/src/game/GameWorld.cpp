#include "GameWorld.hpp"

#include <box2d/b2_chain_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_shape.h>

#include "extension/b2MathExtension.hpp"

#include <random>

using namespace game::extension;

namespace game
{

GameWorld::GameWorld()
    : m_playerList{}, m_bombList{}, m_powerupList{}, m_world({0.0f, 0.0f}),
      m_contactListener(this), m_bodiesToBeDestroyed{}
{
    m_tileMap = map::TileMap{map::TileMapTemplate::getDefault()};

    m_world.SetContactListener(&m_contactListener);
}

void GameWorld::addPlayer(uint id)
{
    m_playerList[id].setState(PlayerEntity::State::Dead);
}

void GameWorld::initialize()
{
    const float mapWidth = m_tileMap.width();
    const float mapHeight = m_tileMap.height();

    // Create ground
    b2BodyDef groundBodyDef;
    groundBodyDef.type = b2_staticBody;
    groundBodyDef.position.Set(0.0f, 0.0f);

    b2Vec2 groundVertices[4] = {{0.0f, 0.0f}, {0.0f, mapHeight}, {mapWidth, mapHeight}, {mapWidth, 0.0f}};
    b2ChainShape groundShape;
    groundShape.CreateLoop(groundVertices, 4);

    b2Body* groundBody = m_world.CreateBody(&groundBodyDef);
    groundBody->CreateFixture(&groundShape, 0.0f);

    // Reusable wall Fixture
    b2PolygonShape wallShape;
    wallShape.SetAsBox(0.5f, 0.5f);

    b2FixtureDef wallFixtureDef;
    wallFixtureDef.shape = &wallShape;
    wallFixtureDef.density = 0.0f;
    wallFixtureDef.friction = 0.0f;

    // Populate map
    const uint width = m_tileMap.width();
    const uint height = m_tileMap.height();

    std::vector<map::Coord2D> breakableWallPositions;

    for (uint y = 0; y < height; y++)
    {
        for (uint x = 0; x < width; x++)
        {
            map::Tile& tile = m_tileMap.at(x, y);

            switch (tile.type())
            {
            case map::TileType::Empty:
                break;
            case map::TileType::Wall: {
                wallShape.SetAsBox(0.5f, 0.5f, b2Vec2{0.5f + x, 0.5f + y}, 0.0f);
                groundBody->CreateFixture(&wallFixtureDef);
                break;
            }
            case map::TileType::Breakable: {
                wallShape.SetAsBox(0.5f, 0.5f, b2Vec2{0.5f + x, 0.5f + y}, 0.0f);
                b2Fixture* wallFixture = groundBody->CreateFixture(&wallFixtureDef);
                tile.setCustomData(wallFixture);

                breakableWallPositions.emplace_back(map::Coord2D{x, y});
                break;
            }
            default:
                break;
            }
        }
    }

    // Place random powerups
    constexpr std::array<PowerupEntity::Type, 3> powerups = {
        PowerupEntity::Type::BombQuantity, PowerupEntity::Type::BombRange, PowerupEntity::Type::Speedup};

    // Randomization strategy:
    // 50% of the time:             no powerup
    // other 50% of the time:       uniformly choose between n powerups
    //
    // Let # of powerups = n
    // ---------------------------------------------------------------------------------------
    // |0%     (0.5 * 1/n)     (0.5 * 2/n)     (...)     (0.5 * n/n)=50%                 100%|
    // |    #1      |      #2       |      #i    |    #n      |               none           |
    // ---------------------------------------------------------------------------------------
    //
    // The threshhold value (50% in the example above) can be configured

    constexpr float randomThresholdValue = 0.5f;

    uint randomSeed = std::random_device()();
    std::default_random_engine randomEngine{randomSeed};
    std::uniform_real_distribution uniformDistribution;

    for (const auto& breakablePosition : breakableWallPositions)
    {
        double randomValue = uniformDistribution(randomEngine);

        if (randomValue >= randomThresholdValue)
        {
            continue;
        }

        // Transform [0, randomThresholdValue) -> [0, 1) -> powerup index
        float powerupIndex = uint(randomValue / randomThresholdValue * powerups.size());
        PowerupEntity::Type powerupType = powerups[powerupIndex];

        m_powerupList.emplace(breakablePosition, PowerupEntity(breakablePosition.x, breakablePosition.y, powerupType));
    }

    // Create players
    b2BodyDef playerBodyDef;
    playerBodyDef.type = b2_dynamicBody;
    playerBodyDef.fixedRotation = true;

    b2CircleShape playerColliderShape;
    playerColliderShape.m_p.Set(0.0f, 0.0f);
    playerColliderShape.m_radius = 0.49f;

    b2FixtureDef playerColliderFixtureDef;
    playerColliderFixtureDef.shape = &playerColliderShape;
    playerColliderFixtureDef.density = 1.0f;
    playerColliderFixtureDef.friction = 0.1f;
    playerColliderFixtureDef.filter.categoryBits = Entity::playerCategoryBits;
    playerColliderFixtureDef.filter.maskBits = Entity::playerMaskBits;

    for (size_t i = 0; i < m_playerList.size(); i++)
    {
        if (m_playerList[i].state() == PlayerEntity::State::Invalid)
        {
            continue;
        }

        // TODO: Use spawn position
        playerBodyDef.position.Set(0.5f + m_tileMap.playerSpawnPosition(i).x,
                                   0.5f + m_tileMap.playerSpawnPosition(i).y);

        b2Body* body = m_world.CreateBody(&playerBodyDef);
        body->CreateFixture(&playerColliderFixtureDef);

        m_playerList[i] = PlayerEntity{body};

        // Store entity as body's user data
        Entity* entity = &m_playerList[i];
        body->GetUserData().pointer = reinterpret_cast<uintptr_t>(entity);
    }
}

const map::TileMap& GameWorld::tileMap() const
{
    return m_tileMap;
}

PlayerEntity::InputData& GameWorld::playerInput(uint id)
{
    return m_playerList[id].inputData();
}

const PlayerEntity& GameWorld::playerEntity(uint id) const
{
    return m_playerList[id];
}

std::vector<uint> GameWorld::getAlivePlayerIDs() const
{
    std::vector<uint> result{};
    for (size_t i = 0; i < m_playerList.size(); i++)
    {
        if (m_playerList[i].state() == PlayerEntity::State::Alive)
        {
            result.emplace_back(i);
        }
    }

    return result;
}

const std::deque<BombEntity>& GameWorld::bombList() const
{
    return m_bombList;
}

const std::unordered_map<map::Coord2D, PowerupEntity, map::Coord2D::Hasher>& GameWorld::powerupList() const
{
    return m_powerupList;
}

void GameWorld::applyPlayerInput()
{
    for (auto& player : m_playerList)
    {
        if (player.state() != PlayerEntity::State::Alive)
        {
            continue;
        }

        player.body()->SetLinearVelocity(normalized(player.inputData().direction) * player.stats().speed());

        // Check for bomb placement input
        if (player.inputData().bombPlaced)
        {
            tryPlaceBomb(player.body()->GetPosition(), &player);
        }

        // Clear bomb placement flag
        player.inputData().bombPlaced = false;
    }
}

void GameWorld::physicStep(time_duration deltaTime)
{
    m_world.Step(deltaTime.count(), 6, 2);
}

void GameWorld::gameLogicStep(time_duration deltaTime)
{
    // Collect player positions and update tile map
    m_tileMap.clearPlayerPresence();

    for (size_t i = 0; i < m_playerList.size(); i++)
    {
        const PlayerEntity& playerEntity = m_playerList[i];

        if (playerEntity.state() != PlayerEntity::State::Alive)
        {
            continue;
        }

        b2Vec2 playerPosition = playerEntity.body()->GetPosition();

        m_tileMap.at(playerPosition.x, playerPosition.y).setPresence(map::EntityMask::s_playerIDToMask[i]);
    }

    // Update bomb detonation timer
    for (auto& bombEntity : m_bombList)
    {
        bombEntity.tickDetonationTimer(deltaTime);
    }

    // Check for detonation
    while (!m_bombList.empty())
    {
        BombEntity& bombEntity = m_bombList.front();

        if (bombEntity.hasDetonated())
        {
            onBombDetonation(bombEntity);
            bombEntity.placer()->stats().incrementCarryingBombCount();
            m_bombList.pop_front();
        }
        else
        {
            // If this bomb hasn't detonated yet, subsequent bombs will not
            break;
        }
    }

    executeBodyDestructionRoutine();
}

bool GameWorld::checkEndCondition() const
{
    uint aliveCount = 0;

    for (const auto& playerEntity : m_playerList)
    {
        if (playerEntity.state() == PlayerEntity::State::Alive)
        {
            aliveCount++;
        }
    }

    return aliveCount <= g_gameEndConditionPlayerAliveCount;
}

void GameWorld::tryPlaceBomb(b2Vec2 bombPosition, PlayerEntity* placer)
{
    if (placer->stats().carryingBombCount() == 0)
    {
        return;
    }

    uint tileX = bombPosition.x;
    uint tileY = bombPosition.y;

    // Check for any bomb already placed at this potition
    if (m_tileMap.at(tileX, tileY).testPresence(map::EntityMask::Bomb))
    {
        return;
    }

    // Create bomb
    b2BodyDef bombBodyDef;
    bombBodyDef.type = b2_staticBody;
    bombBodyDef.position.Set(0.5f + tileX, 0.5f + tileY);

    b2CircleShape bombColliderShape;
    bombColliderShape.m_p.Set(0.0f, 0.0f);
    // TODO: Use global configuration
    bombColliderShape.m_radius = 0.5f;

    b2Body* bombBody = m_world.CreateBody(&bombBodyDef);
    bombBody->CreateFixture(&bombColliderShape, 0.0f);

    // Create bomb entity
    uint bombRange = placer->stats().bombRange();
    BombEntity& newBombEntity = m_bombList.emplace_back(tileX, tileY, bombRange, bombBody, placer);
    m_tileMap.at(tileX, tileY).setPresence(map::EntityMask::Bomb);

    // Pointers to bomb entities will stay valid if
    // we only insert/delete to/from the beginning or the end.
    Entity* entity = &newBombEntity;
    bombBody->GetUserData().pointer = (uintptr_t)entity;

    placer->stats().decrementCarryingBombCount();
}

void GameWorld::onBombDetonation(BombEntity& bombEntity)
{
    // TODO: Move this to global configuration
    constexpr int bombDetonationRange = 2;

    // Remove this bomb from entity map
    m_tileMap.at(bombEntity.positionX(), bombEntity.positionY()).clearPresence(map::EntityMask::Bomb);

    // Destroy bomb's body
    // TODO: Maybe move destruction process to Entity's destructor
    m_world.DestroyBody(bombEntity.body());

    // Check for entities inside detonation range.
    // This process starts at the detonation point and goes through all 4 directions.
    onBombHitTile(bombEntity.positionX(), bombEntity.positionY());

    constexpr struct
    {
        int dx;
        int dy;
    } directions[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (size_t dirIndex = 0; dirIndex < 4; dirIndex++)
    {
        int tileX = bombEntity.positionX();
        int tileY = bombEntity.positionY();

        for (int r = 0; r < bombDetonationRange; r++)
        {
            tileX += directions[dirIndex].dx;
            tileY += directions[dirIndex].dy;

            if (onBombHitTile(tileX, tileY) == false)
            {
                break;
            }
        }
    }
}

bool GameWorld::onBombHitTile(uint tileX, uint tileY)
{
    // Map bounds check
    if (tileX < 0 || tileX >= m_tileMap.width() || tileY < 0 || tileY >= m_tileMap.height())
    {
        return false;
    }

    map::TileType tileType = m_tileMap.at(tileX, tileY).type();

    if (tileType == map::TileType::Wall)
    {
        return false;
    }
    else if (tileType == map::TileType::Breakable)
    {
        onBombHitBreakable(tileX, tileY);
        return false;
    }
    else if (tileType == map::TileType::Empty)
    {
        // Check if any player in range
        for (size_t playerID = 0; playerID < m_playerList.size(); playerID++)
        {
            if (m_tileMap.at(tileX, tileY).testPresence(map::EntityMask::s_playerIDToMask[playerID]))
            {
                onBombHitPlayer(tileX, tileY, playerID);
            }
        }
    }

    return true;
}

void GameWorld::onBombHitPlayer(uint positionX, uint positionY, uint playerID)
{
    PlayerEntity& playerEntity = m_playerList[playerID];

    // TODO: Change this to support multiple lives
    m_world.DestroyBody(playerEntity.body());
    playerEntity.setState(PlayerEntity::State::Dead);
    m_tileMap.at(positionX, positionY).clearPresence(map::EntityMask::s_playerIDToMask[playerID]);
}

void GameWorld::onBombHitBreakable(uint positionX, uint positionY)
{
    // Handle tile breaking
    map::Tile& tile = m_tileMap.at(positionX, positionY);

    // Destroy wall's fixture
    b2Fixture* tileFixture = (b2Fixture*)tile.customData();
    tileFixture->GetBody()->DestroyFixture(tileFixture);

    tile.setType(map::TileType::Empty);
    tile.setCustomData(nullptr);

    // Reaveal powerup if there is one
    auto powerupIterator = m_powerupList.find(map::Coord2D{positionX, positionY});
    if (powerupIterator != m_powerupList.end())
    {
        b2BodyDef powerupBodyDef;
        powerupBodyDef.type = b2_staticBody;
        powerupBodyDef.position.Set(0.5f + positionX, 0.5f + positionY);

        b2CircleShape powerupColliderShape;
        powerupColliderShape.m_p.Set(0.0f, 0.0f);
        powerupColliderShape.m_radius = 0.4f;

        b2FixtureDef powerupColliderFixtureDef;
        powerupColliderFixtureDef.shape = &powerupColliderShape;
        powerupColliderFixtureDef.density = 0.0f;
        powerupColliderFixtureDef.friction = 0.0f;
        powerupColliderFixtureDef.isSensor = true;

        b2Body* powerupBody = m_world.CreateBody(&powerupBodyDef);
        powerupBody->CreateFixture(&powerupColliderFixtureDef);

        // Activate powerup entity
        PowerupEntity& powerupEntity = powerupIterator->second;
        powerupEntity.assignBody(powerupBody);
        powerupEntity.setActive(true);

        // It is safe to use pointers to powerup entities.
        // std::unordered_map guarantees that references to elements
        // stay valid in all cases.
        Entity* entity = &powerupEntity;
        powerupBody->GetUserData().pointer = (uintptr_t)entity;

        tile.setPresence(map::EntityMask::Powerup);
    }
}

void GameWorld::onPlayerPickupPowerup(PlayerEntity* playerEntity, PowerupEntity* powerupEntity)
{
    assert(powerupEntity->isActive());
    switch (powerupEntity->powerupType())
    {
    case PowerupEntity::Type::BombQuantity: {
        playerEntity->stats().upgradeBombCapacity();
        break;
    }
    case PowerupEntity::Type::BombRange: {
        playerEntity->stats().upgradeBombRange();
        break;
    }
    case PowerupEntity::Type::Speedup: {
        playerEntity->stats().upgradeSpeed();
        break;
    }
    default:
        break;
    }

    map::Tile& tile = m_tileMap.at(powerupEntity->positionX(), powerupEntity->positionY());
    tile.clearPresence(map::EntityMask::Powerup);

    registerBodyDestruction(powerupEntity->body());
    powerupEntity->setActive(false);
}

// Must be careful not to add the same body twice
void GameWorld::registerBodyDestruction(b2Body* body)
{
    m_bodiesToBeDestroyed.emplace_back(body);
}

void GameWorld::executeBodyDestructionRoutine()
{
    while (!m_bodiesToBeDestroyed.empty())
    {
        b2Body* body = m_bodiesToBeDestroyed.front();
        m_world.DestroyBody(body);

        m_bodiesToBeDestroyed.pop_front();
    }
    
    
}

} // namespace game
