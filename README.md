# Physically Based Real-Time Rendering

This repository contains the project code and assets for a **real-time interactive 3D scene**.

<p align="center">
  <img src="https://github.com/user-attachments/assets/d1d42d15-c54a-4fa0-a166-5e64961b63a3" alt="water" height="400"/>
  <img height="400" alt="crystal_guy_no_glow" src="https://github.com/user-attachments/assets/16cb1fea-eb35-4a3d-9fc7-ac61be2725d3" />

</p>

## Project Overview

The project presents a procedurally generated world populated with crystal-like objects, featuring multiple camera modes, realistic animations, physically based rendering (PBR), and water effects, combining various advanced computer graphics techniques.

## Features

* **Classic Blinn–Phong Shading:** With texture wrapping and specular highlights.
* **Physically Based Rendering (PBR):** Ground materials using Cook–Torrance BRDF with full PBR texture sets.
<p align="center">
   <img width="223" alt="normal_mapping" src="https://github.com/user-attachments/assets/ea92d30d-0906-4044-a5a5-56ddfc20c441" />
   <img width="200" alt="height_map" src="https://github.com/user-attachments/assets/5a17a961-17ad-4d9c-b18e-3dd8bfab435c" />
   <img width="200" alt="normal_mapping_lava" src="https://github.com/user-attachments/assets/ff1e09d8-79a2-4e50-9e3d-c9a4080ac346" />
   <img width="218" alt="height_map_lava" src="https://github.com/user-attachments/assets/92568f98-09c0-4eca-bac9-2db96da79bb2" />
</p>

* **Normal and Height Mapping:** Applied to all surfaces for added detail.
* **Environment Mapping:** Day, sunset, and universe HDRI backgrounds.

<p align="center">
   <img height="200" alt="sunset" src="https://github.com/user-attachments/assets/64990adf-0d59-4cbf-b29a-d9070ac63cdc" />
   <img height="200" alt="day" src="https://github.com/user-attachments/assets/69d7a747-97c5-4fc8-865d-4d7ae7035531" />
   <img height="200" alt="universe" src="https://github.com/user-attachments/assets/45e4f0d3-94cc-4a3f-a21c-204addd9fdd3" />

</p>

* **Procedural World Generation:** Infinite ground tiles and gem placement with instanced rendering.

<p align="center">
<img height="230" alt="gem_generation" src="https://github.com/user-attachments/assets/702cd8e6-99d4-42f0-9478-65b24d26c6b9" />
<img height="230" alt="gem_generation_many" src="https://github.com/user-attachments/assets/e2c510d6-6330-4ac6-94f7-9cb9f8c7e963" />
</p>

* **Water Rendering:** Gerstner waves, reflections, refractions, and PBR shading.

<p align="center">
  <img height="300" alt="water_sunglade" src="https://github.com/user-attachments/assets/a18ddcbc-4fb1-4794-a0fa-fecf421ea7ef" />
  <img height="300" alt="water_waves_tiling" src="https://github.com/user-attachments/assets/bf875722-259e-47c7-acb0-94ef363c8d9b" />
</p>

* **Multiple Camera Views:** Free, Character-Follow, and Bézier Curve cameras.
<p align="center">
  <img src="https://github.com/user-attachments/assets/16894e73-0050-45e4-bbd4-12cd74c4fce2" alt="constant-speed-1" width="210" />
  <img src="https://github.com/user-attachments/assets/6f468520-8739-4362-aa6f-aca593494a72" alt="constant-2" width="200" />
  <img src="https://github.com/user-attachments/assets/c58b73e9-4ce5-4143-b2b0-8fdb0dacfd33" alt="bezier-birdseye" width="220" />
</p>

* **Character Animation:** Blender-imported walking animation with camera-relative motion.

<p align="center">
  <img height="300" alt="character_walking" src="https://github.com/user-attachments/assets/2d122439-b5c3-4d87-bc0b-259047e08a8c" />
  <img height="300" alt="character_walking_side" src="https://github.com/user-attachments/assets/14a42650-437b-4716-a78e-226a2ef3a997" />
</p>

* **Hierarchical Animation:** Blossom animation of flowers.

<p align="center">
<img height="200" alt="flower-close" src="https://github.com/user-attachments/assets/612611d3-3082-4457-b219-0d7bf0bac4ab" />
<img height="200" alt="flower-open" src="https://github.com/user-attachments/assets/2f153378-0f56-4815-8313-ee2d1b56a6a6" />
<img height="200" alt="up-mid-particle" src="https://github.com/user-attachments/assets/c6dc790b-431d-4519-abe7-ffda6c053ce6" />
<img height="200" alt="up-particle-big" src="https://github.com/user-attachments/assets/647f9d1d-bcb3-44e6-ae9f-c8cc10bcb7c7" />

</p>

* **Post-Processing Effects:** Bloom and IBL.

<p align="center">
  <img height="250" alt="bloom_sunset" src="https://github.com/user-attachments/assets/89aeb56d-98a4-4935-9b5a-16d2522c280d" />
  <img height="250" alt="bloom_universe" src="https://github.com/user-attachments/assets/b92a2b0b-21ae-46ab-afc6-1907f15e222e" />
</p>

* **Multi-light support**

<p align="center">
  <img height="300" alt="multiple_lights_crystal" src="https://github.com/user-attachments/assets/2d397876-c8c3-4874-a7fa-0dd31a78706c" /> 
  <img height="300" alt="multiple_lights_ice" src="https://github.com/user-attachments/assets/2f6de983-456c-41e9-904a-879ee17c3787" />

</p>

* **Shader for Refractive Translucent Materials** used for the player

<p align="center">
<img height="300" alt="crystal_guy" src="https://github.com/user-attachments/assets/f2a9ef39-4cce-4970-9ac5-3b0a309b544a" />
<img height="300" alt="crystal_guy_high_ior" src="https://github.com/user-attachments/assets/0fe4de69-f440-4b84-8331-14eee5bb335b" />
<img height="300" alt="crystal_guy_translucent" src="https://github.com/user-attachments/assets/e7b775ec-0406-4ade-a584-6213f2ed5815" />
<img height="300" alt="crystal_guy_low_ior" src="https://github.com/user-attachments/assets/0c2781ef-e0c9-481e-9581-8eded837a2f3" />

</p>

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
