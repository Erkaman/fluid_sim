# fluid_sim - Some flashy 2D fluid simulations

![Animated](gifs/sim1.gif)
![Animated](gifs/sim2.gif)
![Animated](gifs/sim3.gif)
![Animated](gifs/sim4.gif)

An implementation of the article "Fast Fluid Dynamics Simulation on the GPU", using
C++ and OpenGL 3.3. This implementation implements the "advection", and "projection" steps from the
article. The "diffusion" and "boundary conditions" steps of the article are not implemented.
Finally, note that the primary focus was on making flashy simulations, and not on physical realism.

**The code is minimalistic and is written in only ~1000LOC of C++, and uses only OpenGL and no frameworks whatsoever,
so the code should be readable.**

# Video

Longer video of fluid simulations:

[![Result](http://img.youtube.com/vi/vB2svwmjGUg/0.jpg)](https://www.youtube.com/watch?v=vB2svwmjGUg)

## Building

The only dependencies are [GLFW](https://github.com/glfw/glfw) and [stb_image](https://github.com/nothings/stb).
Both of which are included within this repository.

We use CMake for building. If on Linux or OS X, you can build it in the terminal by doing something like:

```
mkdir build && cd build && cmake .. && make
```

If on Windows, create a `build/` folder, and run `cmake ..` from
inside that folder. This will create a visual studio solution(if you
have visual studio). Launch that solution, and then simply compile the
project named `fluid_sim`.








