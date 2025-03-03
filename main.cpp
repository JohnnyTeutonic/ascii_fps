#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>
#include <sstream>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
#else
    #define PLATFORM_UNIX
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
#endif

// Screen dimensions
const int SCREEN_WIDTH = 120;
const int SCREEN_HEIGHT = 40;

// Player properties
float playerX = 8.0f;
float playerY = 8.0f;
float playerA = 0.0f; // Player angle
float playerFOV = 3.14159f / 4.0f; // Field of view
float playerSpeed = 5.0f; // Movement speed
float playerRotSpeed = 3.14159f; // Rotation speed

// Map dimensions
const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 16;

// Game map (1 = wall, 0 = empty space)
std::string map;

// Bullets
struct Bullet {
    float x, y;
    float dx, dy;
    bool active;
    
    Bullet() : x(0), y(0), dx(0), dy(0), active(false) {}
};

std::vector<Bullet> bullets;
const int MAX_BULLETS = 10;
float bulletSpeed = 10.0f;

// Enemies
struct Enemy {
    float x, y;
    bool alive;
    
    Enemy(float _x, float _y) : x(_x), y(_y), alive(true) {}
};

std::vector<Enemy> enemies;

// Platform-specific keyboard input handling
#ifdef PLATFORM_UNIX
// Terminal mode settings
struct termios orig_termios;

// Function to initialize terminal for raw input
void initTerminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    // Hide cursor
    std::cout << "\033[?25l";
}

// Function to restore terminal settings
void restoreTerminal() {
    // Show cursor
    std::cout << "\033[?25h";
    
    // Reset terminal
    std::cout << "\033[0m";
    std::cout << "\033[2J\033[H";
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Function to check if a key is pressed
bool isKeyPressed(char key) {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        return c == key;
    }
    return false;
}

// Function to clear screen
void clearScreen() {
    // Clear screen and move cursor to home position
    std::cout << "\033[2J\033[H";
}

// Function to get terminal size
void getTerminalSize(int& width, int& height) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;
}
#endif

// Function to initialize the game
void initGame() {
    // Initialize map
    map += "################";
    map += "#..............#";
    map += "#........#.....#";
    map += "#........#.....#";
    map += "#..............#";
    map += "#.......####...#";
    map += "#..............#";
    map += "#..............#";
    map += "#..............#";
    map += "#..............#";
    map += "#......##......#";
    map += "#......##......#";
    map += "#..............#";
    map += "#..............#";
    map += "#..............#";
    map += "################";
    
    // Initialize bullets
    bullets.resize(MAX_BULLETS);
    
    // Initialize enemies
    enemies.push_back(Enemy(10.0f, 10.0f));
    enemies.push_back(Enemy(5.0f, 5.0f));
    enemies.push_back(Enemy(12.0f, 3.0f));
    
    #ifdef PLATFORM_UNIX
    initTerminal();
    #endif
}

// Function to shoot a bullet
void shootBullet() {
    for (auto& bullet : bullets) {
        if (!bullet.active) {
            bullet.x = playerX;
            bullet.y = playerY;
            bullet.dx = sin(playerA) * bulletSpeed;
            bullet.dy = cos(playerA) * bulletSpeed;
            bullet.active = true;
            break;
        }
    }
}

// Function to update bullets
void updateBullets(float elapsedTime) {
    for (auto& bullet : bullets) {
        if (bullet.active) {
            // Update position
            bullet.x += bullet.dx * elapsedTime;
            bullet.y += bullet.dy * elapsedTime;
            
            // Check for collision with walls
            int mapX = static_cast<int>(bullet.x);
            int mapY = static_cast<int>(bullet.y);
            
            if (map[mapY * MAP_WIDTH + mapX] == '#') {
                bullet.active = false;
                continue;
            }
            
            // Check for collision with enemies
            for (auto& enemy : enemies) {
                if (enemy.alive) {
                    float distance = sqrt((bullet.x - enemy.x) * (bullet.x - enemy.x) + 
                                         (bullet.y - enemy.y) * (bullet.y - enemy.y));
                    if (distance < 0.5f) {
                        enemy.alive = false;
                        bullet.active = false;
                        break;
                    }
                }
            }
            
            // Check if bullet is out of bounds
            if (bullet.x < 0 || bullet.x >= MAP_WIDTH || bullet.y < 0 || bullet.y >= MAP_HEIGHT) {
                bullet.active = false;
            }
        }
    }
}

#ifdef PLATFORM_WINDOWS
// Windows-specific rendering function
void render(wchar_t* screen) {
    // Clear screen
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screen[i] = ' ';
    }
    
    // Ray casting for 3D walls
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Calculate ray position and direction
        float rayAngle = (playerA - playerFOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * playerFOV;
        
        float rayDirX = sin(rayAngle);
        float rayDirY = cos(rayAngle);
        
        // Distance to wall
        float distanceToWall = 0.0f;
        bool hitWall = false;
        
        // Step size
        float stepSize = 0.1f;
        
        // Ray position
        float rayX = playerX;
        float rayY = playerY;
        
        while (!hitWall && distanceToWall < 16.0f) {
            distanceToWall += stepSize;
            rayX = playerX + rayDirX * distanceToWall;
            rayY = playerY + rayDirY * distanceToWall;
            
            // Check if ray is out of bounds
            if (rayX < 0 || rayX >= MAP_WIDTH || rayY < 0 || rayY >= MAP_HEIGHT) {
                hitWall = true;
                distanceToWall = 16.0f;
            }
            else {
                // Check if ray hit a wall
                if (map[(int)rayY * MAP_WIDTH + (int)rayX] == '#') {
                    hitWall = true;
                }
            }
        }
        
        // Calculate wall height
        int ceiling = (float)(SCREEN_HEIGHT / 2.0) - SCREEN_HEIGHT / ((float)distanceToWall);
        int floor = SCREEN_HEIGHT - ceiling;
        
        // Shade walls based on distance
        wchar_t wallShade;
        if (distanceToWall <= 1.0f) wallShade = 0x2588; // Very close
        else if (distanceToWall < 2.0f) wallShade = 0x2593;
        else if (distanceToWall < 4.0f) wallShade = 0x2592;
        else if (distanceToWall < 8.0f) wallShade = 0x2591;
        else wallShade = ' '; // Too far away
        
        // Draw walls
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            if (y < ceiling)
                screen[y * SCREEN_WIDTH + x] = ' ';
            else if (y >= ceiling && y <= floor)
                screen[y * SCREEN_WIDTH + x] = wallShade;
            else {
                // Shade floor based on distance
                float b = 1.0f - (((float)y - SCREEN_HEIGHT / 2.0f) / ((float)SCREEN_HEIGHT / 2.0f));
                if (b < 0.25) screen[y * SCREEN_WIDTH + x] = '#';
                else if (b < 0.5) screen[y * SCREEN_WIDTH + x] = 'x';
                else if (b < 0.75) screen[y * SCREEN_WIDTH + x] = '.';
                else if (b < 0.9) screen[y * SCREEN_WIDTH + x] = '-';
                else screen[y * SCREEN_WIDTH + x] = ' ';
            }
        }
    }
    
    // Draw enemies
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            // Calculate angle to enemy
            float enemyAngle = atan2(enemy.y - playerY, enemy.x - playerX);
            
            // Adjust angle to player's perspective
            while (enemyAngle - playerA > 3.14159f) enemyAngle -= 2.0f * 3.14159f;
            while (enemyAngle - playerA < -3.14159f) enemyAngle += 2.0f * 3.14159f;
            
            // Check if enemy is in field of view
            bool inFOV = fabs(enemyAngle - playerA) < playerFOV / 2.0f;
            
            if (inFOV) {
                // Calculate distance to enemy
                float distance = sqrt((enemy.x - playerX) * (enemy.x - playerX) + 
                                     (enemy.y - playerY) * (enemy.y - playerY));
                
                // Calculate enemy height and position on screen
                int enemyHeight = (int)(SCREEN_HEIGHT / distance);
                int enemyCenter = (int)((enemyAngle - playerA + playerFOV / 2.0f) / playerFOV * SCREEN_WIDTH);
                
                // Draw enemy
                for (int y = 0; y < enemyHeight && y < SCREEN_HEIGHT; y++) {
                    for (int x = 0; x < enemyHeight / 2 && x < SCREEN_WIDTH; x++) {
                        int drawY = SCREEN_HEIGHT / 2 - enemyHeight / 2 + y;
                        int drawX = enemyCenter - enemyHeight / 4 + x;
                        
                        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
                            screen[drawY * SCREEN_WIDTH + drawX] = 'E';
                        }
                    }
                }
            }
        }
    }
    
    // Draw bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            // Calculate angle to bullet
            float bulletAngle = atan2(bullet.y - playerY, bullet.x - playerX);
            
            // Adjust angle to player's perspective
            while (bulletAngle - playerA > 3.14159f) bulletAngle -= 2.0f * 3.14159f;
            while (bulletAngle - playerA < -3.14159f) bulletAngle += 2.0f * 3.14159f;
            
            // Check if bullet is in field of view
            bool inFOV = fabs(bulletAngle - playerA) < playerFOV / 2.0f;
            
            if (inFOV) {
                // Calculate distance to bullet
                float distance = sqrt((bullet.x - playerX) * (bullet.x - playerX) + 
                                     (bullet.y - playerY) * (bullet.y - playerY));
                
                // Calculate bullet position on screen
                int bulletCenter = (int)((bulletAngle - playerA + playerFOV / 2.0f) / playerFOV * SCREEN_WIDTH);
                int bulletY = SCREEN_HEIGHT / 2;
                
                // Draw bullet
                if (bulletCenter >= 0 && bulletCenter < SCREEN_WIDTH) {
                    screen[bulletY * SCREEN_WIDTH + bulletCenter] = '*';
                }
            }
        }
    }
    
    // Draw HUD
    // Draw crosshair
    screen[(SCREEN_HEIGHT / 2) * SCREEN_WIDTH + (SCREEN_WIDTH / 2)] = '+';
    
    // Draw stats
    std::stringstream ss;
    int aliveEnemies = 0;
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            aliveEnemies++;
        }
    }
    ss << "FPS: X | Enemies: " << aliveEnemies;
    std::string stats = ss.str();
    
    for (size_t i = 0; i < stats.size(); i++) {
        screen[i] = stats[i];
    }
    
    // Draw mini-map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            screen[(y + 1) * SCREEN_WIDTH + (SCREEN_WIDTH - MAP_WIDTH - 1) + x] = map[y * MAP_WIDTH + x];
        }
    }
    
    // Draw player on mini-map
    screen[(int)(playerY + 1) * SCREEN_WIDTH + (SCREEN_WIDTH - MAP_WIDTH - 1) + (int)playerX] = 'P';
}
#else
// Unix-specific rendering function
void render() {
    // Get terminal size
    int termWidth, termHeight;
    getTerminalSize(termWidth, termHeight);
    
    // Adjust screen dimensions if terminal is too small
    int renderWidth = std::min(SCREEN_WIDTH, termWidth);
    int renderHeight = std::min(SCREEN_HEIGHT, termHeight);
    
    // Create a buffer for the screen
    std::vector<std::string> screenLines(renderHeight);
    for (int y = 0; y < renderHeight; y++) {
        screenLines[y].resize(renderWidth, ' ');
    }
    
    // Ray casting for 3D walls
    for (int x = 0; x < renderWidth; x++) {
        // Calculate ray position and direction
        float rayAngle = (playerA - playerFOV / 2.0f) + ((float)x / (float)renderWidth) * playerFOV;
        
        float rayDirX = sin(rayAngle);
        float rayDirY = cos(rayAngle);
        
        // Distance to wall
        float distanceToWall = 0.0f;
        bool hitWall = false;
        
        // Step size
        float stepSize = 0.1f;
        
        // Ray position
        float rayX = playerX;
        float rayY = playerY;
        
        while (!hitWall && distanceToWall < 16.0f) {
            distanceToWall += stepSize;
            rayX = playerX + rayDirX * distanceToWall;
            rayY = playerY + rayDirY * distanceToWall;
            
            // Check if ray is out of bounds
            if (rayX < 0 || rayX >= MAP_WIDTH || rayY < 0 || rayY >= MAP_HEIGHT) {
                hitWall = true;
                distanceToWall = 16.0f;
            }
            else {
                // Check if ray hit a wall
                if (map[(int)rayY * MAP_WIDTH + (int)rayX] == '#') {
                    hitWall = true;
                }
            }
        }
        
        // Calculate wall height
        int ceiling = (float)(renderHeight / 2.0) - renderHeight / ((float)distanceToWall);
        int floor = renderHeight - ceiling;
        
        // Shade walls based on distance
        char wallShade;
        if (distanceToWall <= 1.0f) wallShade = '#'; // Very close
        else if (distanceToWall < 2.0f) wallShade = 'H';
        else if (distanceToWall < 4.0f) wallShade = '=';
        else if (distanceToWall < 8.0f) wallShade = '-';
        else wallShade = ' '; // Too far away
        
        // Draw walls
        for (int y = 0; y < renderHeight; y++) {
            if (y < ceiling)
                screenLines[y][x] = ' ';
            else if (y >= ceiling && y <= floor)
                screenLines[y][x] = wallShade;
            else {
                // Shade floor based on distance
                float b = 1.0f - (((float)y - renderHeight / 2.0f) / ((float)renderHeight / 2.0f));
                if (b < 0.25) screenLines[y][x] = '#';
                else if (b < 0.5) screenLines[y][x] = 'x';
                else if (b < 0.75) screenLines[y][x] = '.';
                else if (b < 0.9) screenLines[y][x] = '-';
                else screenLines[y][x] = ' ';
            }
        }
    }
    
    // Draw enemies
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            // Calculate angle to enemy
            float enemyAngle = atan2(enemy.y - playerY, enemy.x - playerX);
            
            // Adjust angle to player's perspective
            while (enemyAngle - playerA > 3.14159f) enemyAngle -= 2.0f * 3.14159f;
            while (enemyAngle - playerA < -3.14159f) enemyAngle += 2.0f * 3.14159f;
            
            // Check if enemy is in field of view
            bool inFOV = fabs(enemyAngle - playerA) < playerFOV / 2.0f;
            
            if (inFOV) {
                // Calculate distance to enemy
                float distance = sqrt((enemy.x - playerX) * (enemy.x - playerX) + 
                                     (enemy.y - playerY) * (enemy.y - playerY));
                
                // Calculate enemy height and position on screen
                int enemyHeight = (int)(renderHeight / distance);
                int enemyCenter = (int)((enemyAngle - playerA + playerFOV / 2.0f) / playerFOV * renderWidth);
                
                // Draw enemy
                for (int y = 0; y < enemyHeight && y < renderHeight; y++) {
                    for (int x = 0; x < enemyHeight / 2 && x < renderWidth; x++) {
                        int drawY = renderHeight / 2 - enemyHeight / 2 + y;
                        int drawX = enemyCenter - enemyHeight / 4 + x;
                        
                        if (drawX >= 0 && drawX < renderWidth && drawY >= 0 && drawY < renderHeight) {
                            screenLines[drawY][drawX] = 'E';
                        }
                    }
                }
            }
        }
    }
    
    // Draw bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            // Calculate angle to bullet
            float bulletAngle = atan2(bullet.y - playerY, bullet.x - playerX);
            
            // Adjust angle to player's perspective
            while (bulletAngle - playerA > 3.14159f) bulletAngle -= 2.0f * 3.14159f;
            while (bulletAngle - playerA < -3.14159f) bulletAngle += 2.0f * 3.14159f;
            
            // Check if bullet is in field of view
            bool inFOV = fabs(bulletAngle - playerA) < playerFOV / 2.0f;
            
            if (inFOV) {
                // Calculate distance to bullet
                float distance = sqrt((bullet.x - playerX) * (bullet.x - playerX) + 
                                     (bullet.y - playerY) * (bullet.y - playerY));
                
                // Calculate bullet position on screen
                int bulletCenter = (int)((bulletAngle - playerA + playerFOV / 2.0f) / playerFOV * renderWidth);
                int bulletY = renderHeight / 2;
                
                // Draw bullet
                if (bulletCenter >= 0 && bulletCenter < renderWidth) {
                    screenLines[bulletY][bulletCenter] = '*';
                }
            }
        }
    }
    
    // Draw HUD
    // Draw crosshair
    if (renderHeight / 2 < renderHeight && renderWidth / 2 < renderWidth) {
        screenLines[renderHeight / 2][renderWidth / 2] = '+';
    }
    
    // Draw stats
    std::stringstream ss;
    int aliveEnemies = 0;
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            aliveEnemies++;
        }
    }
    ss << "FPS: X | Enemies: " << aliveEnemies;
    std::string stats = ss.str();
    
    for (size_t i = 0; i < stats.size() && i < renderWidth; i++) {
        screenLines[0][i] = stats[i];
    }
    
    // Draw mini-map if there's enough space
    int mapStartX = renderWidth - MAP_WIDTH - 1;
    if (mapStartX > 0 && MAP_HEIGHT + 1 < renderHeight) {
        for (int y = 0; y < MAP_HEIGHT && y + 1 < renderHeight; y++) {
            for (int x = 0; x < MAP_WIDTH && mapStartX + x < renderWidth; x++) {
                screenLines[y + 1][mapStartX + x] = map[y * MAP_WIDTH + x];
            }
        }
        
        // Draw player on mini-map
        int playerMapY = (int)(playerY) + 1;
        int playerMapX = mapStartX + (int)playerX;
        if (playerMapY < renderHeight && playerMapX < renderWidth && playerMapY >= 0 && playerMapX >= 0) {
            screenLines[playerMapY][playerMapX] = 'P';
        }
    }
    
    // Clear screen and render
    clearScreen();
    
    // Output the screen buffer
    for (int y = 0; y < renderHeight; y++) {
        std::cout << screenLines[y] << std::endl;
    }
    
    // Flush output to ensure it's displayed immediately
    std::cout.flush();
}
#endif

int main() {
    // Initialize game
    initGame();
    
#ifdef PLATFORM_WINDOWS
    // Create screen buffer
    wchar_t* screen = new wchar_t[SCREEN_WIDTH * SCREEN_HEIGHT];
    HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    
    // Check if console creation was successful
    if (console == INVALID_HANDLE_VALUE) {
        std::cerr << "Error creating console buffer: " << GetLastError() << std::endl;
        delete[] screen;
        return 1;
    }
    
    SetConsoleActiveScreenBuffer(console);
    DWORD bytesWritten = 0;
#endif
    
    // Game loop variables
    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();
    
    // Frame rate control
    const int TARGET_FPS = 30;
    const std::chrono::milliseconds FRAME_DURATION(1000 / TARGET_FPS);
    
    // Game loop
    bool gameRunning = true;
    while (gameRunning) {
        // Start frame timing
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Calculate elapsed time
        tp2 = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();
        
#ifdef PLATFORM_WINDOWS
        // Handle input
        if (GetAsyncKeyState('W') & 0x8000) {
            playerX += sin(playerA) * playerSpeed * fElapsedTime;
            playerY += cos(playerA) * playerSpeed * fElapsedTime;
            
            // Collision detection
            if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                playerX -= sin(playerA) * playerSpeed * fElapsedTime;
                playerY -= cos(playerA) * playerSpeed * fElapsedTime;
            }
        }
        
        if (GetAsyncKeyState('S') & 0x8000) {
            playerX -= sin(playerA) * playerSpeed * fElapsedTime;
            playerY -= cos(playerA) * playerSpeed * fElapsedTime;
            
            // Collision detection
            if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                playerX += sin(playerA) * playerSpeed * fElapsedTime;
                playerY += cos(playerA) * playerSpeed * fElapsedTime;
            }
        }
        
        if (GetAsyncKeyState('A') & 0x8000) {
            playerX -= cos(playerA) * playerSpeed * fElapsedTime;
            playerY += sin(playerA) * playerSpeed * fElapsedTime;
            
            // Collision detection
            if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                playerX += cos(playerA) * playerSpeed * fElapsedTime;
                playerY -= sin(playerA) * playerSpeed * fElapsedTime;
            }
        }
        
        if (GetAsyncKeyState('D') & 0x8000) {
            playerX += cos(playerA) * playerSpeed * fElapsedTime;
            playerY -= sin(playerA) * playerSpeed * fElapsedTime;
            
            // Collision detection
            if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                playerX -= cos(playerA) * playerSpeed * fElapsedTime;
                playerY += sin(playerA) * playerSpeed * fElapsedTime;
            }
        }
        
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
            playerA -= playerRotSpeed * fElapsedTime;
        }
        
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            playerA += playerRotSpeed * fElapsedTime;
        }
        
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            static bool spacePressed = false;
            if (!spacePressed) {
                shootBullet();
                spacePressed = true;
            }
            else {
                spacePressed = false;
            }
        }
        
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            gameRunning = false;
        }
#else
        // Handle input for Unix systems
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            switch (c) {
                case 'w':
                    playerX += sin(playerA) * playerSpeed * fElapsedTime;
                    playerY += cos(playerA) * playerSpeed * fElapsedTime;
                    
                    // Collision detection
                    if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                        playerX -= sin(playerA) * playerSpeed * fElapsedTime;
                        playerY -= cos(playerA) * playerSpeed * fElapsedTime;
                    }
                    break;
                case 's':
                    playerX -= sin(playerA) * playerSpeed * fElapsedTime;
                    playerY -= cos(playerA) * playerSpeed * fElapsedTime;
                    
                    // Collision detection
                    if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                        playerX += sin(playerA) * playerSpeed * fElapsedTime;
                        playerY += cos(playerA) * playerSpeed * fElapsedTime;
                    }
                    break;
                case 'a':
                    playerX -= cos(playerA) * playerSpeed * fElapsedTime;
                    playerY += sin(playerA) * playerSpeed * fElapsedTime;
                    
                    // Collision detection
                    if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                        playerX += cos(playerA) * playerSpeed * fElapsedTime;
                        playerY -= sin(playerA) * playerSpeed * fElapsedTime;
                    }
                    break;
                case 'd':
                    playerX += cos(playerA) * playerSpeed * fElapsedTime;
                    playerY -= sin(playerA) * playerSpeed * fElapsedTime;
                    
                    // Collision detection
                    if (map[(int)playerY * MAP_WIDTH + (int)playerX] == '#') {
                        playerX -= cos(playerA) * playerSpeed * fElapsedTime;
                        playerY += sin(playerA) * playerSpeed * fElapsedTime;
                    }
                    break;
                case 'q': // Left arrow substitute
                    playerA -= playerRotSpeed * fElapsedTime;
                    break;
                case 'e': // Right arrow substitute
                    playerA += playerRotSpeed * fElapsedTime;
                    break;
                case ' ':
                    shootBullet();
                    break;
                case 27: // ESC
                    gameRunning = false;
                    break;
            }
        }
#endif
        
        // Update bullets
        updateBullets(fElapsedTime);
        
        // Render
#ifdef PLATFORM_WINDOWS
        render(screen);
        
        // Display frame
        screen[SCREEN_WIDTH * SCREEN_HEIGHT - 1] = '\0';
        WriteConsoleOutputCharacter(console, screen, SCREEN_WIDTH * SCREEN_HEIGHT, { 0, 0 }, &bytesWritten);
#else
        render();
#endif
        
        // Frame rate control
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (frameDuration < FRAME_DURATION) {
            std::this_thread::sleep_for(FRAME_DURATION - frameDuration);
        }
    }
    
    // Clean up
#ifdef PLATFORM_WINDOWS
    delete[] screen;
    CloseHandle(console);
#else
    restoreTerminal();
#endif
    
    return 0;
} 