#include "animation.h"
#include "algebra.h"
#include "common.h"

#include <cstdlib>
#include <string.h>

/* -------------------------------------------- */
/*        Skeleton                              */
/* -------------------------------------------- */

s32 Skeleton::get_joint_index(const char *name) {
    for(u32 joint_index = 0; joint_index < num_joints; ++joint_index) {
        Joint *joint = joints + joint_index;
        if(strcmp(joint->name, name) == 0) {
            return joint_index;
        }
    }
    ASSERT(!"Invalid code path");
    return -1;
}

bool Skeleton::joint_is_in_hierarchy(s32 index, s32 parent_index) {

    if(index == parent_index) return true;

    Joint *joint = joints + index;
    while(joint->parent > parent_index) {
        joint = joints + joint->parent;
    }
    if(joint->parent == parent_index) return true;
    return false;
}

/* -------------------------------------------- */
/*        Animation State                       */
/* -------------------------------------------- */

void AnimationState::sample_prev_and_next_animation_pose(AnimationSample *prev, AnimationSample *next, f32 time) {
    
    u32 next_sample_index = 1;
    for(u32 sample_index = 0; sample_index < animation->num_samples; ++sample_index) {
        AnimationSample *sample = animation->samples + sample_index;
        if(sample->time_stamp > time) {
            next_sample_index = sample_index;
            break;
        }
    }
    
    u32 prev_sample_index = next_sample_index - 1;
    
    *prev = animation->samples[prev_sample_index];
    *next = animation->samples[next_sample_index];

}

void AnimationState::mix_samples(JointPose *dst, JointPose *a, JointPose *b, f32 t) {
    Skeleton *skeleton = animation->skeleton;
    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) {
        dst[joint_index].position = v3_lerp(a[joint_index].position, b[joint_index].position, t);
        dst[joint_index].rotation = q4_slerp(a[joint_index].rotation, b[joint_index].rotation, t);
        dst[joint_index].scale = v3_lerp(a[joint_index].scale, b[joint_index].scale, t);
    }
}

void AnimationState::sample_animation_pose(JointPose *pose) {
    
    AnimationSample prev, next; 
    sample_prev_and_next_animation_pose(&prev, &next, time);
    
    f32 progression = (time - prev.time_stamp) / (next.time_stamp - prev.time_stamp);
    mix_samples(pose, prev.local_poses, next.local_poses, progression);
}

/* -------------------------------------------- */
/*        Animation Set                         */
/* -------------------------------------------- */

void AnimationSet::initialize(AnimationClip *animations, u32 num_animations) {
    
    skeleton = animations[0].skeleton;
    num_states = num_animations;
    states = (AnimationState *)malloc(sizeof(AnimationState)*num_states);

    for(u32 animation_index = 0; animation_index < num_animations; ++animation_index) {
        
        AnimationClip *animation = animations + animation_index;
        AnimationState *animation_state = states + animation_index;
        
        ASSERT(skeleton == animation->skeleton);
        
        animation_state->animation = animation;
        animation_state->time = 0;
        animation_state->weight = 0;
        animation_state->enable = false;
        animation_state->loop = false;
        animation_state->root = 0;
    
    }

    final_local_pose = (JointPose *)malloc(sizeof(JointPose)*skeleton->num_joints);
    intermidiate_local_pose = (JointPose *)malloc(sizeof(JointPose)*skeleton->num_joints);
    
    final_transform_matrices = (M4 *)malloc(sizeof(M4)*skeleton->num_joints);
}

void AnimationSet::terminate(void) {
    free(states);
    free(final_local_pose);
    free(intermidiate_local_pose);
    free(final_transform_matrices);
}

void AnimationSet::play(const char *name, f32 weight, bool loop) {
    AnimationState *animation = find_animation_by_name(name);
    ASSERT(animation != nullptr);
    animation->time = 0;
    animation->weight = weight;
    animation->enable = true;
    animation->loop = loop;
}

void AnimationSet::stop(const char *name) {
    AnimationState *animation = find_animation_by_name(name);
    ASSERT(animation != nullptr);
    animation->enable = false;
}

void AnimationSet::update_weight(const char *name, f32 weight) {
    AnimationState *animation = find_animation_by_name(name);
    ASSERT(animation != nullptr);
    animation->weight = weight;
}

void AnimationSet::set_root_joint(const char *name, const char *joint) {
    AnimationState *animation = find_animation_by_name(name);
    ASSERT(animation != nullptr);
    animation->root = skeleton->get_joint_index(joint);
}

void AnimationSet::update(f32 dt) {
    
    zero_final_local_pose();
    
    for(u32 state_index = 0; state_index < num_states; ++state_index) {
        AnimationState *state = states + state_index;
        if(state->enable) {
            update_animation_state(state, dt);
        }
    }

    calculate_final_transform_matrices();

}

void AnimationSet::calculate_final_transform_matrices(void) {

    // NOTE: Calculate final transformation Matrix
    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) { 
        V3 final_position = final_local_pose[joint_index].position;
        Q4 final_rotation = final_local_pose[joint_index].rotation;
        V3 final_scale    = final_local_pose[joint_index].scale;
        final_transform_matrices[joint_index] = m4_mul(m4_translate(final_position), m4_mul(q4_to_m4(final_rotation), m4_scale_v3(final_scale)));
    }

    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) {
        Joint *joint = skeleton->joints + joint_index;
        if(joint->parent == -1) {
            final_transform_matrices[joint_index] = m4_mul(m4_identity(), final_transform_matrices[joint_index]);
        } else {
            ASSERT(joint->parent < (s32)joint_index);
            final_transform_matrices[joint_index] = m4_mul(final_transform_matrices[joint->parent], final_transform_matrices[joint_index]);

        }
    }

    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) {
        Joint *joint = skeleton->joints + joint_index;
        final_transform_matrices[joint_index] = m4_mul(final_transform_matrices[joint_index], joint->inv_bind_transform);
    }

}

void AnimationSet::update_animation_state(AnimationState *state, f32 dt) {
    AnimationClip *animation = state->animation;
    
    // NOTE: update animation time
    state->time += dt;
    if(state->time >= animation->duration) {
        if(state->loop) {
            state->time = 0;
        } else {
            state->enable = false;
            return;
        }
    }
    
    state->sample_animation_pose(intermidiate_local_pose);

    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) { 
        if(skeleton->joint_is_in_hierarchy(joint_index, state->root)) {
            JointPose *sample_local_pose = intermidiate_local_pose + joint_index;
            JointPose *local_pose = final_local_pose + joint_index;
            
            local_pose->position = v3_lerp(local_pose->position, sample_local_pose->position, state->weight);
            local_pose->rotation = q4_slerp(local_pose->rotation, sample_local_pose->rotation, state->weight);
            local_pose->scale = v3_lerp(local_pose->scale, sample_local_pose->scale, state->weight);
        }
    }
}

AnimationState *AnimationSet::find_animation_by_name(const char *name) {
    for(u32 state_index = 0; state_index < num_states; ++state_index) {
        AnimationState *state = states + state_index;
        if(strcmp(state->animation->name, name) == 0) {
            return state;
        }
    }
    return nullptr;
}

void AnimationSet::zero_final_local_pose(void) {
    
    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) { 
        JointPose *local_pose  = final_local_pose + joint_index;
        local_pose->position = v3(0, 0, 0);
        local_pose->rotation = q4(0, 0, 0, 0);
        local_pose->scale = v3(0, 0, 0);
    }
}
