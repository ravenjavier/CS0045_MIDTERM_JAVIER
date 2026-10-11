# Smart Traffic Congestion and Signal Simulation

## Overview

This project is an interactive 2D C++ OpenGL/GLUT simulation of a four-way intersection. Vehicles move through roads while traffic signals control their movement. Adaptive mode compares waiting vehicles and can give the busier direction a longer green period.

## Controls

- `1` - Low traffic
- `2` - Medium traffic
- `3` - Heavy traffic
- `A` - Toggle adaptive/manual mode
- `N` - Advance to the next signal state
- `SPACE` - Start or pause the simulation
- `R` - Reset the simulation
- `Left mouse click` - Select a traffic-light area in manual mode
- `ESC` - Exit

## Build

## VS Code on Windows with MinGW and FreeGLUT

1. Install VS Code and the **C/C++** extension.
2. Install MinGW-w64 or MSYS2 and make sure `g++` and `gdb` are available in the PATH.
3. Install or copy the FreeGLUT headers and libraries into the compiler installation.
4. Open the `SmartTrafficSimulation` folder in VS Code.
5. Press `Ctrl+Shift+B` and choose **Build Smart Traffic - Windows MinGW**.
6. Press `F5` to build and run the simulation.

If FreeGLUT is installed in a custom folder, add its include folder to the compiler's include path and its library folder to the linker path. The required Windows libraries are `freeglut`, `opengl32`, and `glu32`.

The included `.vscode` folder contains a Windows build task and a debugger launch configuration. Select the Linux task instead when using Linux.

### Windows with MinGW/FreeGLUT

Link against `freeglut`, `opengl32`, and `glu32`. The exact command depends on the location of your FreeGLUT headers and libraries.

## Course Concepts Demonstrated

The source demonstrates functions, variables, conditions, loops, geometric primitives, colors, GLUT callbacks, bitmap text, arrays, and a reusable vertex array rendered with `glDrawArrays(GL_QUADS, ...)`.

## Important Note

This is an educational simulation. It does not control real traffic systems and uses illustrative traffic values.
