#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>

Display *g_x11_display = 0;
int g_x11_screen = 0;

u32 os_keyboard[1024];

OsFile *os_file_read_entire(const char *path) {
    OsFile *result = (OsFile *)malloc(sizeof(OsFile));

    FILE *file = fopen((char *)path, "rb");
    if(!file) {
        printf("Cannot load file: %s\n", path);
        free(result);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    u64 file_size = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    result = (OsFile *)malloc(sizeof(OsFile) + file_size+1);
    
    result->data = result + 1;
    result->size = file_size;

    ASSERT(((u64)result->data % 8) == 0);
    fread(result->data, file_size+1, 1, file);
    ((u8 *)result->data)[file_size] = '\0';
    
    fclose(file);

    return result;
}

void os_file_free(OsFile *file) {
    free(file);
}

u64 os_get_ticks(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    u64 result = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    return result;
}

void os_sleep(u64 milliseconds) {
    
    u64 seconds = milliseconds / 1000; 
    u64 nanoseconds = (milliseconds - (seconds * 1000)) * 1000000;

    struct timespec req;
    req.tv_sec = seconds; 
    req.tv_nsec = nanoseconds;
    while(nanosleep(&req, &req) == -1);
}

void os_initialize(void) {

    g_x11_display = XOpenDisplay(NULL);
    g_x11_screen = DefaultScreen(g_x11_display);
}

void os_terminate(void) {
    XCloseDisplay(g_x11_display);
}

struct OsWindow *os_window_create(char *title, u32 x, u32 y, u32 w, u32 h) {

    OsWindow *window = (OsWindow *)malloc(sizeof(OsWindow));
    memset(window, 0, sizeof(OsWindow));
    
    window->resize = true;
    window->width = w;
    window->height = h;

    Display *d = g_x11_display;
  
    static int fb_attr[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
  
    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(d, DefaultScreen(d), fb_attr, &fbcount);
    if (!fbc) {
        printf("Failed to retrieve a X11 framebuffer config\n");
        return NULL;
    }
  
    /* Pick the FB config/visual with the most samples per pixel */
    int best_fbc_index = -1, worst_fbc_index = -1, best_num_samp = -1, worst_num_samp = 999;
  
    for(int i = 0; i < fbcount; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(d, fbc[i]);
        if(vi) {
            int samp_buf, samples;
            glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLES, &samples);
            
           if((best_fbc_index < 0 || samp_buf) && samples > best_num_samp)
               best_fbc_index = i, best_num_samp = samples;
           if((worst_fbc_index < 0 || !samp_buf) || samples < worst_num_samp)
               worst_fbc_index = i, worst_num_samp = samples;

        }
        XFree(vi);
    }

    window->best_fbc = fbc[best_fbc_index];
    XFree(fbc);
    XVisualInfo *vi = glXGetVisualFromFBConfig(d, window->best_fbc);
  
    XSetWindowAttributes swa;
    window->cm = XCreateColormap(d, RootWindow(d, vi->screen), vi->visual, AllocNone);

    long event_mask = StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|KeymapStateMask;
    swa.colormap = window->cm;
    swa.background_pixmap = None ;
    swa.border_pixel      = 0;
    swa.event_mask        = event_mask;

    window->os_window = XCreateWindow(d, RootWindow(d, vi->screen), 
                            x, y, window->width, window->height, 0, vi->depth, InputOutput, vi->visual, 
                            CWBorderPixel|CWColormap|CWEventMask, &swa);

    XStoreName(d, window->os_window, title);
    XSelectInput(d, window->os_window, event_mask);
    XMapWindow(d, window->os_window);
    
    /* TODO: investigate why is necesary to do this to catch WM_DELETE_WINDOW message */
    window->delete_message = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, window->os_window, &window->delete_message, 1);
    
    XFlush(d);

    return window;

}

void os_window_destroy(struct OsWindow *window) {
    Display *d = g_x11_display;
    XFreeColormap(d, window->cm);
    XDestroyWindow(d, window->os_window);
    free(window);
}

void os_window_poll_events(struct OsWindow *window) {
    
    Display *d = g_x11_display;
    XEvent e; 

    while(XPending(d)) {
     
        XNextEvent(d, &e);
        
        switch(e.type) {
        
        case ClientMessage: {
            if (e.xclient.data.l[0] == (long int)window->delete_message) {
                window->should_close = true;
            }
        } break;
       
        case ConfigureNotify: {
            XConfigureEvent xce = e.xconfigure;
            if (xce.width != window->width || xce.height != window->height) {
                
                window->resize = true;
                window->width = xce.width;
                window->height = xce.height;
            }
        } break;

        case Expose: {

        } break;
        
        case KeymapNotify: {
            XRefreshKeyboardMapping(&e.xmapping);
        } break;
        
        case KeyPress: {

            KeySym sym = XLookupKeysym(&e.xkey, 0);

            if(sym == XK_Right) {
            } else if(sym == XK_Left) {
            } else if(sym == XK_BackSpace) {
            } else if(sym == XK_Delete) {
            } else if(sym == XK_1) {
                os_keyboard['1'] = true;
            } else if(sym == XK_2) {
                os_keyboard['2'] = true;
            } else if(sym == XK_3) {
                os_keyboard['3'] = true;
            }

        } break;

        case KeyRelease: {

            KeySym sym = XLookupKeysym(&e.xkey, 0);

            if(sym == XK_Right) {
            } else if(sym == XK_Left) {
            } else if(sym == XK_BackSpace) {
            } else if(sym == XK_Delete) {
            } else if(sym == XK_1) {
                os_keyboard['1'] = false;
            } else if(sym == XK_2) {
                os_keyboard['2'] = false;
            } else if(sym == XK_3) {
                os_keyboard['3'] = false;
            }

        } break;

        case ButtonPress: {
            if(e.xbutton.button == 1) {
            }
        } break;
        case ButtonRelease: {
            if(e.xbutton.button == 1) {
            }
        } break;
        
        
        }
    }

    Window inwin;
    Window inchildwin;
    int rootx, rooty;
    int childx, childy;
    unsigned int mask;
    XQueryPointer(d, window->os_window, &inwin, &inchildwin, &rootx, &rooty, &childx, &childy, &mask);
}

s32 window_width(OsWindow *window) {
    return window->width;
}

s32 window_height(OsWindow *window) {
    return window->height;
}

/* ---------------------
        OpenGL 
   --------------------- */

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
typedef void (*glXSwapIntervalEXTProc)(Display *, GLXDrawable , int);

#define X(return, name, params) TGUI_GL_PROC(name) name;
GL_FUNCTIONS(X)
#undef X

void os_gl_create_context(struct OsWindow *window) {
    Display *d = g_x11_display;
    
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
  
    int ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        None
    };
  
    /* TODO: Get context version to be sure we are in 3.3 version */
    window->ctx = glXCreateContextAttribsARB(d, window->best_fbc, 0, True, ctx_attribs);
    XSync(d, False);
    if(!glXIsDirect(d, window->ctx)) {
        printf("Indirect rendering not supported\n");
        return;
    }

    glXMakeCurrent(d, window->os_window, window->ctx); 
  
    /* Set vertical Sync */
    glXSwapIntervalEXTProc glXSwapIntervalEXT = (glXSwapIntervalEXTProc)glXGetProcAddress((GLubyte *)"glXSwapIntervalEXT");
    if(glXSwapIntervalEXT) {
        glXSwapIntervalEXT(d, window->os_window, 0);
        printf("Vertical Sync enable\n");
    } else {
        printf("vertical Sync disable\n");
    }

    /* TODO: load opnegl function: */
    #define X(return, name, params) name = (TGUI_GL_PROC(name))glXGetProcAddress((GLubyte *)#name);
    GL_FUNCTIONS(X)
    #undef X

}

void os_gl_destroy_context(struct OsWindow *window) {
    Display *d = g_x11_display;
    glXDestroyContext(d, window->ctx);
}

void os_gl_swap_buffers(struct OsWindow *window) {
    Display *d = g_x11_display;
    glXSwapBuffers(d, window->os_window);
}
