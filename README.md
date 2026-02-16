# LibW
  LibW - A simple, lightweight, and fast-to-use window library.

# Usage
  LibW works with callback functions, so we have `on_init`, `on_update`, `on_destroy`:
  
  ```c
  #include <window.h>
  
  void on_init(void)
  {
          /** logic called when display inits */
          /** NOTE: when you're in android, it can be called everytime it resumes (android lifecycle onResume). */
  }

  void on_update(struct window_t *window)
  {
          /** called every frame */
          /** you can draw here */
  }

  void on_destroy(void)
  {
          /** called when window closes and application ends */
  }
  ```

# More About the API
  LibW aim is just to provide the Window layer, so there's not additional features.  
  On the other hand, LibW already initializes an OpenGL(ES) context, ES for Android.  
  it uses OpenGL(ES) 3.2 by the default  
  you can change it by redefining the following macros before include window.h:  
    - `WGLMAJOR`:   3  
    - `WGLMINOR`:   2  
    - `WGLESMAJOR`: 3  
    - `WGLESMINOR`: 2  

  `NOTE`: LibW doesn't load OpenGL Functions for the version, so if you want use any version after GL 1.0 you should use GLAD.