#ifndef _ANIMATOR_H_
#define _ANIMATOR_H_

#include "gpu.h"

struct Transition {
    bool active;

    f32 duration;
    Animation *animation_src;
    Animation *animation_dst;

    f32 time;
    f32 animation_src_time;
    f32 animation_dst_time;

    void initialize(bool active, f32 duration, Animation *dst, f32 dst_animation_time, Animation *src, f32 src_animation_time);
    void terminate(void);
    
    void update(f32 dt);
    void calculate_keyframes(KeyFrame *keyframe);
}; 

struct BaseAnimation {
    
    Transition transition;
    bool transition_was_active;

    Animation *animation;
    f32 time;

    void initialize(Animation *animation);
    
    void update(f32 dt);
    void calculate_keyframes(KeyFrame *keyframe, f32 time);
};

struct MixAnimation {

};

#define MAX_ANIMATION_QUEUE 256
struct Animator {
    
    Model model;
    Skeleton skeleton;
    
    f32 time;
    BaseAnimation base_animation;

    KeyFrame calculated_animation_frame;
    M4 *final_transforms;

    void initialize(Model model, Skeleton skeleton);
    void terminate(void);

    void update(f32 dt);

    void play(const char *name);
};

#endif // _ANIMATOR_H_
