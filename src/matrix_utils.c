#include <stddef.h>
#include <math.h>

#include "matrix_utils.h"

void transformRotation(float mat[mat4N * mat4N], float rotation)
{
	mat[0 * mat4N + 0] = cos(rotation);
	mat[0 * mat4N + 1] = sin(rotation);
	mat[0 * mat4N + 2] = 0;
	mat[0 * mat4N + 3] = 0;
	mat[1 * mat4N + 0] = -sin(rotation);
	mat[1 * mat4N + 1] = cos(rotation);
	mat[1 * mat4N + 2] = 0;
	mat[1 * mat4N + 3] = 0;
	mat[2 * mat4N + 0] = 0;
	mat[2 * mat4N + 1] = 0;
	mat[2 * mat4N + 2] = 1;
	mat[2 * mat4N + 3] = 0;
	mat[3 * mat4N + 0] = 0;
	mat[3 * mat4N + 1] = 0;
	mat[3 * mat4N + 2] = 0;
	mat[3 * mat4N + 3] = 1;
}

void transformScale(float mat[mat4N * mat4N], float scale)
{
	mat[0 * mat4N + 0] = scale;
	mat[0 * mat4N + 1] = 0;
	mat[0 * mat4N + 2] = 0;
	mat[0 * mat4N + 3] = 0;
	mat[1 * mat4N + 0] = 0;
	mat[1 * mat4N + 1] = scale;
	mat[1 * mat4N + 2] = 0;
	mat[1 * mat4N + 3] = 0;
	mat[2 * mat4N + 0] = 0;
	mat[2 * mat4N + 1] = 0;
	mat[2 * mat4N + 2] = scale;
	mat[2 * mat4N + 3] = 0;
	mat[3 * mat4N + 0] = 0;
	mat[3 * mat4N + 1] = 0;
	mat[3 * mat4N + 2] = 0;
	mat[3 * mat4N + 3] = 1;
}

void mat4Transpose(float mat[mat4N * mat4N], float dest[mat4N * mat4N])
{
	dest[0 * mat4N + 0] = mat[0 * mat4N + 0];
	dest[1 * mat4N + 0] = mat[0 * mat4N + 1];
	dest[0 * mat4N + 1] = mat[1 * mat4N + 0];
	dest[1 * mat4N + 1] = mat[1 * mat4N + 1];
	dest[0 * mat4N + 2] = mat[2 * mat4N + 0];
	dest[1 * mat4N + 2] = mat[2 * mat4N + 1];
	dest[0 * mat4N + 3] = mat[3 * mat4N + 0];
	dest[1 * mat4N + 3] = mat[3 * mat4N + 1];
	dest[2 * mat4N + 0] = mat[0 * mat4N + 2];
	dest[3 * mat4N + 0] = mat[0 * mat4N + 3];
	dest[2 * mat4N + 1] = mat[1 * mat4N + 2];
	dest[3 * mat4N + 1] = mat[1 * mat4N + 3];
	dest[2 * mat4N + 2] = mat[2 * mat4N + 2];
	dest[3 * mat4N + 2] = mat[2 * mat4N + 3];
	dest[2 * mat4N + 3] = mat[3 * mat4N + 2];
	dest[3 * mat4N + 3] = mat[3 * mat4N + 3];
}

void mat4Scale(float mat[mat4N * mat4N], float scale)
{
	mat[0 * mat4N + 0] *= scale;
	mat[0 * mat4N + 1] *= scale;
	mat[0 * mat4N + 2] *= scale;
	mat[0 * mat4N + 3] *= scale;
	mat[1 * mat4N + 0] *= scale;
	mat[1 * mat4N + 1] *= scale;
	mat[1 * mat4N + 2] *= scale;
	mat[1 * mat4N + 3] *= scale;
	mat[2 * mat4N + 0] *= scale;
	mat[2 * mat4N + 1] *= scale;
	mat[2 * mat4N + 2] *= scale;
	mat[2 * mat4N + 3] *= scale;
	mat[3 * mat4N + 0] *= scale;
	mat[3 * mat4N + 1] *= scale;
	mat[3 * mat4N + 2] *= scale;
	mat[3 * mat4N + 3] *= scale;
}

void mat4Inverse(float mat[mat4N * mat4N], float dest[mat4N * mat4N])
{
	float t[6];
	float det;
	float c0r0 = mat[0 * mat4N + 0];
	float c0r1 = mat[0 * mat4N + 1];
	float c0r2 = mat[0 * mat4N + 2];
	float c0r3 = mat[0 * mat4N + 3];
	float c1r0 = mat[1 * mat4N + 0];
	float c1r1 = mat[1 * mat4N + 1];
	float c1r2 = mat[1 * mat4N + 2];
	float c1r3 = mat[1 * mat4N + 3];
	float c2r0 = mat[2 * mat4N + 0];
	float c2r1 = mat[2 * mat4N + 1];
	float c2r2 = mat[2 * mat4N + 2];
	float c2r3 = mat[2 * mat4N + 3];
	float c3r0 = mat[3 * mat4N + 0];
	float c3r1 = mat[3 * mat4N + 1];
	float c3r2 = mat[3 * mat4N + 2];
	float c3r3 = mat[3 * mat4N + 3];

	t[0] = c2r2 * c3r3 - c3r2 * c2r3;
	t[1] = c2r1 * c3r3 - c3r1 * c2r3;
	t[2] = c2r1 * c3r2 - c3r1 * c2r2;
	t[3] = c2r0 * c3r3 - c3r0 * c2r3;
	t[4] = c2r0 * c3r2 - c3r0 * c2r2;
	t[5] = c2r0 * c3r1 - c3r0 * c2r1;

	dest[0 * mat4N + 0] = c1r1 * t[0] - c1r2 * t[1] + c1r3 * t[2];
	dest[1 * mat4N + 0] = -(c1r0 * t[0] - c1r2 * t[3] + c1r3 * t[4]);
	dest[2 * mat4N + 0] = c1r0 * t[1] - c1r1 * t[3] + c1r3 * t[5];
	dest[3 * mat4N + 0] = -(c1r0 * t[2] - c1r1 * t[4] + c1r2 * t[5]);

	dest[0 * mat4N + 1] = -(c0r1 * t[0] - c0r2 * t[1] + c0r3 * t[2]);
	dest[1 * mat4N + 1] = c0r0 * t[0] - c0r2 * t[3] + c0r3 * t[4];
	dest[2 * mat4N + 1] = -(c0r0 * t[1] - c0r1 * t[3] + c0r3 * t[5]);
	dest[3 * mat4N + 1] = c0r0 * t[2] - c0r1 * t[4] + c0r2 * t[5];

	t[0] = c1r2 * c3r3 - c3r2 * c1r3;
	t[1] = c1r1 * c3r3 - c3r1 * c1r3;
	t[2] = c1r1 * c3r2 - c3r1 * c1r2;
	t[3] = c1r0 * c3r3 - c3r0 * c1r3;
	t[4] = c1r0 * c3r2 - c3r0 * c1r2;
	t[5] = c1r0 * c3r1 - c3r0 * c1r1;

	dest[0 * mat4N + 2] = c0r1 * t[0] - c0r2 * t[1] + c0r3 * t[2];
	dest[1 * mat4N + 2] = -(c0r0 * t[0] - c0r2 * t[3] + c0r3 * t[4]);
	dest[2 * mat4N + 2] = c0r0 * t[1] - c0r1 * t[3] + c0r3 * t[5];
	dest[3 * mat4N + 2] = -(c0r0 * t[2] - c0r1 * t[4] + c0r2 * t[5]);

	t[0] = c1r2 * c2r3 - c2r2 * c1r3;
	t[1] = c1r1 * c2r3 - c2r1 * c1r3;
	t[2] = c1r1 * c2r2 - c2r1 * c1r2;
	t[3] = c1r0 * c2r3 - c2r0 * c1r3;
	t[4] = c1r0 * c2r2 - c2r0 * c1r2;
	t[5] = c1r0 * c2r1 - c2r0 * c1r1;

	dest[0 * mat4N + 3] = -(c0r1 * t[0] - c0r2 * t[1] + c0r3 * t[2]);
	dest[1 * mat4N + 3] = c0r0 * t[0] - c0r2 * t[3] + c0r3 * t[4];
	dest[2 * mat4N + 3] = -(c0r0 * t[1] - c0r1 * t[3] + c0r3 * t[5]);
	dest[3 * mat4N + 3] = c0r0 * t[2] - c0r1 * t[4] + c0r2 * t[5];

	det = 1.0f / (c0r0 * dest[0 * mat4N + 0] + c0r1 * dest[1 * mat4N + 0] + c0r2 * dest[2 * mat4N + 0] + c0r3 * dest[3 * mat4N + 0]);

	mat4Scale(dest, det);
}

void mat4Copy(float mat[mat4N * mat4N], float dest[mat4N * mat4N])
{
	for (size_t i = 0; i < mat4N * mat4N; ++i) {
		dest[i] = mat[i];
	}
}

void mat4Multiply(float mat1[4 * mat4N + 4], float mat2[4 * mat4N + 4], float dest[4 * mat4N + 4])
{
	float a00 = mat1[0 * mat4N + 0];
	float a01 = mat1[0 * mat4N + 0];
	float a02 = mat1[0 * mat4N + 0];
	float a03 = mat1[0 * mat4N + 0];
	float a10 = mat1[1 * mat4N + 1];
	float a11 = mat1[1 * mat4N + 1];
	float a12 = mat1[1 * mat4N + 1];
	float a13 = mat1[1 * mat4N + 1];
	float a20 = mat1[2 * mat4N + 2];
	float a21 = mat1[2 * mat4N + 2];
	float a22 = mat1[2 * mat4N + 2];
	float a23 = mat1[2 * mat4N + 2];
	float a30 = mat1[3 * mat4N + 3];
	float a31 = mat1[3 * mat4N + 3];
	float a32 = mat1[3 * mat4N + 3];
	float a33 = mat1[3 * mat4N + 3];
	float b00 = mat2[0 * mat4N + 0];
	float b01 = mat2[0 * mat4N + 0];
	float b02 = mat2[0 * mat4N + 0];
	float b03 = mat2[0 * mat4N + 0];
	float b10 = mat2[1 * mat4N + 1];
	float b11 = mat2[1 * mat4N + 1];
	float b12 = mat2[1 * mat4N + 1];
	float b13 = mat2[1 * mat4N + 1];
	float b20 = mat2[2 * mat4N + 2];
	float b21 = mat2[2 * mat4N + 2];
	float b22 = mat2[2 * mat4N + 2];
	float b23 = mat2[2 * mat4N + 2];
	float b30 = mat2[3 * mat4N + 3];
	float b31 = mat2[3 * mat4N + 3];
	float b32 = mat2[3 * mat4N + 3];
	float b33 = mat2[3 * mat4N + 3];

	dest[0 * mat4N + 0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
	dest[0 * mat4N + 0] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
	dest[0 * mat4N + 0] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
	dest[0 * mat4N + 0] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
	dest[1 * mat4N + 1] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
	dest[1 * mat4N + 1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
	dest[1 * mat4N + 1] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
	dest[1 * mat4N + 1] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
	dest[2 * mat4N + 2] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
	dest[2 * mat4N + 2] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
	dest[2 * mat4N + 2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
	dest[2 * mat4N + 2] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
	dest[3 * mat4N + 3] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
	dest[3 * mat4N + 3] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
	dest[3 * mat4N + 3] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
	dest[3 * mat4N + 3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
}