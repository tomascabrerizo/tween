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
    s32 parent;
    M4 local_transform;
    M4 inv_bind_transform;
};

struct Skeleton {
    char name[MAX_NAME_SIZE];
    Joint *joints;
    u32 num_joints;

    s32 get_joint_index(const char *name);
    bool joint_is_in_hierarchy(s32 index, s32 parent_index);
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
    AnimationClip *animation;

    f32 time;
    f32 weight;
    
    bool enable;
    bool loop;

    s32 root;

    void sample_animation_pose(JointPose *pose);

private:

    void sample_prev_and_next_animation_pose(AnimationSample *prev, AnimationSample *next, f32 time);
    void mix_samples(JointPose *dst, JointPose *a, JointPose *b, f32 t);

};

struct AnimationSet {
    
    Skeleton *skeleton;
    AnimationState *states;
    u32 num_states;
    M4 *final_transform_matrices;

    void initialize(AnimationClip *animations, u32 num_animations);
    void terminate(void);
    
    void play(const char *name, f32 weight, bool loop);
    void stop(const char *name);
    void update_weight(const char *name, f32 weight);

    void update(f32 dt);
    
    void set_root_joint(const char *name, const char *joint);

private:
    
    void update_animation_state(AnimationState *state, f32 dt);
    void zero_final_local_pose(void);
    void calculate_final_transform_matrices(void);
    
    AnimationState *find_animation_by_name(const char *name);

    // NOTE: This must be skeleton poses
    JointPose *intermidiate_local_pose;
    JointPose *final_local_pose;

};


#endif // _ANIMATION_H_
