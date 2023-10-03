#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include "common.h"
#include "algebra.h"

#define MAX_NAME_SIZE 256
#define MAX_FINAL_BONE_MATRICES 100
#define MAX_BONES_INFLUENCE 4

typedef struct Vertex {
    V3 pos;
    V2 uv;
    V3 color;

    s32 bones_id[MAX_BONES_INFLUENCE];
    f32 weights[MAX_BONES_INFLUENCE];

} Vertex;

struct Joint {
    char name[MAX_NAME_SIZE];
    u32 parent;
    M4 local_transform;
    M4 inv_bind_transform;
};

struct Skeleton {
    char name[MAX_NAME_SIZE];
    Joint *joints;
    u32 num_joints;
};

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

    Skeleton *skeleton;

} Model;

// NOTE: the size of all JointPose array is the number of joints of the parent skeleton
struct JointPose {
    V3 position;
    Q4 rotation;
    V3 scale;
};

struct SkeletonPose {
    Skeleton *skeleton;
    JointPose *local_poses;
};

struct AnimationSample {
    f32 time_stamp;
    JointPose *local_poses;
};

struct AnimationClip {
    Skeleton *skeleton;
    
    char name[MAX_NAME_SIZE];
    f32 duration;
    AnimationSample *samples;
    u32 num_samples;
};

struct AnimationState {
    f32 time;
    f32 weight;
    
    bool enable;
    bool loop;
    
    AnimationClip *animation;   
};

struct AnimationSet {
    AnimationState *states;
    u32 num_states;
};


#endif // _ANIMATION_H_
