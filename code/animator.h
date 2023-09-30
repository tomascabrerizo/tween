#ifndef _ANIMATOR_H_
#define _ANIMATOR_H_

#include "gpu.h"

struct PlayingAnimation {
    
    f32 time;
    f32 weight;
    bool loop;
    
    Animation *animation;

    void initialize(Animation *animation, f32 weight, bool loop);
};

#define MAX_ANIMATION_QUEUE 256
struct Animator {
    
    Model model;
    Skeleton skeleton;

    PlayingAnimation animation_queue[MAX_ANIMATION_QUEUE];
    u32 animation_queue_size;
   
    KeyFrame current_keyframe;
    KeyFrame final_keyframe;
    M4 *final_transforms;
    
    void initialize(Model model, Skeleton skeleton);
    void terminate(void);

    void play(char *animation_name, f32 weight, bool loop);
    void update(f32 dt);

};

#endif // _ANIMATOR_H_
