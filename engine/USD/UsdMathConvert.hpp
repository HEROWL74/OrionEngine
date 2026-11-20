// engine/USD/UsdMathConvert.hpp

#pragma once
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include "Math/Math.hpp"

namespace Engine::USD
{
    inline Math::Vector3 ToVector3(const pxr::GfVec3f& v)
    {
        return Math::Vector3(v[0], v[1], v[2]);
    }
    
    inline Math::Vector3 ToVector3(const pxr::GfVec3d& v)
    {
        return Math::Vector3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
    }
    
    inline Math::Matrix4 ToMatrix4(const pxr::GfMatrix4d& m)
    {
        Math::Matrix4 out;
        for (int r = 0; r < 4;++r)
        {
            for (int c = 0; c < 4;c++)
            {
                out.m[r][c] = static_cast<float>(m[r][c]);
            }
        }
        
        return out;
    }
    
}
