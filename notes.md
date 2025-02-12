# An integrative view of foveated rendering
## Foveated rendering implementation methods
### Adaptive resolution
#### Static
- Wavelet rasterization
    -   (A Theory for Multiresolution Signal Decomposition:
The Wavelet Representation
STEPHANE G. MALLAT) We explained how to extract the
difference of information between successive resolutions
and thus define a new (complete) representation called the
wavelet representation. This representation is computed
by decomposing the original signal using a wavelet orthonormal basis, and can be interpreted as a decomposition using a set of independent frequency channels having
a spatial orientation tuning.
    - (Wavelet FoveationEe-Chien Chang, 1999)

#### Dynamic

- Creating three different resolution images (Foveated 3D Graphics Brian Guenter)
- Multi-Layer Pyramid
    - partition the image into a small
set of discrete areas that are then composited
- kernel foveated rendering - log-polar coordinate

### Geometric simplification

- Adaptive Tesselation (Tiwary A, Accelerated foveated rendering based
on adaptive tessellation. 2020)