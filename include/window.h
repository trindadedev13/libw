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

/** OpenGL Version */
/** Default: 3.2 */
#ifndef WGLMAJOR
#define WGLMAJOR 3
#endif
#ifndef WGLMINOR
#define WGLMINOR 2
#endif

/** OpenGLES Version */
/** Default: 3.2 */
#ifndef WGLESMAJOR
#define WGLESMAJOR 3
#endif
#ifndef WGLESMINOR
#define WGLESMINOR 2
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
