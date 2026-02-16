#include "window.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

struct window_t *window = NULL;

typedef void (*egl_update_callback)(struct window_t *);
typedef int (*egl_after_config_callback)(EGLConfig);

WINTERNALAPI int egl_init(egl_after_config_callback, EGLNativeDisplayType);
WINTERNALAPI int egl_destroy(void);
WINTERNALAPI int egl_update(egl_update_callback);
WINTERNALAPI WINLINE EGLNativeWindowType native_window(void);

static const char *egl_err_tostr(EGLint error)
{
        switch (error) {
        case EGL_SUCCESS:
                return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
                return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
                return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
                return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
                return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
                return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
                return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
                return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
                return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
                return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
                return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
                return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
                return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
                return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
                return "EGL_CONTEXT_LOST";
        default:
                return "UNKNOWN_EGL_ERROR";
        }
}

#if defined(WANDROID)
/** Android Implementation */

#include <errno.h>

#include <android_native_app_glue.h>
#include <android/choreographer.h>
#include <android/native_window.h>
#include <android/log.h>

#undef printf
#define LOG_TAG "nubcube"

/** we don't use WPLATFORMAPI here because android_main shouldn't be static */
void android_main(struct android_app *);
WPLATFORMAPI void android_init_window(void);
WPLATFORMAPI void android_resume_window(void);
WPLATFORMAPI void android_frame(long, void *);
WPLATFORMAPI void android_next_frame(void);
WPLATFORMAPI void android_app_cmd(struct android_app *, int32_t);

int printf(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, args);
        va_end(args);
        return 0;
}

void perror(const char *msg)
{
        char *st = strerror(errno);
        if (!msg) {
                msg = "";
        }
        printf("%s:%s", msg, st);
}

void android_main(struct android_app *app)
{
        int events;
        struct android_poll_source *src;
        app->onAppCmd = android_app_cmd;

        window = malloc(sizeof(*window));
        if (!window) {
                perror("error: can't alloc window");
                exit(1);
        }

        window->display = EGL_NO_DISPLAY;
        window->context = EGL_NO_CONTEXT;
        window->surface = EGL_NO_SURFACE;
        window->width = 0;
        window->height = 0;
        window->running = 0;
        window->__pdata = app;

        do {
                if (ALooper_pollOnce(0, NULL, &events, (void **)&src) >= 0) {
                        if (src)
                                src->process(app, src);
                }
        } while (!app->destroyRequested);
}

WINTERNALAPI WINLINE EGLNativeWindowType native_window(void)
{
        return ((struct android_app *)window->__pdata)->window;
}

/** it'll be called always it calls onResume */
WPLATFORMAPI void android_init_window(void)
{
        int res;
        res = egl_init(NULL, EGL_DEFAULT_DISPLAY);
        if (res != 0) {
                printf("error: android egl failed\n");
                exit(1);
        }
        on_init();
}

/** when user goes back to the activity */
WPLATFORMAPI void android_resume_window(void)
{
        if (!window->running) {
                window->running = 1;
                android_next_frame();
        }
}

/** current frame */
WPLATFORMAPI void android_frame(long _l, void *_d)
{
        (void)_l;
        (void)_d;
        if (!window->running)
                return;
        android_next_frame();
        egl_update(on_update);
}

/** post next frame */
WPLATFORMAPI void android_next_frame(void)
{
        AChoreographer_postFrameCallback64(AChoreographer_getInstance(),
                                           android_frame, NULL);
}

/** handle android events */
WPLATFORMAPI void android_app_cmd(struct android_app *app, int32_t cmd)
{
        switch (cmd) {
        case APP_CMD_INIT_WINDOW:
                if (window && app->window)
                        android_init_window();
                break;
        case APP_CMD_TERM_WINDOW:
                if (window)
                        window->running = 0;
                egl_destroy();
                break;
        case APP_CMD_DESTROY:
                on_destroy();
                free(window);
                window = NULL;
                break;
        case APP_CMD_GAINED_FOCUS:
                if (window && window->display != EGL_NO_DISPLAY)
                        android_resume_window();
                break;
        default:
                break;
        }
}

#elif defined(__linux__)
/** Linux X11 Implementation */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct linux_window {
        Window window;
        Display *display;
};

WPLATFORMAPI int x11_on_delete(struct linux_window *);
WPLATFORMAPI int x11_after_egl_config(EGLConfig);

int main(void)
{
        int ret;
        XEvent event;
        Atom wm_delete_window;
        Window root;
        struct linux_window *lw = calloc(1, sizeof(*lw));
        if (!lw) {
                printf("error: linux_window struct is null\n");
                return -1;
        }

        window = malloc(sizeof(*window));
        if (!window) {
                perror("error: can't alloc window");
                return -1;
        }

        window->display = EGL_NO_DISPLAY;
        window->context = EGL_NO_CONTEXT;
        window->surface = EGL_NO_SURFACE;
        window->width = 0;
        window->height = 0;
        window->running = 1;
        window->__pdata = lw;

        lw->display = XOpenDisplay(NULL);
        if (!lw->display) {
                printf("error: faildd to open display\n");
                return -1;
        }

        root = DefaultRootWindow(lw->display);
        if (root == None) {
                printf("error: no root window\n");
                return -1;
        }

        ret = egl_init(x11_after_egl_config, lw->display);
        if (ret != 0) {
                printf("error: failed to initialize egl\n");
                return -1;
        }

        XMapWindow(lw->display, lw->window);

        wm_delete_window = XInternAtom(lw->display, "WM_DELETE_WINDOW", False);
        if (wm_delete_window == BadValue || wm_delete_window == BadAlloc) {
                printf("error: failed to get atom WM_DELETE_WINDOW.\b");
                return -1;
        }

        XSetWMProtocols(lw->display, lw->window, &wm_delete_window, 1);

        while (window->running) {
                XNextEvent(lw->display, &event);
                switch (event.type) {
                case ClientMessage:
                        if (event.xclient.data.l[0] == (long)wm_delete_window) {
                                x11_on_delete(lw);
                        }
                };

                egl_update(on_update);
        }

        XCloseDisplay(lw->display);

        return 0;
}

WINTERNALAPI WINLINE EGLNativeWindowType native_window(void)
{
        return ((struct linux_window *)window->__pdata)->window;
}

WPLATFORMAPI int x11_on_delete(struct linux_window *lw)
{
        XDestroyWindow(lw->display, lw->window);
        egl_destroy();
        window->running = 0;
        return 0;
}

WPLATFORMAPI int x11_after_egl_config(EGLConfig config)
{
        Window root;
        EGLint visual_id;
        XVisualInfo vi_template;
        XVisualInfo *vi;
        Colormap cmap;
        XSetWindowAttributes swa;
        int n;
        struct linux_window *lw = window->__pdata;

        memset(&vi_template, 0, sizeof(vi_template));
        memset(&cmap, 0, sizeof(cmap));
        memset(&swa, 0, sizeof(swa));

        root = DefaultRootWindow(lw->display);

        /** we can use window->display here safely */
        eglGetConfigAttrib(window->display, config, EGL_NATIVE_VISUAL_ID,
                           &visual_id);

        vi_template.visualid = visual_id;
        vi = XGetVisualInfo(lw->display, VisualIDMask, &vi_template, &n);

        cmap = XCreateColormap(lw->display, root, vi->visual, AllocNone);
        swa.colormap = cmap;
        swa.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;

        lw->window = XCreateWindow(lw->display, root, 0, 0, 100, 100, 0,
                                   vi->depth, InputOutput, vi->visual,
                                   CWColormap | CWEventMask, &swa);

        if (!lw->window) {
                printf("error: failed to crrate X Window\n");
                return -1;
        }

        return 0;
}

#endif

#ifdef WANDROID
#define RENDERABLE_TYPE EGL_OPENGL_ES3_BIT
#define GLMAJOR WGLESMAJOR
#define GLMINOR WGLESMINOR
#else
#define RENDERABLE_TYPE EGL_OPENGL_BIT
#define GLMAJOR WGLMAJOR
#define GLMINOR WGLMINOR
#endif

const EGLint attrs[] = { EGL_RENDERABLE_TYPE,
                         RENDERABLE_TYPE,
                         EGL_SURFACE_TYPE,
                         EGL_WINDOW_BIT,
                         EGL_BLUE_SIZE,
                         8,
                         EGL_GREEN_SIZE,
                         8,
                         EGL_RED_SIZE,
                         8,
                         EGL_NONE };

const EGLint attrs_ctx[] = { EGL_CONTEXT_MAJOR_VERSION, GLMAJOR,
                             EGL_CONTEXT_MINOR_VERSION, GLMINOR, EGL_NONE };

WINTERNALAPI int egl_init(egl_after_config_callback after_config,
                          EGLNativeDisplayType native_display)
{
        int i;
        EGLConfig config, *supported_configs;
        EGLint num_configs;

        window->display = eglGetDisplay(native_display);
        if (window->display == EGL_NO_DISPLAY) {
                printf("error: failed to get default display\n");
                return -1;
        }

        if (!eglInitialize(window->display, NULL, NULL)) {
                printf("error: failed to init egl\n");
                return -1;
        }

        eglChooseConfig(window->display, attrs, NULL, 0, &num_configs);
        supported_configs = malloc(sizeof(*supported_configs) * num_configs);
        if (!supported_configs) {
                perror("null EGLConfig*");
                return -1;
        }
        eglChooseConfig(window->display, attrs, supported_configs, num_configs,
                        &num_configs);
        if (!num_configs) {
                free(supported_configs);
                printf("error: can't find a config for EGL\n");
                return -1;
        }

        for (i = 0; i < num_configs; i++) {
                EGLint r, g, b;
                EGLConfig cfg = supported_configs[i];
                if (eglGetConfigAttrib(window->display, cfg, EGL_RED_SIZE,
                                       &r) &&
                    eglGetConfigAttrib(window->display, cfg, EGL_GREEN_SIZE,
                                       &g) &&
                    eglGetConfigAttrib(window->display, cfg, EGL_BLUE_SIZE,
                                       &b) &&
                    r == 8 && g == 8 && b == 8) {
                        config = supported_configs[i];
                        break;
                }
        }

        if (i == num_configs)
                config = supported_configs[i - 1];

        free(supported_configs);

        if (!config) {
                printf("error: no config for EGL\n");
                return -1;
        }

        if (after_config) {
                if (after_config(config) != 0)
                        return -1;
        }

        window->surface = eglCreateWindowSurface(window->display, config,
                                                 native_window(), NULL);
        if (window->surface == EGL_NO_SURFACE) {
                printf("error: failed to create window surface\n");
                printf("%s\n", egl_err_tostr(eglGetError()));
                return -1;
        }

        window->context =
                eglCreateContext(window->display, config, NULL, attrs_ctx);
        if (window->context == EGL_NO_CONTEXT) {
                printf("error: failed to create a context\n");
                return -1;
        }

        if (!eglMakeCurrent(window->display, window->surface, window->surface,
                            window->context)) {
                printf("error: failed to set current EGL context");
                return -1;
        }

        eglQuerySurface(window->display, window->surface, EGL_WIDTH,
                        &window->width);
        eglQuerySurface(window->display, window->surface, EGL_HEIGHT,
                        &window->height);
        return 0;
}

WINTERNALAPI int egl_update(egl_update_callback cb_update)
{
        if (!window || window->display == EGL_NO_DISPLAY ||
            window->surface == EGL_NO_SURFACE)
                return -1;
        cb_update(window);
        eglSwapBuffers(window->display, window->surface);
        return 0;
}

WINTERNALAPI int egl_destroy(void)
{
        if (!window)
                return -1;

        if (window->display != EGL_NO_DISPLAY) {
                eglMakeCurrent(window->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                               EGL_NO_CONTEXT);
                if (window->context != EGL_NO_CONTEXT) {
                        eglDestroyContext(window->display, window->context);
                }
                if (window->surface != EGL_NO_SURFACE) {
                        eglDestroySurface(window->display, window->surface);
                }
                eglTerminate(window->display);
        }
        window->display = EGL_NO_DISPLAY;
        window->context = EGL_NO_CONTEXT;
        window->surface = EGL_NO_SURFACE;
        return 0;
}
