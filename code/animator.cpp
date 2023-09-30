#include "animator.h"
#include "algebra.h"
#include "common.h"
#include "gpu.h"

#include <string.h>

void PlayingAnimation::initialize(Animation *animation, f32 weight, s32 parent, bool loop) {
    this->time = 0;
    this->weight = weight;
    this->loop = loop;
    this->parent = parent;
    this->animation = animation;
}


static void initialize_empty_key_frame(KeyFrame *keyframe, u32 num_bones) {
    keyframe->num_animated_bones = num_bones;
    keyframe->bone_ids    = (u32 *)malloc(sizeof(u32)*keyframe->num_animated_bones);
    keyframe->time_stamps = (f32 *)malloc(sizeof(f32)*keyframe->num_animated_bones);
    keyframe->scales      = (V3 *)malloc(sizeof(V3)*keyframe->num_animated_bones);
    keyframe->positions   = (V3 *)malloc(sizeof(V3)*keyframe->num_animated_bones);
    keyframe->rotations   = (Q4 *)malloc(sizeof(Q4)*keyframe->num_animated_bones);

    for(u32 frame_index = 0; frame_index < keyframe->num_animated_bones; ++frame_index) {
        keyframe->bone_ids[frame_index] = frame_index;
        keyframe->time_stamps[frame_index] = 0;
        keyframe->scales[frame_index] = v3(1, 1, 1);
        keyframe->positions[frame_index] = v3(0, 0, 0);
        keyframe->rotations[frame_index] = q4(1, 0, 0, 0);
    }

}

static void terminate_key_frame(KeyFrame *keyframe) {
    free(keyframe->bone_ids);
    free(keyframe->time_stamps);
    free(keyframe->scales);
    free(keyframe->positions);
    free(keyframe->rotations);

    keyframe->num_animated_bones = 0;
}

static Animation *find_animation_by_name(Skeleton *skeleton, char *name) {
    for(u32 animation_index = 0; animation_index < skeleton->num_animations; ++animation_index) {
        Animation *animation = skeleton->animations + animation_index;
        if(strcmp(animation->name, name) == 0) {
            return animation;
        }
    }
    return nullptr;
}

static s32 find_bone_id_by_name(Skeleton *skeleton, char *bone_name) {
    for(u32 bone_index = 0; bone_index < skeleton->num_bones; ++bone_index) {
        Bone *bone = skeleton->bones + bone_index;
        if(strcmp(bone->name, bone_name) == 0) {
            return bone_index;
        }
    }
    ASSERT(!"Invalid code path");
    return -1;
}

void Animator::initialize(Model model, Skeleton skeleton) {
    this->model = model;
    this->skeleton = skeleton;
    
    memset(animation_queue, 0, sizeof(PlayingAnimation)*MAX_ANIMATION_QUEUE);
    animation_queue_size = 0;
    
    initialize_empty_key_frame(&current_keyframe, skeleton.num_bones);
    initialize_empty_key_frame(&final_keyframe, skeleton.num_bones);
    
    final_transforms = (M4 *)malloc(sizeof(M4)*skeleton.num_bones);
}

void Animator::terminate(void) {
    terminate_key_frame(&current_keyframe);
    terminate_key_frame(&final_keyframe);
}

void Animator::play(char *parent_bone_name, char *animation_name, f32 weight, bool loop) {
    
    if(animation_queue_size >= MAX_ANIMATION_QUEUE) return;
    
    s32 parent = find_bone_id_by_name(&skeleton, parent_bone_name);
    Animation *animation = find_animation_by_name(&skeleton, animation_name);
    ASSERT(animation != nullptr);
    animation_queue[animation_queue_size++].initialize(animation, weight, parent, loop);
}

static void calculate_prev_and_next_frame_for_animation(Animation *animation, KeyFrame *prev, KeyFrame *next, f32 time) {
    u32 prev_frame_index = 0;
    u32 next_frame_index = 1;

    for(u32 frame_index = 1; frame_index < animation->num_frames; ++frame_index) {
        KeyFrame *frame = animation->frames + frame_index;
        ASSERT(frame->num_animated_bones > 0);
        f32 frame_time_stamp = frame->time_stamps[0];

        if(frame_time_stamp > time) {
            next_frame_index = frame_index;
            break;
        }
    }
    
    ASSERT(next_frame_index > 0);
    prev_frame_index = (next_frame_index - 1);

    *prev = animation->frames[prev_frame_index];
    *next = animation->frames[next_frame_index];
}

static bool bone_is_in_hierarchy(Skeleton *skeleton, s32 bone_index, s32 parent_index) {
    
    Bone *bone = skeleton->bones + bone_index;

    while(bone->parent > parent_index) {
        bone = skeleton->bones + bone->parent;
    }
    
    if(bone->parent == parent_index) return true;
    
    return false;
}

static void mix_keyframes(KeyFrame *dst, KeyFrame a, KeyFrame b, f32 t, s32 parent, Skeleton *skeleton) {
    
    // TODO: Only mix the necesary information 

    ASSERT(a.num_animated_bones == b.num_animated_bones);
    
    for(u32 keyframe_bone_index = 0; keyframe_bone_index < a.num_animated_bones; ++keyframe_bone_index) {
        
        ASSERT(a.bone_ids[keyframe_bone_index] == b.bone_ids[keyframe_bone_index]);
        u32 bone_index = a.bone_ids[keyframe_bone_index];
        
        ASSERT(bone_index < skeleton->num_bones);
        if(!bone_is_in_hierarchy(skeleton, bone_index, parent)) continue;


        dst->time_stamps[bone_index] = lerp(a.time_stamps[keyframe_bone_index], b.time_stamps[keyframe_bone_index], t);
        dst->positions[bone_index] = v32_lerp(a.positions[keyframe_bone_index], b.positions[keyframe_bone_index], t);
        dst->rotations[bone_index] = q4_slerp(a.rotations[keyframe_bone_index], b.rotations[keyframe_bone_index], t);
        dst->scales[bone_index] = v32_lerp(a.scales[keyframe_bone_index], b.scales[keyframe_bone_index], t);
    }
}

void Animator::update(f32 dt) {
    
    // NOTE: Update all animations times and remove old animations
    for(s32 animation_index = 0; animation_index < (s32)animation_queue_size; ++animation_index) {
        PlayingAnimation *playing = animation_queue + animation_index;
        playing->time += dt;
        
        if(playing->time >= playing->animation->duration) {
            if(playing->loop == true) {
                playing->time = 0;
            } else {
                u32 next_animation_index = animation_index + 1;
                u32 total_size_to_move = (animation_queue_size - next_animation_index)*sizeof(PlayingAnimation);
                memmove(animation_queue + animation_index, animation_queue + next_animation_index, total_size_to_move);
                --animation_index;
                --animation_queue_size;
            }
        }
    }

    // NOTE: Calculate and Mix all current animations keyframes
    for(u32 animation_index = 0; animation_index < animation_queue_size; ++animation_index) {
        PlayingAnimation *playing = animation_queue + animation_index;
        
        KeyFrame prev, next;
        calculate_prev_and_next_frame_for_animation(playing->animation, &prev, &next, playing->time);
        
        // TODO: remove time_stamp for each bones, all bones must have the same timestamp
        f32 prev_frame_time_stamp = prev.time_stamps[0];
        f32 next_frame_time_stamp = next.time_stamps[0];
        f32 progression = (playing->time - prev_frame_time_stamp) / (next_frame_time_stamp - prev_frame_time_stamp);
        
        mix_keyframes(&current_keyframe, prev, next, progression, playing->parent, &skeleton);
        mix_keyframes(&final_keyframe, final_keyframe, current_keyframe, playing->weight, playing->parent, &skeleton);
    }
    
    
    if(animation_queue_size > 0) {

        // NOTE: Calculate final transformation Matrix
        for(u32 bone_index = 0; bone_index < skeleton.num_bones; ++bone_index) { 
            V3 final_position = final_keyframe.positions[bone_index];
            Q4 final_rotation = final_keyframe.rotations[bone_index];
            V3 final_scale    = final_keyframe.scales[bone_index];
            final_transforms[bone_index] = m4_mul(m4_translate(final_position), m4_mul(q4_to_m4(final_rotation), m4_scale_v3(final_scale)));
        }

        for(u32 bone_index = 0; bone_index < skeleton.num_bones; ++bone_index) {
            Bone *bone = skeleton.bones + bone_index;
            if(bone->parent == -1) {
                final_transforms[bone_index] = m4_mul(m4_identity(), final_transforms[bone_index]);
            } else {
                ASSERT(bone->parent < (s32)bone_index);
                final_transforms[bone_index] = m4_mul(final_transforms[bone->parent], final_transforms[bone_index]);

            }
        }

        ASSERT(skeleton.num_bones == model.num_inv_bind_transform);
        
        for(u32 bone_index = 0; bone_index < skeleton.num_bones; ++bone_index) {
            final_transforms[bone_index] = m4_mul(final_transforms[bone_index], model.inv_bind_transform[bone_index]);
        }
    } else {
        
        for(u32 bone_index = 0; bone_index < skeleton.num_bones; ++bone_index) { 
            final_transforms[bone_index] = m4_identity();
        }
    }

}
