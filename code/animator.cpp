#include "animator.h"
#include "algebra.h"
#include "common.h"
#include "gpu.h"

#include <math.h>
#include <string.h>

static Animation *find_animation_by_name(Skeleton *skeleton, char *name) {
    for(u32 animation_index = 0; animation_index < skeleton->num_animations; ++animation_index) {
        Animation *animation = skeleton->animations + animation_index;
        if(strcmp(animation->name, name) == 0) {
            return animation;
        }
    }
    return nullptr;
}

static void calculate_prev_and_next_frame_for_animation(Animation *animation, KeyFrame *prev, KeyFrame *next, f32 time, f32 *progression) {
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


    if(progression != nullptr) {
        f32 prev_frame_time_stamp = prev->time_stamps[0];
        f32 next_frame_time_stamp = next->time_stamps[0];
        *progression = (time - prev_frame_time_stamp) / (next_frame_time_stamp - prev_frame_time_stamp);
    }
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
        
        if(parent != -1) {
            ASSERT(skeleton != nullptr); ASSERT(bone_index < skeleton->num_bones);
            if(!bone_is_in_hierarchy(skeleton, bone_index, parent)) continue;
        }

        dst->time_stamps[bone_index] = lerp(a.time_stamps[keyframe_bone_index], b.time_stamps[keyframe_bone_index], t);
        dst->positions[bone_index] = v32_lerp(a.positions[keyframe_bone_index], b.positions[keyframe_bone_index], t);
        dst->rotations[bone_index] = q4_slerp(a.rotations[keyframe_bone_index], b.rotations[keyframe_bone_index], t);
        dst->scales[bone_index] = v32_lerp(a.scales[keyframe_bone_index], b.scales[keyframe_bone_index], t);
    }

}

void Transition::initialize(bool active, f32 duration, Animation *dst, f32 dst_animation_time, Animation *src, f32 src_animation_time) {
    this->active = active;
    this->time = 0;
    this->duration = duration;
    
    this->animation_dst = dst;
    this->animation_src = src;
    
    this->animation_dst_time = dst_animation_time;
    this->animation_src_time = src_animation_time;
}

void Transition::terminate(void) {

}

void Transition::update(f32 dt) {
    this->time += dt;
    if(this->time >= duration) {
        this->time = 0;
        this->active = false;
    }
    
    this->animation_dst_time += dt;
    if(this->animation_dst_time >= animation_dst->duration) {
        this->animation_dst_time = 0;
    }
    
    this->animation_src_time += dt;
    if(this->animation_src_time >= animation_src->duration) {
        this->animation_src_time = 0;
    }
}

void Transition::calculate_keyframes(KeyFrame *keyframe) {

    ASSERT(animation_dst != nullptr);
    ASSERT(animation_src != nullptr);

    f32 dst_progression = 0;
    KeyFrame dst_prev, dst_next;
    calculate_prev_and_next_frame_for_animation(animation_dst, &dst_prev, &dst_next, animation_dst_time, &dst_progression);
    mix_keyframes(keyframe, dst_prev, dst_next, dst_progression, -1, nullptr);

    f32 src_progression = 0;
    KeyFrame src_prev, src_next;
    calculate_prev_and_next_frame_for_animation(animation_src, &src_prev, &src_next, animation_src_time, &src_progression);
    
    for(u32 frame_index = 0; frame_index < src_prev.num_animated_bones; ++frame_index) {
        
        u32 bone_index = src_prev.bone_ids[frame_index];

        f32 src_timestamp = lerp(src_prev.time_stamps[frame_index], src_next.time_stamps[frame_index], src_progression);
        V3 src_position = v32_lerp(src_prev.positions[frame_index], src_next.positions[frame_index], src_progression);
        Q4 src_rotation = q4_slerp(src_prev.rotations[frame_index], src_next.rotations[frame_index], src_progression);
        V3 src_scale = v32_lerp(src_prev.scales[frame_index], src_next.scales[frame_index], src_progression);

        keyframe->time_stamps[bone_index] = lerp(keyframe->time_stamps[bone_index], src_timestamp, time);
        keyframe->positions[bone_index] = v32_lerp(keyframe->positions[bone_index], src_position, time);
        keyframe->rotations[bone_index] = q4_slerp(keyframe->rotations[bone_index], src_rotation, time);
        keyframe->scales[bone_index] = v32_lerp(keyframe->scales[bone_index], src_scale, time);
    }
}

void BaseAnimation::update(f32 dt) {
    if(transition.active == true) {
        transition.update(dt);
    } else {
        if(transition_was_active) {
            time = transition.animation_src_time;
            animation = transition.animation_src;
        }
        time += dt;
        if(time >= animation->duration) {
            time = 0;
        }
    }
    transition_was_active = transition.active;
}

void BaseAnimation::initialize(Animation *animation) {
    this->transition.initialize(false, 0, nullptr, 0, nullptr, 0);
    this->animation = animation;
    this->time = 0;
}

void BaseAnimation::calculate_keyframes(KeyFrame *keyframe, f32 time) {
    
    ASSERT(animation != nullptr);

    if(transition.active == true) {
        transition.calculate_keyframes(keyframe);

    } else {
        
        f32 progression = 0;
        KeyFrame prev, next;
        calculate_prev_and_next_frame_for_animation(animation, &prev, &next, time, &progression); 
        mix_keyframes(keyframe, prev, next, progression, -1, nullptr);
    }

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

void Animator::initialize(Model model, Skeleton skeleton) {
    
    this->model = model;
    this->skeleton = skeleton;

    time = 0;
    base_animation.initialize(nullptr);
    initialize_empty_key_frame(&calculated_animation_frame, skeleton.num_bones);
    final_transforms = (M4 *)malloc(sizeof(M4)*skeleton.num_bones);
}

void Animator::terminate(void) {
}

void Animator::play(const char *name) {
    Animation *animation = find_animation_by_name(&skeleton, (char *)name);
    if(base_animation.animation == nullptr) {
        base_animation.initialize(animation);
    } else {
        base_animation.transition.initialize(true, 0.5f, base_animation.animation, time, animation, 0);
    }
}

void Animator::update(f32 dt) {
    
    time += dt;
    base_animation.update(dt);
    base_animation.calculate_keyframes(&calculated_animation_frame, time);

    // NOTE: Calculate final transformation Matrix
    for(u32 bone_index = 0; bone_index < skeleton.num_bones; ++bone_index) { 
        V3 final_position = calculated_animation_frame.positions[bone_index];
        Q4 final_rotation = calculated_animation_frame.rotations[bone_index];
        V3 final_scale    = calculated_animation_frame.scales[bone_index];
        final_transforms[bone_index] = m4_mul(m4_translate(final_position), m4_mul(q4_to_m4(final_rotation), m4_scale_v3(final_scale)));
    }

    // TODO: Create final transfomation matrices
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
    
}
