#include <math.h>
#include <string.h>
#include <stdio.h>

#include "ref_matrix.h"

void ref_matrix_identity(float m[static 9])
{
    static const float i[9] = {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
    memcpy(m, i, sizeof(i));
}

void ref_matrix_multiply(float m[static 9], const float a[static 9], const float b[static 9])
{
    float p[9];

    p[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    p[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    p[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];

    p[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    p[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    p[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];

    p[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    p[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    p[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];

    memcpy(m, p, sizeof(p));
}

void ref_matrix_transpose(float m[static 9], const float a[static 9])
{
    float transposition[9] = {
        a[0],
        a[3],
        a[6],
        a[1],
        a[4],
        a[7],
        a[2],
        a[5],
        a[8],
    };
    memcpy(m, transposition, sizeof(transposition));
}

void ref_matrix_translate(float m[static 9], float x, float y)
{
    float t[9] = {
        1.0f,
        0.0f,
        x,
        0.0f,
        1.0f,
        y,
        0.0f,
        0.0f,
        1.0f,
    };

    ref_matrix_multiply(m, m, t);
}

void ref_matrix_scale(float m[static 9], float x, float y)
{
    float s[9] = {
        x,
        0.0f,
        0.0f,
        0.0f,
        y,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
    ref_matrix_multiply(m, m, s);
}

void ref_matrix_rotate(float m[static 9], float rad)
{
    float r[9] = {
        cos(rad),
        -sin(rad),
        0.0f,
        sin(rad),
        cos(rad),
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
    ref_matrix_multiply(m, m, r);
}

static const float transforms[][9] = {
    [REF_TRANSFORM_NORMAL] = {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_90] = {
        0.0f,
        1.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_180] = {
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_270] = {
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_FLIPPED] = {
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_FLIPPED_90] = {
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_FLIPPED_180] = {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    [REF_TRANSFORM_FLIPPED_270] = {
        0.0f,
        -1.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    },
};

void ref_matrix_transform(float m[static 9], enum ref_output_transform t)
{
    ref_matrix_multiply(m, m, transforms[t]);
}

void ref_matrix_projection(float m[static 9], int w, int h, enum ref_output_transform t)
{
    memset(m, 0, sizeof(*m) * 9);

    const float* tr = transforms[t];
    float x = 2.0f / w;
    float y = 2.0f / h;

    // Rotation + reflection
    m[0] = x * tr[0];
    m[1] = x * tr[1];
    m[3] = y * -tr[3];
    m[4] = y * -tr[4];

    // Translation
    m[2] = -copysign(1.0f, m[0] + m[1]);
    m[5] = -copysign(1.0f, m[3] + m[4]);

    // Identity
    m[8] = 1.0f;
}

void ref_matrix_project_box(float m[static 9], const struct ref_box* b, enum ref_output_transform t, float r, const float p[static 9])
{
    int x = b->x;
    int y = b->y;
    int w = b->width;
    int h = b->height;

    ref_matrix_identity(m);
    ref_matrix_translate(m, x, y);

    if (r != 0) {
        ref_matrix_translate(m, w / 2, h / 2);
        ref_matrix_rotate(m, r);
        ref_matrix_translate(m, -w / 2, -h / 2);
    }

    ref_matrix_scale(m, w, h);

    if (t != REF_TRANSFORM_NORMAL) {
        ref_matrix_translate(m, 0.5, 0.5);
        ref_matrix_transform(m, t);
        ref_matrix_translate(m, -0.5, -0.5);
    }

    ref_matrix_multiply(m, p, m);
}
