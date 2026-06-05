


<h1 align="center">
   <img src="Logo/XULTAIK-2.png" width=410>

  
  ##              🎮 Xultaik is a 3D Game Engine implemented in C++ and [Direct3D 12®](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)

  
</h1>


[![Build status](https://github.com/FaberSanZ/Zeckoxe/workflows/ci/badge.svg)](https://github.com/FaberSanZ/Xultaik/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/FaberSanZ/Xultaik/blob/master/LICENSE) 

<br>

> [!IMPORTANT]
> The engine is being rewritten from scratch in C++.


<br>

## 📝 Overview
Welcome to Xultaik, the game engine based centrally on Direct3D 12 and written in C++. It is a toy engine I use both to experiment with graphical techniques and to build my own games.

The code is licensed by MIT. Feel free to use it for any purpose.

<br>

## Key Features 🚀

**Cutting-Edge Graphics with Direct3D 12**:
Xultaik maximizes Direct3D 12 capabilities, a low-level graphics API that offers direct GPU control. Built exclusively for Direct3D 12, Xultaik delivers superior performance and full access to advanced API features. It implements the following extensions and key features:

- [ ]  **Multi-Threading**: Direct3D 12 is designed to leverage multi-core processors, allowing simultaneous execution of multiple graphics tasks. This translates to significantly improved performance and more efficient utilization of system resources.
- [ ]  **Multi-GPU**: Direct3D 12 natively supports multi-GPU configurations, enabling developers to distribute graphics workload across multiple graphics cards. This not only enhances performance but also allows for the creation of more complex and detailed graphics.
- [ ]  **Ray Tracing**: Enables advanced lighting and shading effects for realistic graphics and immersive visual experiences. Direct3D 12 Ray Tracing supports both hardware acceleration and software-based implementation, providing flexibility and performance.
- [ ]  **Variable Rate Shading**: Enhances performance efficiency by adjusting shading rate based on scene complexity, maintaining visual quality. This technique allows developers to optimize GPU usage and improve frame rates without sacrificing important details.
- [ ]  **Mesh Shaders**: Facilitates efficient rendering of complex models by allowing geometry manipulation at the shader level. Direct3D 12 Mesh Shaders provide greater flexibility and control in geometry rendering, enhancing both performance and visual quality.
- [ ]  **Video**: Allows playback and manipulation of high-quality video content directly on the GPU, seamlessly integrating with the engine's graphic capabilities. This is ideal for games incorporating cinematics or interactive videos.
- [ ]  **Sampler Feedback**: Texture Optimization: This feature allows for more efficient handling of textures, improving performance by using only the necessary parts of a texture during rendering.
- [ ]  **¿Work Graphs?**:


<br>

**Immersive Audio with Steam Audio and XAudio**:
Xultaik utilizes Steam Audio for advanced spatialized audio effects. Steam Audio offers a range of features that significantly enhance the game's auditory experience:

- [ ]  **Spatialized Audio**: Allows precise placement of sound effects in three-dimensional space, enhancing player immersion.
- [ ]  **Occlusion**: Simulates how objects in the environment block or dampen sounds, providing a more realistic auditory experience. For example, a sound coming from an adjacent room will sound more muffled if there's a wall between the player and the sound source.
- [ ]  **Reverberation**: Emulates how sound reflects off different surfaces and spaces, creating an authentic sonic environment. This includes everything from reverberation in large cathedrals to echo in small rooms.
- [ ]  **HRTF (Head-Related Transfer Function)**: Enhances 3D sound perception, allowing players to accurately locate where sounds are coming from.
- [ ]  **Sound Reflections**: Calculates how sounds bounce off surfaces in the environment, adding an additional layer of realism. For example, the sound of a gunshot in a narrow alley will have multiple reflections and echoes.
- [ ]  **Support for Virtual Reality**: Optimizes the audio experience for virtual reality devices, ensuring that sounds react accurately to the player's head movements.

<br>

**Intuitive and Customizable Editor**:
Xultaik's editor provides an intuitive graphical interface for real-time manipulation of graphics, audio, physics, and more. It facilitates adjustment and optimization of all aspects of the game during development.

- [ ]  **Real-Time Manipulation**
- [ ]  **Advanced Audio Tools**
- [ ]  **Physics Manipulation**
- [ ]  **Shader Editor**
- [ ]  **Advanced Scripting System**
- [ ]  **User Interface Manipulation**

<br>

**Realistic Physics with JoltPhysics**:
Integrated to deliver dynamic and precise real-time physics simulations, JoltPhysics ensures realistic interactions between objects in the game, from collisions to complex behaviors. This physics engine is highly optimized to work efficiently alongside Direct3D 12.

- [ ]  **Advanced Collision Simulation**: JoltPhysics offers precise and fast collision detection, ensuring that physical interactions in the game are realistic and error-free. This includes the ability to handle complex collisions between multiple dynamic and static objects.
- [ ]  **Rigid and Soft Body Dynamics**: Supports both rigid and soft bodies, allowing simulation of a wide range of materials and physical behaviors. This includes simulation of objects such as rocks, liquids, fabrics, and more.
- [ ]  **Multi-Threading Optimization**: JoltPhysics is designed to leverage multi-core architectures to the fullest, allowing concurrent physics simulations that significantly enhance performance. This ensures that complex physics do not compromise overall game performance.
- [ ]  **Support for Special Effects**: JoltPhysics facilitates creation of realistic special effects, such as explosions, object destruction, and particle simulation, adding an additional layer of immersion and dynamism to the game.

<br>

**Advanced Texture Optimization**:
Xultaik optimizes loading and manipulation of textures using formats like DDS, specifically designed to maximize graphic performance without compromising visual quality. DDS is ideal for handling compressed textures essential for high-quality real-time rendering.
- [x]  **WIC**
- [x]  **DDS**
<br>

**Xultaik** uses native Windows and Direct3D 12 APIs in C++.
