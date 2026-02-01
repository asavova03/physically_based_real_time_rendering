# Physically Based Real-Time Rendering

This repository contains the project code and assets for a **real-time interactive 3D scene**.

## Project Overview

The project presents a procedurally generated world populated with crystal-like objects, featuring multiple camera modes, realistic animations, physically based rendering (PBR), and water effects, combining various advanced computer graphics techniques.

## Features

* **Multiple Camera Views:** Free, Character-Follow, and Bézier Curve cameras.
* **Classic Blinn–Phong Shading:** With texture wrapping and specular highlights.
* **Physically Based Rendering (PBR):** Ground materials using Cook–Torrance BRDF with full PBR texture sets.
* **Normal Mapping:** Applied to all surfaces for added detail.
* **Environment Mapping:** Day, sunset, and universe HDRI backgrounds.
* **Hierarchical Animation:** Blossom animation of flowers.
* **Character Animation:** Blender-imported walking animation with camera-relative motion.
* **Procedural World Generation:** Infinite ground tiles and gem placement with instanced rendering.
* **Water Rendering:** Gerstner waves, reflections, refractions, and PBR shading.
* **Post-Processing Effects:** Bloom, IBL, and multi-light support.
* **User Interface:** Toggles and sliders for cameras, animation, materials, water, and world generation.

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/asavova03/physically_based_real_time_rendering.git
   cd physically_based_real_time_rendering
   ```

2. Build the project using CMake or your preferred build system.

## Usage

* Run the executable to open the interactive scene.
* Navigate with keyboard and mouse controls:

  * Arrow keys: Move character
  * W/A/S/D + E/Q: Free Camera movement
  * Right mouse drag: Orbit around character
* Toggle UI panels to adjust camera, animation, materials, water, and other scene parameters.

## References

* LearnOpenGL tutorials for instancing, normal mapping, and IBL.
* Mixamo animations for character walking.
* Gerstner wave implementation inspired by research and online tutorials.
* PBR shading references from UE4 BRDF models and Physically Based Rendering lectures.

## License

This repository is for academic purposes only.
