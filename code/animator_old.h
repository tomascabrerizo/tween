#ifndef _ANIMATOR_H_
#define _ANIMATOR_H_

#include "gpu.h"

enum PlayingAnimationFlags {
   
    ANIMATION_SMOOTH       = (1 << 1),
    ANIMATION_SMOOTH_START = (1 << 2),
    ANIMATION_SMOOTH_END   = (1 << 3),
    
    ANIMATION_TO_REMOVE    = (1 << 4),
    ANIMATION_TO_LOOP      = (1 << 5),
};

struct PlayingAnimation {
    

    char name[MAX_NAME_SIZE];
    f32 weight;
    s32 parent;
    
    f32 time;
    
    u32 flags;
    Animation *animation;

    void initialize(Animation *animation, f32 weight, s32 parent, u32 flags);
    
    void flag_add(u32 flag);
    void flag_remove(u32 flag);
    bool flag_is_set(u32 flag);
    
};

#define MAX_ANIMATION_QUEUE 256
struct Animator {
    
    Model model;
    Skeleton skeleton;
    
    Animation *animation;
    PlayingAnimation animation_queue[MAX_ANIMATION_QUEUE];
    u32 animation_queue_size;
   
    KeyFrame current_keyframe;
    KeyFrame final_keyframe;
    M4 *final_transforms;
    
    void initialize(Model model, Skeleton skeleton);
    void terminate(void);

    void play(char *parent_bone_name, char *animation_name, f32 weight, bool loop);
    void update(f32 dt);

    void start_animation(const char *name, b32 loop, b32 smooth, f32 wieght, const char *parent_bone_name);
    void stop_animation(const char *name, b32 smooth);
    
    bool animation_is_playing(const char *name);
    void update_animations(f32 dt);
    
private:
    void remove_animation_from_queue(u32 animation_index);
    s32 find_playing_animation_index(const char *name);

};

#endif // _ANIMATOR_H_
