#include <window.h>
#include <glad/glad.h>

void on_init(void)
{
}

void on_update(struct window_t *window)
{
        if (!window) return;

        glViewport(0, 0, window->width, window->height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(1, 0, 0, 1);
}

void on_destroy(void)
{
}
