#ifndef REF_MATRIX_H
#define REF_MATRIX_H

struct ref_box {
    int x, y;
    int width, height;
};

enum ref_output_transform {
    REF_TRANSFORM_NORMAL = 0,
    REF_TRANSFORM_90 = 1,
    REF_TRANSFORM_180 = 2,
    REF_TRANSFORM_270 = 3,
    REF_TRANSFORM_FLIPPED = 4,
    REF_TRANSFORM_FLIPPED_90 = 5,
    REF_TRANSFORM_FLIPPED_180 = 6,
    REF_TRANSFORM_FLIPPED_270 = 7,
};

void ref_matrix_identity(float mat[9]);
void ref_matrix_multiply(float mat[9], const float a[9], const float b[9]);
void ref_matrix_transpose(float mat[9], const float a[9]);
void ref_matrix_translate(float mat[9], float x, float y);
void ref_matrix_scale(float mat[9], float x, float y);
void ref_matrix_rotate(float mat[9], float rad);
void ref_matrix_transform(float mat[9], enum ref_output_transform transform);
void ref_matrix_projection(float mat[9], int width, int height, enum ref_output_transform transform);
void ref_matrix_project_box(float mat[9], const struct ref_box* box, enum ref_output_transform transform, float rotation, const float projection[9]);

#endif
