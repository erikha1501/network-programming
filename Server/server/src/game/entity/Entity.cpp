#include "Entity.hpp"

namespace game
{
    
static uint s_idGenerator = 0;

uint Entity::nextID()
{
    return s_idGenerator++;
}

} // namespace gam
