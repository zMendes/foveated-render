# VRS
Exploring VRS OpenGL `GL_SHADING_RATE_IMAGE_NV` extension to enable foveated rendering.

---

## Demo

### How it works

Variable Rate Shading is enabled and defined by a Shading rate image. The rating.

![a](resources/media/view_shading.png)

- Red: 1 invocation per pixel
- Yellow: 1 invocation per 2x2 pixels
- Green: 1 invocation per 4x4 pixels

This is similar to how the eye behaves, with max accuity in the foveal center and less detail in the peripheral regions.

### Comparison


Focus on the center of the image             |  Focus on the border
:-------------------------:|:-------------------------:
![](resources/media/lion_focus.png)  |  ![](resources/media/lion_not_focus.png)
![](resources/media/wall_focus.png)  |  ![](resources/media/wall_not_focus.png)