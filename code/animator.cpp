#include "animator.h"
#include "common.h"
#include "gpu.h"

#include <string.h>

void PlayingAnimation::initialize(Animation *animation, f32 weight, bool loop) {
    this->time = 0;
    this->weight = weight;
    this->loop = loop;
    this->animation = animation;
}


static void initialize_empty_key_frame(KeyFrame *keyframe, u32 num_bones) {
    keyframe->num_animated_bones = num_bones;
    keyframe->bone_ids    = (u32 *)malloc(sizeof(u32)*keyframe->num_animated_bones);
    keyframe->time_stamps = (f32 *)malloc(sizeof(f32)*keyframe->num_animated_bones);
    keyframe->scales      = (V3 *)malloc(sizeof(V3)*keyframe->num_animated_bones);
    keyframe->positions   = (V3 *)malloc(sizeof(V3)*keyframe->num_animated_bones);
    keyframe->rotations   = (Q4 *)malloc(sizeof(Q4)*keyframe->num_animated_bones);

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

void Animator::initialize(Model model, Skeleton skeleton) {
    this->model = model;
    this->skeleton = skeleton;
    
    memset(animation_queue, 0, sizeof(PlayingAnimation)*MAX_ANIMATION_QUEUE);
    animation_queue_size = 0;
    
    initialize_empty_key_frame(&current_keyframe, skeleton.num_bones);
}

void Animator::terminate(void) {
    terminate_key_frame(&current_keyframe);
}

void Animator::play(char *animation_name, f32 weight, bool loop) {
    Animation *animation = find_animation_by_name(&skeleton, animation_name);
    ASSERT(animation != nullptr);
    animation_queue[animation_queue_size++].initialize(animation, weight, loop);
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
    for(s32 animation_index = 0; animation_index < (s32)animation_queue_size; ++animation_index) {
        PlayingAnimation *playing = animation_queue + animation_index;
        ASSERT(!"Uninplemented codepath!"); (void)playing;
    }

}
