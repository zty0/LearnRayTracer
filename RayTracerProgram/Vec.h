#pragma once

struct Vec4
{
	union 
	{
		struct { float x, y, z, w; };
		float data[4];
	};

	float operator[](int index) { return data[index]; }
	Vec4 operator+(Vec4 vec) {

	}
};