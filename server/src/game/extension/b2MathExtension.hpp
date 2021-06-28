#pragma once

#include <box2d/b2_math.h>

namespace game::extension
{

b2Vec2 operator*(const b2Vec2& vec, float mul)
{
    return b2Vec2(vec.x * mul, vec.y * mul);
}

b2Vec2 normalized(const b2Vec2& vec)
{
    b2Vec2 t = vec;
    t.Normalize();
    return t;
}



} // namespace game::extension
