# Basic First-Person Shooter Game

A simple first-person shooter game implemented in C++ without using external libraries. The game uses ASCII characters for rendering and runs in both Windows console and Linux/WSL2 terminals.

## Features

- 3D rendering using raycasting technique
- First-person perspective
- Player movement (WASD keys)
- Camera rotation (arrow keys on Windows, Q/E on Linux/WSL2)
- Shooting mechanics (spacebar)
- Simple enemies
- Collision detection
- Mini-map display
- Cross-platform support (Windows and Linux/WSL2)
- Adaptive rendering based on terminal size (Linux/WSL2)

## Requirements

- C++ compiler with C++11 support (e.g., Visual Studio, MinGW, GCC)
- For Windows: Windows operating system
- For Linux/WSL2: Terminal with ANSI escape sequence support

## How to Compile

### On Windows

#### Using Visual Studio

1. Open Visual Studio
2. Create a new C++ Console Application project
3. Replace the generated code with the contents of `main.cpp`
4. Build the solution (F7 or Ctrl+Shift+B)
5. Run the program (F5)

#### Using MinGW (g++)

1. Open Command Prompt
2. Navigate to the directory containing `main.cpp`
3. Compile the code:
   ```
   g++ -o fps_game main.cpp -std=c++11
   ```
4. Run the program:
   ```
   fps_game
   ```

### On Linux/WSL2

1. Open Terminal
2. Navigate to the directory containing `main.cpp`
3. Compile the code:
   ```
   g++ -o fps_game main.cpp -std=c++11
   ```
4. Run the program:
   ```
   ./fps_game
   ```

## Controls

### Windows Controls
- **W**: Move forward
- **S**: Move backward
- **A**: Strafe left
- **D**: Strafe right
- **Left Arrow**: Rotate camera left
- **Right Arrow**: Rotate camera right
- **Spacebar**: Shoot
- **ESC**: Exit game

### Linux/WSL2 Controls
- **W**: Move forward
- **S**: Move backward
- **A**: Strafe left
- **D**: Strafe right
- **Q**: Rotate camera left
- **E**: Rotate camera right
- **Spacebar**: Shoot
- **ESC**: Exit game

## Game Elements

- **Walls**: Displayed with different shading based on distance
- **Enemies**: Represented by 'E' characters
- **Bullets**: Represented by '*' characters
- **Player**: Represented by 'P' on the mini-map
- **Crosshair**: '+' in the center of the screen

## How the Game Works

The game uses a raycasting technique to create a 3D-like environment from a 2D map. For each column of the screen, a ray is cast from the player's position in the direction they are facing. The distance to the nearest wall is calculated, and this distance determines the height of the wall column drawn on the screen.

The game runs in a continuous loop that:
1. Handles player input
2. Updates game state (player position, bullets, enemies)
3. Renders the scene
4. Displays the frame

## Platform-Specific Implementation

The game uses conditional compilation to support both Windows and Linux/WSL2:

- On Windows, it uses the Windows Console API for rendering and input handling
- On Linux/WSL2, it uses ANSI escape sequences for rendering and terminal input handling

## Troubleshooting

### Windows Issues
1. Make sure you're using a C++11 compatible compiler (add `-std=c++11` flag)
2. If you get errors about missing headers, check that your compiler environment is properly set up
3. If the game runs too fast or too slow, adjust the `TARGET_FPS` constant in the code

### Linux/WSL2 Issues
1. If the terminal display is garbled:
   - Make sure your terminal supports ANSI escape sequences
   - Try resizing your terminal window to be larger
   - Check if your terminal font supports the characters being used
   - Try running in a different terminal emulator (like xterm, gnome-terminal, or Windows Terminal)

2. If the game runs too fast or too slow:
   - The game is set to run at 30 FPS by default. You can adjust the `TARGET_FPS` constant in the code
   - Some terminals may have performance issues with rapid screen updates

3. If input doesn't work properly:
   - Make sure you're using a terminal that properly supports raw input mode
   - Try running the game in a different terminal emulator
   - If using WSL2, make sure you're running in a proper terminal and not redirecting output

4. If the screen size is incorrect:
   - The game now automatically adapts to your terminal size
   - For best results, use a terminal with at least 120x40 characters
   - If your terminal is smaller, the game will scale down to fit

5. If you see excessive `=` characters or other display artifacts:
   - This has been fixed in the latest version by improving the rendering method
   - If you still see issues, try using a different terminal or font

## Extending the Game

You can extend the game by:
- Adding more enemy types
- Implementing different weapons
- Creating larger and more complex maps
- Adding textures to walls
- Implementing a scoring system
- Adding sound effects
- Improving the cross-platform rendering

## License

This project is open source and available for anyone to use and modify. 