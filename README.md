# OpenGL Thing Shooter

This project is a simple 3D shooting game developed using C++ and the OpenGL graphics library. It serves as a basic example of how to load and render 3D models, implement camera controls, and handle user input for a first-person perspective game.

## Features

*   **3D Model Loading:** Loads and displays 3D models in the `.3ds` file format.
*   **First-Person Camera:** Implements a camera system with controls for movement (forward, backward, strafe left/right) and mouse-look for aiming.
*   **Shooting Mechanic:** A basic shooting mechanic allows the player to fire projectiles.
*   **Sound Effects:** Includes simple sound effects for actions like shooting, hitting a target, and reloading.
*   **Basic Scene:** Renders a simple 3D environment with a skybox and various objects.

## Dependencies

To compile and run this project, you will need the following libraries:

*   **GLEW (OpenGL Extension Wrangler Library):** Used to manage OpenGL extensions.
*   **GLUT (OpenGL Utility Toolkit):** Provides a simple windowing and input API.
*   **GLAux (OpenGL Auxiliary Library):** A legacy library used for some helper functions.

All required `.h`, `.lib`, and `.dll` files are included in the project repository.

## How to Run

The project is set up as a Visual Studio solution.

1.  Open the `OpenGLMeshLoader.sln` file in Visual Studio.
2.  Build the solution (usually by pressing `F7` or `Ctrl+Shift+B`).
3.  Run the application (usually by pressing `F5`).

The executable `OpenGLMeshLoader.exe` will be generated in the `Debug` directory.

## Controls

*   **W, A, S, D:** Move the camera forward, left, backward, and right.
*   **Mouse:** Look around and aim.
*   **Left Mouse Button:** Shoot.
*   **R:** Reload.

## Assets

The project includes various 3D models, textures, and sound files located in the following directories:

*   `models/`: Contains `.3ds` models for objects like targets, furniture, and environmental props.
*   `textures/`: Contains `.bmp` image files used for texturing the models and the environment.
*   `Sounds/`: Contains `.wav` files for the game's sound effects.
