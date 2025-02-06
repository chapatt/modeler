#ifndef MODELER_MATRIX_UTILS_H
#define MODELER_MATRIX_UTILS_H

#define mat4N 4
#define mat3N 3
#define mat2N 2

void perspectiveProjection(float mat[mat4N * mat4N], float fovy, float aspectRatio, float near, float far);
void transformTranslation(float mat[mat4N * mat4N], float x, float y, float z);
void transformRotation(float mat[mat4N * mat4N], float rotation, float x, float y, float z);
void transformRotateZ(float mat[mat4N * mat4N], float rotation);
void transformScale(float mat[mat4N * mat4N], float scale);
void mat4Transpose(float mat[mat4N * mat4N], float dest[mat4N * mat4N]);
void mat4Scale(float mat[mat4N * mat4N], float scale);
void mat4Inverse(float mat[mat4N * mat4N], float dest[mat4N * mat4N]);
void mat4Copy(float mat[mat4N * mat4N], float dest[mat4N * mat4N]);
void vec4Copy(float vec[mat4N], float dest[mat4N]);
void mat4Multiply(float m1[mat4N * mat4N], float m2[mat4N * mat4N], float dest[mat4N * mat4N]);
void mat4Vec4Multiply(float mat[mat4N * mat4N], float vec[mat4N], float dest[mat4N]);
void vec4ScalarDivide(float scalar, float vec[mat4N]);
void vec3Add(float v1[mat3N], float v2[mat3N], float dest[mat3N]);
void vec3ScalarMultiply(float scalar, float vec[mat3N]);
void vec3Subtract(float v1[mat3N], float v2[mat3N], float dest[mat3N]);
void normalize(float vec[mat3N]);
float dot(float v1[mat3N], float v2[mat3N]);
void screenToWorld(float homogenous[mat3N], float world[mat3N], float inverseViewProjection[mat4N * mat4N]);
void castScreenToPlane(float intersection[mat3N], float screen[mat2N], float planePoint[mat3N], float planeNormal[mat3N], float inverseViewProjection[mat4N * mat4N]);

#endif /* MODELER_MATRIX_UTILS_H */