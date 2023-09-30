#ifndef _GPU_H_
#define _GPU_H_

#include "x11_gl.h"
#include "algebra.h"

#define MAX_FINAL_BONE_MATRICES 100
#define MAX_BONES_INFLUENCE 4
typedef struct Vertex {
    V3 pos;
    V2 uv;
    V3 color;

    s32 bones_id[MAX_BONES_INFLUENCE];
    f32 weights[MAX_BONES_INFLUENCE];

} Vertex;

#define MAX_NAME_SIZE 256
typedef struct Mesh {
    
    u32 vao;
    u32 vbo;
    u32 ebo;

    u32 texture;
    
    Vertex *vertices;
    u32 num_vertices;

    u32 *indices;
    u32 num_indices;

    char material[MAX_NAME_SIZE];
    
} Mesh;

typedef struct Model {
    
    Mesh *meshes;
    u32 num_meshes;

    M4 *inv_bind_transform;
    u32 num_inv_bind_transform;
    
} Model;

typedef struct Bone {
    char name[MAX_NAME_SIZE];
    s32 parent;
    M4 transformation;
} Bone;

typedef struct KeyFrame {
    
    u32 *bone_ids;
    f32 *time_stamps;
    
    V3 *positions;
    Q4 *rotations;
    V3 *scales;
    
    u32 num_animated_bones;

} KeyFrame;

typedef struct Animation {
    char name[MAX_NAME_SIZE];
    f32 duration;
    KeyFrame *frames;
    u32 num_frames;
} Animation;

typedef struct Skeleton {

    char name[MAX_NAME_SIZE];
    
    Bone *bones;
    u32 num_bones;

    Animation *animations;
    u32 num_animations;

    s32 active_animation;
    s32 last_animation;
    KeyFrame transition_keyframe;

} Skeleton;

u32 gpu_create_prorgam(char *vert, char *frag);

void gpu_destroy_program(void *program);

u32 gpu_create_texture(u32 *data, u32 width, u32 height);

void gpu_destroy_texture(void *texture);

void gpu_load_mesh_data(u32 *vao, u32 *vbo, u32 *ebo, Vertex *vertices, u32 num_vertices, u32 *indices, u32 num_indices);

#endif /* _GPU_H_ */
