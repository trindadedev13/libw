#ifndef __window_h__
#define __window_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#ifndef WINLINE
#define WINLINE inline
#endif

#ifndef NAPI
#define NAPI
#endif

#ifndef WINTERNALAPI
#define WINTERNALAPI NAPI static
#endif

#ifndef NCALLBACK
#define NCALLBACK extern
#endif

#ifndef WPLATFORMAPI
#define WPLATFORMAPI static
#endif

struct window_t {
        EGLDisplay display;
        EGLSurface surface;
        EGLContext context;

        int width, height, running;

        void *__pdata; /** platform data */
};

NCALLBACK void on_init(void);
NCALLBACK void on_update(struct window_t *);
NCALLBACK void on_destroy(void);

#ifdef __cplusplus
}
#endif

#endif
