#pragma once
#include "tmpl8math.h"

namespace Tmpl8
{
    struct mat4
    {
        // 4x4 Matrix (float)
        float cell[4][4] = { 0 };

        // Creates a matrix from a coordinate frame (right, up, forward) and a position
        static mat4 FromAxes(const float3& right, const float3& up, const float3& forward, const float3& pos)
        {
            mat4 matrix;

            // Right Axis
            matrix.cell[0][0] = right.x;
            matrix.cell[0][1] = right.y;
            matrix.cell[0][2] = right.z;
            matrix.cell[0][3] = 0;

            // Up Axis
            matrix.cell[1][0] = up.x;
            matrix.cell[1][1] = up.y;
            matrix.cell[1][2] = up.z;
            matrix.cell[1][3] = 0;

            // Forward Axis
            matrix.cell[2][0] = forward.x;
            matrix.cell[2][1] = forward.y;
            matrix.cell[2][2] = forward.z;
            matrix.cell[2][3] = 0;

            // Position (World Space)
            matrix.cell[3][0] = pos.x;
            matrix.cell[3][1] = pos.y;
            matrix.cell[3][2] = pos.z;
            matrix.cell[3][3] = 1;

            return matrix;
        }
        
        // Transform matrix from localPos to worldPos (+ rotation)
        float3 TransformPoint(const float3& p) const
        {
            return float3
            (
                p.x * cell[0][0] + p.y * cell[1][0] + p.z * cell[2][0] + cell[3][0],
                p.x * cell[0][1] + p.y * cell[1][1] + p.z * cell[2][1] + cell[3][1],
                p.x * cell[0][2] + p.y * cell[1][2] + p.z * cell[2][2] + cell[3][2]
            );
        }
    };
}