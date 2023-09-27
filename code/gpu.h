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

#define MESH_MAX_NAME_SIZE 256
typedef struct Mesh {
    
    u32 vao;
    u32 vbo;
    u32 ebo;

    u32 texture;
    
    Vertex *vertices;
    u32 num_vertices;

    u32 *indices;
    u32 num_indices;

    char material[MESH_MAX_NAME_SIZE];

} Mesh;

typedef struct Joint {
    
    u32 id;
    
    M4 local_bind_transform;
    M4 invers_bind_transform;

    struct Joint *parent;
    struct Joint *first;
    struct Joint *last;
    struct Joint *next;

} Joint;

struct BoneInfo;

typedef struct Model {
    
    Mesh *meshes;
    u32 num_meshes;
    
    struct BoneInfo *bone_info_registry;
    u32 bone_info_registry_count;
    u32 bone_info_registry_capacity;

    Joint *root;
    u32 num_joint;

} Model;

u32 gpu_create_prorgam(char *vert, char *frag);

void gpu_destroy_program(void *program);

u32 gpu_create_texture(u32 *data, u32 width, u32 height);

void gpu_destroy_texture(void *texture);

void gpu_load_mesh_data(u32 *vao, u32 *vbo, u32 *ebo, Vertex *vertices, u32 num_vertices, u32 *indices, u32 num_indices);

#endif /* _GPU_H_ */
