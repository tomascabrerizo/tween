#ifndef _OS_H_
#define _OS_H_

#include "common.h"

#include <X11/X.h>
#include <x11_gl.h>

typedef struct OsFile {
    void *data;
    u64 size;
} OsFile;

OsFile *os_file_read_entire(const char *path);

void os_file_free(OsFile *file);

u64 os_get_ticks(void);

void os_sleep(u64 milliseconds);

void os_initialize(void);

void os_terminate(void);

typedef struct OsWindow {
    s32 width;
    s32 height;
    b32 resize;
    b32 should_close;

    Window os_window;
    Atom delete_message;

    Colormap cm;
    GLXContext ctx;
    GLXFBConfig best_fbc;
    
} OsWindow;

struct OsWindow *os_window_create(char *title, u32 x, u32 y, u32 w, u32 h);

void os_window_destroy(struct OsWindow *window);

void os_window_poll_events(struct OsWindow *window);

s32 window_width(OsWindow *window);

s32 window_height(OsWindow *window);

/* ---------------------
        OpenGL 
   --------------------- */

void os_gl_create_context(struct OsWindow *window);

void os_gl_destroy_context(struct OsWindow *window);

void os_gl_swap_buffers(struct OsWindow *window);

#endif
