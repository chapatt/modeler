#ifndef MODELER_MATRIX_UTILS_H
#define MODELER_MATRIX_UTILS_H

#define mat4N 4

void perspectiveProjection(float mat[mat4N * mat4N], float fovy, float aspectRatio, float near, float far);
void transformTranslation(float mat[mat4N * mat4N], float x, float y, float z);
void transformRotation(float mat[mat4N * mat4N], float rotation, float x, float y, float z);
void transformRotateZ(float mat[mat4N * mat4N], float rotation);
void transformScale(float mat[mat4N * mat4N], float scale);
void mat4Transpose(float mat[mat4N * mat4N], float dest[4 * 4]);
void mat4Scale(float mat[mat4N * mat4N], float scale);
void mat4Inverse(float mat[mat4N * mat4N], float dest[4 * 4]);
void mat4Copy(float mat[mat4N * mat4N], float dest[4 * 4]);
void mat4Multiply(float mat1[4 * mat4N + 4], float mat2[4 * mat4N + 4], float dest[4 * mat4N + 4]);

#endif /* MODELER_MATRIX_UTILS_H */