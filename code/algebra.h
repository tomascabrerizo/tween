#ifndef _MATH_H_
#define _MATH_H_

#include "common.h"
#include <math.h>

static inline f32 to_rad(f32 degree) {
    return (degree / 180.0f) * M_PI;
}

static inline f32 lerp(f32 a, f32 b, f32 t) {
    return a*(1-t) + b*t;
}

typedef struct V2 {
    float x;
    float y;
} V2;

static inline V2 v2(float x, float y) {
    return (V2){x, y};
}

typedef struct V3 {
    float x;
    float y;
    float z;
} V3;

static inline V3 v3(float x, float y,  float z) {
    return (V3){x, y, z};
}

static inline V3 v32_lerp(V3 a, V3 b, f32 t) {
    V3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

static inline V3 v3_add(V3 a, V3 b) {
    return (V3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline V3 v3_sub(V3 a, V3 b) {
    return (V3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline V3 v3_neg(V3 a) {
    return (V3){-a.x, -a.y, -a.z};
}

static inline V3 v3_scale(V3 a, float s) {
    return (V3){a.x*s, a.y*s, a.z*s};
}

static inline float v3_dot(V3 a, V3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline V3 v3_cross(V3 a, V3 b) {
    return (V3){a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x};
}

static inline float v3_length_sqr(V3 a) {
    return v3_dot(a, a);
}

static inline float v3_length(V3 a) {
    return sqrtf(v3_length_sqr(a));
}

static inline V3 v3_normalize(V3 a) {
    float len = v3_length(a);
    if(len != 0) {
        float inv_len = 1.0f / len;
        a = v3_scale(a, inv_len);
    }
    return a;
}

typedef struct M4 {
    float m[16];
} M4;

static inline M4 m4_identity(void) {
    M4 m = (M4){{1, 0, 0, 0, 
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 0, 0, 0, 1}};
    return m; 
}

static inline M4 m4_translate(V3 t) {
    M4 m = (M4){{1, 0, 0, t.x, 
                 0, 1, 0, t.y,
                 0, 0, 1, t.z,
                 0, 0, 0, 1}};
    return m; 
}

static inline M4 m4_scale_v3(V3 s) {
    M4 m = (M4){{s.x, 0, 0, 0, 
                 0, s.y, 0, 0,
                 0, 0, s.z, 0,
                 0, 0, 0, 1}};
    return m; 
}

static inline M4 m4_scale(float s) {
    M4 m = (M4){{s, 0, 0, 0, 
                 0, s, 0, 0,
                 0, 0, s, 0,
                 0, 0, 0, 1}};
    return m; 
}

static inline M4 m4_rotate_x(float r) {
    float s = sinf(r);
    float c = cosf(r);
    M4 m = {{
        1,  0,  0,  0,
        0,  c, -s,  0,
        0,  s,  c,  0,
        0,  0,  0,  1}};
    return m;
}

static inline M4 m4_rotate_y(float r) {
    float s = sinf(r);
    float c = cosf(r);
    M4 m = {{
        c,  0,  s,  0,
        0,  1,  0,  0,
       -s,  0,  c,  0,
        0,  0,  0,  1}};
    return m;
}

static inline M4 m4_rotate_z(float r) {
    float s = sinf(r);
    float c = cosf(r);
    M4 m = {{
         c, -s,  0,  0,
         s,  c,  0,  0,
         0,  0,  1,  0,
         0,  0,  0,  1}};
    return m;
}

static inline M4 m4_ortho(float l, float r, float t, float b, float n, float f) {
    float rml = 1.0f / (r - l);
    float tmb = 1.0f / (t - b);
    float fmn = 1.0f / (f - n);
    M4 m = (M4){{2*rml, 0    , 0  , -((r+l)*rml), 
                 0    , 2*tmb, 0  , -((t+b)*tmb),
                 0    , 0    , fmn, -(n*fmn)    ,
                 0    , 0    , 0  , 1           }};
    return m; 
}

static inline M4 m4_perspective(float l, float r, float t, float b, float n, float f) {
    float _2n = 2 * n;
    float rml = 1.0f / (r - l);
    float rpl = r + l;
    float tmb = 1.0f / (t - b);
    float tpb = t + b;
    float fmn = 1.0f / (f - n);
    float fpn = f + n;
    M4 m = (M4){{_2n*rml,  0     , rpl*rml   ,  0          , 
                  0     , _2n*tmb, tpb*tmb   ,  0          ,
                  0     ,  0     , -(fpn/fmn), -(_2n*f*fmn),
                  0     ,  0     , -1        ,  0          }};
    return m; 
}

static inline M4 m4_perspective2(float fov, float aspect, float n, float f) {
    float c = 1.0f / tanf(fov/2);
    float fmn = f - n;
    float fpn = f + n;
    M4 result = {{
        c/aspect, 0,               0,   0,
        0,        c,               0,   0,
        0,        0,    -(fpn / fmn),   -((2*n*f) / fmn),
        0,        0,              -1,   0}};
    return result;
}

static inline M4 m4_lookat(V3 r, V3 u, V3 v, V3 t) {
    V3 neg_t = v3_neg(t);
    M4 m = (M4){{r.x, r.y, r.z, v3_dot(neg_t, r), 
                 u.x, u.y, u.z, v3_dot(neg_t, u),
                 v.x, v.y, v.z, v3_dot(neg_t, v),
                 0, 0, 0,                     1}};
    return m; 
}

static inline M4 m4_lookat2(V3 pos, V3 target, V3 up) {
    /* NOTE: Camera basis verctor */
    V3 dir = v3_sub(target, pos);
    V3 v = v3_normalize(v3_neg(dir));
    V3 u = v3_sub(up, v3_scale(v, v3_dot(up, v)));
    V3 r = v3_cross(u, v);
    return m4_lookat(r, u, v, pos);
}

static inline M4 m4_mul(M4 a, M4 b) {
#define RTC(r, c) a.m[(r<<2)]*b.m[c] + a.m[(r<<2)+1]*b.m[c+4] + a.m[(r<<2)+2]*b.m[c+8] + a.m[(r<<2)+3]*b.m[c+12]
    M4 m = {{
        RTC(0, 0), RTC(0, 1), RTC(0, 2), RTC(0, 3), 
        RTC(1, 0), RTC(1, 1), RTC(1, 2), RTC(1, 3), 
        RTC(2, 0), RTC(2, 1), RTC(2, 2), RTC(2, 3), 
        RTC(3, 0), RTC(3, 1), RTC(3, 2), RTC(3, 3)}};
#undef RTC
  return m;
}

static inline M4 m4_transpose(M4 a) {
    M4 result;
    for(u32 i = 0; i < 4; ++i) {
        for(u32 j = 0; j < 4; ++j) {
            result.m[(i<<2)+j] = a.m[(j<<2)+i];
        }
    }
    return result;
}

static inline M4 m4_affine_transform_inverse(M4 a) {
    
    M4 inv_translation = m4_identity();
    
    inv_translation.m[3] = -a.m[3];
    inv_translation.m[7] = -a.m[7];
    inv_translation.m[11] = -a.m[11];
    
    /* TODO: Is this inv_scale as well??? */
    M4 inv_rotation = m4_identity();
    
    inv_rotation.m[0] = a.m[0];
    inv_rotation.m[1] = a.m[1];
    inv_rotation.m[2] = a.m[2];

    inv_rotation.m[4] = a.m[4];
    inv_rotation.m[5] = a.m[5];
    inv_rotation.m[6] = a.m[6];

    inv_rotation.m[8] = a.m[8];
    inv_rotation.m[9] = a.m[9];
    inv_rotation.m[10] = a.m[10];
    
    inv_rotation = m4_transpose(inv_rotation);
    
    M4 result = m4_mul(inv_rotation, inv_translation);

    return result;
}

/* TODO: remove stdio.h from this file */
#include <stdio.h>

static inline void m4_print(M4 m) {
    for(int j = 0; j < 4; ++j) {
        printf("%.2f, %.2f, %.2f, %.2f\n", 
                m.m[(j<<2)+0], m.m[(j<<2)+1], m.m[(j<<2)+2], m.m[(j<<2)+3]);
    }
}

typedef struct Q4 {
    f32 w, x, y, z;
} Q4;

static inline Q4 q4(f32 w, f32 x, f32 y, f32 z) {
    return (Q4){w, x, y, z};
}

static inline Q4 q4_slerp(Q4 a, Q4 b, f32 t) {

    f32 k0, k1;

    f32 cos_omega = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if(cos_omega < 0.0f) {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        cos_omega = -cos_omega;
    }
    
    if(cos_omega > 0.9999f) {
        k0 = (1 - t);
        k1 = t;
    } else {
        
        f32 sin_omega = sqrtf(1.0f - cos_omega*cos_omega);
        f32 omega = atan2(sin_omega, cos_omega);
        f32 one_over_sin_omega = 1.0f / sin_omega;
        k0 = sin((1.0f - t) * omega) * one_over_sin_omega;
        k1 = sin(t * omega) * one_over_sin_omega;
    }

    Q4 result;

    result.w = a.w*k0 + b.w*k1;
    result.x = a.x*k0 + b.x*k1;
    result.y = a.y*k0 + b.y*k1;
    result.z = a.z*k0 + b.z*k1;

    return result;
}

static inline Q4 q4_add(Q4 a, Q4 b) {
    Q4 result;
    result.w = a.w + b.w;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static inline Q4 q4_scale(Q4 a, f32 s) {
    Q4 result;
    result.w = a.w * s;
    result.x = a.x * s;
    result.y = a.y * s;
    result.z = a.z * s;
    return result;
}

static inline Q4 q4_normalize(Q4 q) {
    Q4 result = q;

    f32 inv_len = 1.0f / sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
    result.w *= inv_len; 
    result.x *= inv_len; 
    result.y *= inv_len; 
    result.z *= inv_len; 

    return result;
}

static inline M4 q4_to_m4(Q4 q) {
    
    M4 result;
    
    f32 w = q.w;
    f32 x = q.x;
    f32 y = q.y;
    f32 z = q.z;

    result.m[0] = 1 - 2*y*y - 2*z*z;
    result.m[1] = 2 * (x*y - w*z);
    result.m[2] = 2 * (x*z + w*y);
    result.m[3] = 0;

    result.m[4] = 2 * (x*y + w*z);
    result.m[5] = 1 - 2*x*x - 2*z*z;
    result.m[6] = 2 * (y*z - w*x);
    result.m[7] = 0;

    result.m[8]  = 2 * (x*z - w*y);
    result.m[9]  = 2 * (y*z + w*x);
    result.m[10] = 1 - 2*x*x - 2*y*y;
    result.m[11] = 0;

    result.m[12] = 0;
    result.m[13] = 0;
    result.m[14] = 0;
    result.m[15] = 1;

    return result;
}

/* M4 helper functions */

static inline V3 m4_get_v3_translation(M4 a) {
    V3 result = v3(a.m[3], a.m[7], a.m[11]);
    return result;
}

static inline Q4 m4_get_q4_rotation(M4 a) {
    
    a = m4_transpose(a);

    f32 m11 = a.m[0];
    f32 m12 = a.m[1];
    f32 m13 = a.m[2];

    f32 m21 = a.m[4];
    f32 m22 = a.m[5];
    f32 m23 = a.m[6];

    f32 m31 = a.m[8];
    f32 m32 = a.m[9];
    f32 m33 = a.m[10];

    f32 four_w_squared_minus1 = m11 + m22 + m33;
    f32 four_x_squared_minus1 = m11 - m22 - m33;
    f32 four_y_squared_minus1 = m22 - m11 - m33;
    f32 four_z_squared_minus1 = m33 - m11 - m22;
    
    s32 biggest_index = 0;
    f32 four_biggest_squared_minus1 = four_w_squared_minus1;

    if(four_x_squared_minus1 > four_biggest_squared_minus1) {
        four_biggest_squared_minus1 = four_x_squared_minus1;
        biggest_index = 1;
    }

    if(four_y_squared_minus1 > four_biggest_squared_minus1) {
        four_biggest_squared_minus1 = four_y_squared_minus1;
        biggest_index = 2;
    }
    
    if(four_z_squared_minus1 > four_biggest_squared_minus1) {
        four_biggest_squared_minus1 = four_z_squared_minus1;
        biggest_index = 3;
    }
    
    f32 biggest_val = sqrtf(four_biggest_squared_minus1 + 1.0f) * 0.5f;
    f32 mult = 0.25f;

    Q4 result = {};
    
    switch (biggest_index) {
        case 0: {
            result.w = biggest_val;
            result.x = (m23 - m32) * mult; 
            result.y = (m31 - m13) * mult;
            result.z = (m12 - m21) * mult;
        } break; 

        case 1: {
            result.x = biggest_val;
            result.w = (m23 - m32) * mult; 
            result.y = (m12 + m21) * mult;
            result.z = (m31 + m13) * mult;
        } break; 

        case 2: {
            result.y = biggest_val;
            result.w = (m31 - m13) * mult; 
            result.x = (m12 + m21) * mult;
            result.z = (m23 + m32) * mult;
        } break; 

        case 3: {
            result.z = biggest_val;
            result.w = (m12 - m21) * mult; 
            result.x = (m31 + m13) * mult;
            result.y = (m23 + m32) * mult;
        } break; 
    }

    return result;
}
static inline Q4 q4_from_m4(M4 a) {
    
    float w, x, y, z;
    
    f32 m00 = a.m[0];
    f32 m01 = a.m[1];
    f32 m02 = a.m[2];

    f32 m10 = a.m[4];
    f32 m11 = a.m[5];
    f32 m12 = a.m[6];

    f32 m20 = a.m[8];
    f32 m21 = a.m[9];
    f32 m22 = a.m[10];


    float diagonal = m00 + m11 + m22;
    if (diagonal > 0) {
        float w4 = (float) (sqrtf(diagonal + 1.0f) * 2.0f);
        w = w4 / 4.0f;
        x = (m21 - m12) / w4;
        y = (m02 - m20) / w4;
        z = (m10 - m01) / w4;
    } else if ((m00 > m11) && (m00 > m22)) {
        float x4 = (float) (sqrtf(1.0f + m00 - m11 - m22) * 2.0f);
        w = (m21 - m12) / x4;
        x = x4 / 4.0f;
        y = (m01 + m10) / x4;
        z = (m02 + m20) / x4;
    } else if (m11 > m22) {
        float y4 = (float) (sqrtf(1.0f + m11 - m00 - m22) * 2.0f);
        w = (m02 - m20) / y4;
        x = (m01 + m10) / y4;
        y = y4 / 4.0f;
        z = (m12 + m21) / y4;
    } else {
        float z4 = (float) (sqrtf(1.0f + m22 - m00 - m11) * 2.0f);
        w = (m10 - m01) / z4;
        x = (m02 + m20) / z4;
        y = (m12 + m21) / z4;
        z = z4 / 4.0f;
    }
    return (Q4){w, x, y, z};
}

#define M4_3X3MINOR(c0, c1, c2, r0, r1, r2) \
    (m.m[(c0 << 2) + r0] * (m.m[(c1 << 2) + r1] * m.m[(c2 << 2) + r2] - m.m[(c1 << 2) + r2] * m.m[(c2 << 2) + r1]) - \
     m.m[(c1 << 2) + r0] * (m.m[(c0 << 2) + r1] * m.m[(c2 << 2) + r2] - m.m[(c0 << 2) + r2] * m.m[(c2 << 2) + r1]) + \
     m.m[(c2 << 2) + r0] * (m.m[(c0 << 2) + r1] * m.m[(c1 << 2) + r2] - m.m[(c0 << 2) + r2] * m.m[(c1 << 2) + r1]))


static inline float m4_determinant(M4 m) {
    return  m.m[0]  * M4_3X3MINOR(1, 2, 3, 1, 2, 3)
		  - m.m[4]  * M4_3X3MINOR(0, 2, 3, 1, 2, 3)
		  + m.m[8]  * M4_3X3MINOR(0, 1, 3, 1, 2, 3)
		  - m.m[12] * M4_3X3MINOR(0, 1, 2, 1, 2, 3);
}

static inline M4 m4_adjugate(M4 m) {
	M4 cofactor;

	cofactor.m[0] =  M4_3X3MINOR(1, 2, 3, 1, 2, 3);
	cofactor.m[1] = -M4_3X3MINOR(1, 2, 3, 0, 2, 3);
	cofactor.m[2] =  M4_3X3MINOR(1, 2, 3, 0, 1, 3);
	cofactor.m[3] = -M4_3X3MINOR(1, 2, 3, 0, 1, 2);

	cofactor.m[4] = -M4_3X3MINOR(0, 2, 3, 1, 2, 3);
	cofactor.m[5] =  M4_3X3MINOR(0, 2, 3, 0, 2, 3);
	cofactor.m[6] = -M4_3X3MINOR(0, 2, 3, 0, 1, 3);
	cofactor.m[7] =  M4_3X3MINOR(0, 2, 3, 0, 1, 2);

	cofactor.m[8] =   M4_3X3MINOR(0, 1, 3, 1, 2, 3);
	cofactor.m[9] =  -M4_3X3MINOR(0, 1, 3, 0, 2, 3);
	cofactor.m[10] =  M4_3X3MINOR(0, 1, 3, 0, 1, 3);
	cofactor.m[11] = -M4_3X3MINOR(0, 1, 3, 0, 1, 2);

	cofactor.m[12] = -M4_3X3MINOR(0, 1, 2, 1, 2, 3);
	cofactor.m[13] =  M4_3X3MINOR(0, 1, 2, 0, 2, 3);
	cofactor.m[14] = -M4_3X3MINOR(0, 1, 2, 0, 1, 3);
	cofactor.m[15] =  M4_3X3MINOR(0, 1, 2, 0, 1, 2);

	return m4_transpose(cofactor);
}

static inline M4 m4_mul_value(M4 a, f32 b) {
    for(u32 i = 0; i < 16; ++i){
        a.m[i] *= b;
    }
    return a;
}

static inline M4 m4_inverse(M4 m) {

	float det = m4_determinant(m);

	if (det == 0.0f) { // Epsilon check would need to be REALLY small
		printf("WARNING: Trying to ivert a matrix with a zero determinant\n");
		return m4_identity();
	}
	M4 adj = m4_adjugate(m);
    
    M4 final = m4_mul_value(adj, (1.0f / det));
    return final;
}

static inline b32 m4_inverse2(const float m[16], float invOut[16])
{
    float inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}


#endif /* _MATH_H_ */
