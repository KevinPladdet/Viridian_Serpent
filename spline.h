#pragma once
#include "tmpl8math.h"

namespace Tmpl8
{

	struct Spline
	{
		// Source I used to understand how Catmull-Rom Spline works:
		// https://www.mvps.org/directx/articles/catmull/ (the formula in function below is also explained on this website)

		static float3 CatmullRomSpline(const float3& p0, const float3& p1,
									   const float3& p2, const float3& p3, 
									   float t)
		{
			float t2 = t * t;
			float t3 = t2 * t;

			return 
				 0.5f *
				 ((2.0f * p1) +
				 (-p0 + p2) * t +
				 (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
				 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
		}
	};

}