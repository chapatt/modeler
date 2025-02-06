#include <stdio.h>
#include <stddef.h>
#include <math.h>

#include "matrix_utils.h"

void perspectiveProjection(float mat[mat4N * mat4N], float fovy, float aspectRatio, float near, float far)
{
	const float e = 1.0f / tan(fovy * 0.5f);
	float newMatrix[] = {
		e / aspectRatio, 0.0f, 0.0f, 0.0f,
		0.0f, e, 0.0f, 0.0f,
		0.0f, 0.0f, far / (far - near), 1.0f,
		0.0f, 0.0f, (far * near) / (near - far), 0.0f
	};

	mat4Copy(newMatrix, mat);
}

void transformTranslation(float mat[mat4N * mat4N], float x, float y, float z)
{
	float newMatrix[] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		x, y, z, 1
	};
	mat4Copy(newMatrix, mat);
}

void transformRotation(float mat[mat4N * mat4N], float rotation, float x, float y, float z)
{
	float newMatrix[] = {
		cos(rotation) + pow(x, 2) * (1 - cos(rotation)), y * x * (1 - cos(rotation)) + z * sin(rotation), z * x * (1 - cos(rotation)) - y * sin(rotation), 0,
		x * y * (1 - cos(rotation)) - z * sin(rotation), cos(rotation) + pow(y, 2) * (1 - cos(rotation)), z * y * (1 - cos(rotation)) + x * sin(rotation), 0,
		x * z * (1 - cos(rotation)) + y * sin(rotation), y * z * (1 - cos(rotation)) - x * sin(rotation), cos(rotation) + pow(z, 2) * (1 - cos(rotation)), 0,
		0, 0, 0, 1
	};
	mat4Copy(newMatrix, mat);
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

void vec4Copy(float vec[mat4N], float dest[mat4N])
{
	for (size_t i = 0; i < mat4N; ++i) {
		dest[i] = vec[i];
	}
}

void vec3Copy(float vec[mat3N], float dest[mat3N])
{
	for (size_t i = 0; i < mat3N; ++i) {
		dest[i] = vec[i];
	}
}

void mat4Multiply(float m1[mat4N * mat4N], float m2[mat4N * mat4N], float dest[mat4N * mat4N])
{
	float a00 = m1[0 * mat4N + 0];
	float a01 = m1[0 * mat4N + 1];
	float a02 = m1[0 * mat4N + 2];
	float a03 = m1[0 * mat4N + 3];
	float a10 = m1[1 * mat4N + 0];
	float a11 = m1[1 * mat4N + 1];
	float a12 = m1[1 * mat4N + 2];
	float a13 = m1[1 * mat4N + 3];
	float a20 = m1[2 * mat4N + 0];
	float a21 = m1[2 * mat4N + 1];
	float a22 = m1[2 * mat4N + 2];
	float a23 = m1[2 * mat4N + 3];
	float a30 = m1[3 * mat4N + 0];
	float a31 = m1[3 * mat4N + 1];
	float a32 = m1[3 * mat4N + 2];
	float a33 = m1[3 * mat4N + 3];
	float b00 = m2[0 * mat4N + 0];
	float b01 = m2[0 * mat4N + 1];
	float b02 = m2[0 * mat4N + 2];
	float b03 = m2[0 * mat4N + 3];
	float b10 = m2[1 * mat4N + 0];
	float b11 = m2[1 * mat4N + 1];
	float b12 = m2[1 * mat4N + 2];
	float b13 = m2[1 * mat4N + 3];
	float b20 = m2[2 * mat4N + 0];
	float b21 = m2[2 * mat4N + 1];
	float b22 = m2[2 * mat4N + 2];
	float b23 = m2[2 * mat4N + 3];
	float b30 = m2[3 * mat4N + 0];
	float b31 = m2[3 * mat4N + 1];
	float b32 = m2[3 * mat4N + 2];
	float b33 = m2[3 * mat4N + 3];

	dest[0 * mat4N + 0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
	dest[0 * mat4N + 1] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
	dest[0 * mat4N + 2] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
	dest[0 * mat4N + 3] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
	dest[1 * mat4N + 0] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
	dest[1 * mat4N + 1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
	dest[1 * mat4N + 2] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
	dest[1 * mat4N + 3] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
	dest[2 * mat4N + 0] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
	dest[2 * mat4N + 1] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
	dest[2 * mat4N + 2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
	dest[2 * mat4N + 3] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
	dest[3 * mat4N + 0] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
	dest[3 * mat4N + 1] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
	dest[3 * mat4N + 2] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
	dest[3 * mat4N + 3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
 }

void mat4Vec4Multiply(float mat[mat4N * mat4N], float vec[mat4N], float dest[mat4N])
{
	dest[0] = mat[0 * mat4N + 0] * vec[0] + mat[1 * mat4N + 0] * vec[1] + mat[2 * mat4N + 0] * vec[2] + mat[3 * mat4N + 0] * vec[3];
	dest[1] = mat[0 * mat4N + 1] * vec[0] + mat[1 * mat4N + 1] * vec[1] + mat[2 * mat4N + 1] * vec[2] + mat[3 * mat4N + 1] * vec[3];
	dest[2] = mat[0 * mat4N + 2] * vec[0] + mat[1 * mat4N + 2] * vec[1] + mat[2 * mat4N + 2] * vec[2] + mat[3 * mat4N + 2] * vec[3];
	dest[3] = mat[0 * mat4N + 3] * vec[0] + mat[1 * mat4N + 3] * vec[1] + mat[2 * mat4N + 3] * vec[2] + mat[3 * mat4N + 3] * vec[3];
}

void vec4ScalarDivide(float scalar, float vec[mat4N])
{
	for (size_t i = 0; i < mat4N; ++i) {
		vec[i] = vec[i] / scalar;
	}
}

void vec3ScalarMultiply(float scalar, float vec[mat3N])
{
	for (size_t i = 0; i < mat3N; ++i) {
		vec[i] = vec[i] * scalar;
	}
}

void vec3Subtract(float v1[mat3N], float v2[mat3N], float dest[mat3N])
{
	for (size_t i = 0; i < mat3N; ++i) {
		dest[i] = v1[i] - v2[i];
	}
}

void vec3Add(float v1[mat3N], float v2[mat3N], float dest[mat3N])
{
	for (size_t i = 0; i < mat3N; ++i) {
		dest[i] = v1[i] + v2[i];
	}
}

void normalize(float vec[mat3N])
{
	float length = sqrt(dot(vec, vec));
	float newVector[] = {
		vec[0] / length,
		vec[1] / length,
		vec[2] / length
	};

	vec3Copy(newVector, vec);
}

float dot(float v1[mat3N], float v2[mat3N])
{
	return (v1[0] * v2[0]) + (v1[1] * v2[1]) + (v1[2] * v2[2]);
}

void screenToWorld(float homogenous[mat3N], float world[mat3N], float inverseViewProjection[mat4N * mat4N])
{
	float homogenousVec4[mat4N] = {homogenous[0], homogenous[1], homogenous[2], 1.0f};
	float worldVec4[mat4N];
	mat4Vec4Multiply(inverseViewProjection, homogenousVec4, worldVec4);
	vec4ScalarDivide(worldVec4[3], worldVec4);
	world[0] = worldVec4[0];
	world[1] = worldVec4[1];
	world[2] = worldVec4[2];
}

void castScreenToPlane(float intersection[mat3N], float screen[mat2N], float planePoint[mat3N], float planeNormal[mat3N], float inverseViewProjection[mat4N * mat4N])
{
	float rayOrigin[mat4N];
	float rayEnd[mat4N];
	float rayNormal[mat3N];
	float homogenousOrigin[mat3N] = {screen[0], screen[1], 0.0f};
	float homogenousEnd[mat3N] = {screen[0], screen[1], 1.0f};
	float rayToPlane[mat3N];
	float output[mat3N];
	screenToWorld(homogenousOrigin, rayOrigin, inverseViewProjection);
	screenToWorld(homogenousEnd, rayEnd, inverseViewProjection);
	vec3Subtract(rayEnd, rayOrigin, rayNormal);
	normalize(rayNormal);
	vec3Subtract(planePoint, rayOrigin, rayToPlane);
	float t = dot(rayToPlane, planeNormal) / dot(rayNormal, planeNormal);
	vec3ScalarMultiply(t, rayNormal);
	vec3Add(rayOrigin, rayNormal, intersection);
	printf("on plane: %f\n", t);
}

