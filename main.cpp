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

// Define trail length as a global constant
const int BULLET_TRAIL_LENGTH = 5;

// Bullets
struct Bullet {
    float x, y;
    float dx, dy;
    bool active;
    
    // Add trail positions to make bullets more visible
    float trailX[BULLET_TRAIL_LENGTH];
    float trailY[BULLET_TRAIL_LENGTH];
    
    Bullet() : x(0), y(0), dx(0), dy(0), active(false) {
        // Initialize trail positions
        for (int i = 0; i < BULLET_TRAIL_LENGTH; i++) {
            trailX[i] = 0;
            trailY[i] = 0;
        }
    }
};

std::vector<Bullet> bullets;
const int MAX_BULLETS = 10;
float bulletSpeed = 5.0f; // Reduced to make bullets more visible

// Enemies
struct Enemy {
    float x, y;
    bool alive;
    
    Enemy(float _x, float _y) : x(_x), y(_y), alive(true) {}
};

std::vector<Enemy> enemies;

// Debug counters
int bulletsFired = 0;
int activeBullets = 0;

// Platform-specific keyboard input handling
#ifdef PLATFORM_UNIX
// Terminal mode settings
struct termios orig_termios;

// Function to initialize terminal for raw input
void initTerminal() {
    // Save original terminal settings
    tcgetattr(STDIN_FILENO, &orig_termios);
    
    // Configure terminal for raw input
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); // Disable echo, canonical mode, and signals
    raw.c_iflag &= ~(IXON | ICRNL);         // Disable software flow control and CR to NL
    raw.c_oflag &= ~(OPOST);                // Disable output processing
    raw.c_cc[VMIN] = 0;                     // Return immediately with whatever is available
    raw.c_cc[VTIME] = 0;                    // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    // Hide cursor and clear screen
    std::cout << "\033[?25l";
    std::cout << "\033[2J\033[H";
    
    // Print debug message
    std::cout << "\033[1;32mTerminal initialized for WSL2. Press SPACE to shoot.\033[0m" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "\033[2J\033[H";
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
    // First check if we have reached the maximum number of bullets
    if (activeBullets >= MAX_BULLETS) {
        // Debug message for WSL2
        #ifdef PLATFORM_UNIX
        std::cout << "\033[1;31mMax bullets reached!\033[0m" << std::endl;
        #endif
        return;
    }
    
    for (auto& bullet : bullets) {
        if (!bullet.active) {
            bullet.x = playerX;
            bullet.y = playerY;
            bullet.dx = sin(playerA) * bulletSpeed;
            bullet.dy = cos(playerA) * bulletSpeed;
            bullet.active = true;
            
            // Initialize all trail positions to create a visible initial trail
            for (int i = 0; i < BULLET_TRAIL_LENGTH; i++) {
                // Offset slightly to create an immediate visible trail
                bullet.trailX[i] = bullet.x - (sin(playerA) * 0.1f * i);
                bullet.trailY[i] = bullet.y - (cos(playerA) * 0.1f * i);
            }
            
            // Debug message for WSL2
            #ifdef PLATFORM_UNIX
            std::cout << "\033[1;31mBullet fired at position (" << bullet.x << ", " << bullet.y 
                      << ") with direction (" << bullet.dx << ", " << bullet.dy << ")\033[0m" << std::endl;
            #endif
            
            bulletsFired++;
            activeBullets++;
            
            break;
        }
    }
}

// Function to update bullets
void updateBullets(float elapsedTime) {
    // Debug output for WSL2
    #ifdef PLATFORM_UNIX
    if (activeBullets > 0) {
        std::cout << "\033[1;31mUpdating " << activeBullets << " active bullets\033[0m" << std::endl;
    }
    #endif
    
    for (auto& bullet : bullets) {
        if (bullet.active) {
            // Debug output for WSL2
            #ifdef PLATFORM_UNIX
            std::cout << "\033[1;33mBullet position: (" << bullet.x << ", " << bullet.y << ")\033[0m" << std::endl;
            #endif
            
            // Update trail positions first (shift all positions)
            for (int i = BULLET_TRAIL_LENGTH - 1; i > 0; i--) {
                bullet.trailX[i] = bullet.trailX[i-1];
                bullet.trailY[i] = bullet.trailY[i-1];
            }
            
            // Store current position as first trail position
            bullet.trailX[0] = bullet.x;
            bullet.trailY[0] = bullet.y;
            
            // Update position
            bullet.x += bullet.dx * elapsedTime;
            bullet.y += bullet.dy * elapsedTime;
            
            // Check for collision with walls
            int mapX = static_cast<int>(bullet.x);
            int mapY = static_cast<int>(bullet.y);
            
            // Make sure we're in bounds
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                bullet.active = false;
                activeBullets--;
                continue;
            }
            
            // Check for wall collision
            if (map[mapY * MAP_WIDTH + mapX] == '#') {
                bullet.active = false;
                activeBullets--;
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
                        activeBullets--;
                        break;
                    }
                }
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
    
    // Draw mini-map with border - make it smaller and position it in the corner
    int miniMapWidth = std::min(MAP_WIDTH, 16);  // Limit minimap width
    int miniMapHeight = std::min(MAP_HEIGHT, 16); // Limit minimap height
    int mapStartX = SCREEN_WIDTH - miniMapWidth - 3;
    
    // Draw map content and border
    for (int y = 0; y <= miniMapHeight + 1; y++) {
        for (int x = 0; x <= miniMapWidth + 1; x++) {
            int screenX = mapStartX + x - 1;
            int screenIndex = y * SCREEN_WIDTH + screenX;
            
            // Check if we're within screen bounds
            if (screenIndex >= 0 && screenIndex < SCREEN_WIDTH * SCREEN_HEIGHT) {
                if (y == 0 || y == miniMapHeight + 1 || x == 0 || x == miniMapWidth + 1) {
                    // Draw border
                    screen[screenIndex] = '+';
                } else if (y > 0 && y <= miniMapHeight && x > 0 && x <= miniMapWidth) {
                    // Draw map content - scale if needed
                    int mapY = (y - 1) * MAP_HEIGHT / miniMapHeight;
                    int mapX = (x - 1) * MAP_WIDTH / miniMapWidth;
                    screen[screenIndex] = map[mapY * MAP_WIDTH + mapX];
                }
            }
        }
    }
    
    // Draw player on mini-map - adjust for scaling
    int playerMapY = (playerY * miniMapHeight / MAP_HEIGHT) + 1;
    int playerMapX = (playerX * miniMapWidth / MAP_WIDTH);
    int playerScreenX = mapStartX + playerMapX;
    int playerScreenIndex = playerMapY * SCREEN_WIDTH + playerScreenX;
    
    if (playerScreenIndex >= 0 && playerScreenIndex < SCREEN_WIDTH * SCREEN_HEIGHT) {
        screen[playerScreenIndex] = 'P';
    }
    
    // Add a label for the mini-map
    std::string mapLabel = "MAP";
    for (size_t i = 0; i < mapLabel.size() && (mapStartX + i) < SCREEN_WIDTH; i++) {
        screen[0 * SCREEN_WIDTH + (mapStartX + i)] = mapLabel[i];
    }
    
    // Draw bullets last to ensure they appear on top of everything else
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
                
                // Draw the main bullet
                for (int y = SCREEN_HEIGHT / 2 - 3; y <= SCREEN_HEIGHT / 2 + 3; y++) {
                    for (int x = bulletCenter - 3; x <= bulletCenter + 3; x++) {
                        if (y >= 0 && y < SCREEN_HEIGHT && x >= 0 && x < SCREEN_WIDTH) {
                            // Use a larger, more visible character with ANSI color
                            screen[y * SCREEN_WIDTH + x] = 'O';
                        }
                    }
                }
                
                // Draw the bullet trail
                for (int i = 0; i < BULLET_TRAIL_LENGTH; i++) {
                    // Skip the first position as it's already drawn as the main bullet
                    if (i == 0) continue;
                    
                    // Calculate trail position on screen
                    float trailAngle = atan2(bullet.trailY[i] - playerY, bullet.trailX[i] - playerX);
                    
                    // Adjust angle to player's perspective
                    while (trailAngle - playerA > 3.14159f) trailAngle -= 2.0f * 3.14159f;
                    while (trailAngle - playerA < -3.14159f) trailAngle += 2.0f * 3.14159f;
                    
                    // Check if trail is in field of view
                    bool trailInFOV = fabs(trailAngle - playerA) < playerFOV / 2.0f;
                    
                    if (trailInFOV) {
                        // Calculate trail position on screen
                        int trailCenter = (int)((trailAngle - playerA + playerFOV / 2.0f) / playerFOV * SCREEN_WIDTH);
                        
                        // Draw trail segment (smaller than the main bullet)
                        for (int y = SCREEN_HEIGHT / 2 - 1; y <= SCREEN_HEIGHT / 2 + 1; y++) {
                            for (int x = trailCenter - 1; x <= trailCenter + 1; x++) {
                                if (y >= 0 && y < SCREEN_HEIGHT && x >= 0 && x < SCREEN_WIDTH) {
                                    // Use a different character for the trail
                                    screen[y * SCREEN_WIDTH + x] = '*';
                                }
                            }
                        }
                    }
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
    ss << "FPS: X | Enemies: " << aliveEnemies << " | Bullets Fired: " << bulletsFired << " | Active Bullets: " << activeBullets;
    std::string stats = ss.str();
    
    for (size_t i = 0; i < stats.size(); i++) {
        screen[i] = stats[i];
    }
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
    
    // Draw mini-map with border - make it smaller and position it in the corner
    int miniMapWidth = std::min(MAP_WIDTH, 16);  // Limit minimap width
    int miniMapHeight = std::min(MAP_HEIGHT, 16); // Limit minimap height
    int mapStartX = renderWidth - miniMapWidth - 3;
    
    // Only draw mini-map if there's enough space
    if (mapStartX > renderWidth / 2 && miniMapHeight + 2 < renderHeight) {
        // Draw a border around the mini-map
        for (int y = 0; y <= miniMapHeight + 1; y++) {
            for (int x = 0; x <= miniMapWidth + 1; x++) {
                // Calculate position in screen buffer
                int screenX = mapStartX + x - 1;
                
                // Check if we're within screen bounds
                if (screenX >= 0 && screenX < renderWidth && y < renderHeight) {
                    if (y == 0 || y == miniMapHeight + 1 || x == 0 || x == miniMapWidth + 1) {
                        // Draw border
                        screenLines[y][screenX] = '+';
                    } else if (y > 0 && y <= miniMapHeight && x > 0 && x <= miniMapWidth) {
                        // Draw map content - scale if needed
                        int mapY = (y - 1) * MAP_HEIGHT / miniMapHeight;
                        int mapX = (x - 1) * MAP_WIDTH / miniMapWidth;
                        screenLines[y][screenX] = map[mapY * MAP_WIDTH + mapX];
                    }
                }
            }
        }
        
        // Draw player on mini-map - adjust for scaling
        int playerMapY = (playerY * miniMapHeight / MAP_HEIGHT) + 1;
        int playerMapX = mapStartX + (playerX * miniMapWidth / MAP_WIDTH);
        
        if (playerMapY >= 0 && playerMapY < renderHeight && 
            playerMapX >= 0 && playerMapX < renderWidth) {
            screenLines[playerMapY][playerMapX] = 'P';
        }
        
        // Add a label for the mini-map
        std::string mapLabel = "MAP";
        for (size_t i = 0; i < mapLabel.size() && (mapStartX + i) < renderWidth; i++) {
            screenLines[0][mapStartX + i] = mapLabel[i];
        }
    }
    
    // Draw bullets last to ensure they appear on top of everything else
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
                
                // Draw the main bullet
                for (int y = renderHeight / 2 - 3; y <= renderHeight / 2 + 3; y++) {
                    for (int x = bulletCenter - 3; x <= bulletCenter + 3; x++) {
                        if (y >= 0 && y < renderHeight && x >= 0 && x < renderWidth) {
                            // Use a larger, more visible character with ANSI color
                            screenLines[y][x] = 'O';
                        }
                    }
                }
                
                // Draw the bullet trail
                for (int i = 0; i < BULLET_TRAIL_LENGTH; i++) {
                    // Skip the first position as it's already drawn as the main bullet
                    if (i == 0) continue;
                    
                    // Calculate trail position on screen
                    float trailAngle = atan2(bullet.trailY[i] - playerY, bullet.trailX[i] - playerX);
                    
                    // Adjust angle to player's perspective
                    while (trailAngle - playerA > 3.14159f) trailAngle -= 2.0f * 3.14159f;
                    while (trailAngle - playerA < -3.14159f) trailAngle += 2.0f * 3.14159f;
                    
                    // Check if trail is in field of view
                    bool trailInFOV = fabs(trailAngle - playerA) < playerFOV / 2.0f;
                    
                    if (trailInFOV) {
                        // Calculate trail position on screen
                        int trailCenter = (int)((trailAngle - playerA + playerFOV / 2.0f) / playerFOV * renderWidth);
                        
                        // Draw trail segment (smaller than the main bullet)
                        for (int y = renderHeight / 2 - 1; y <= renderHeight / 2 + 1; y++) {
                            for (int x = trailCenter - 1; x <= trailCenter + 1; x++) {
                                if (y >= 0 && y < renderHeight && x >= 0 && x < renderWidth) {
                                    // Use a different character for the trail
                                    screenLines[y][x] = '*';
                                }
                            }
                        }
                    }
                }
                
                // Add a prominent debug message at the top of the screen
                std::string bulletMsg = "!!!!! BULLET ACTIVE !!!!!";
                for (size_t i = 0; i < bulletMsg.size() && i + 20 < renderWidth; i++) {
                    screenLines[2][i + 20] = bulletMsg[i];
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
    ss << "FPS: X | Enemies: " << aliveEnemies << " | Bullets Fired: " << bulletsFired << " | Active Bullets: " << activeBullets;
    std::string stats = ss.str();
    
    for (size_t i = 0; i < stats.size() && i < renderWidth; i++) {
        screenLines[0][i] = stats[i];
    }
    
    // Draw screen
    clearScreen();
    for (int y = 0; y < renderHeight; y++) {
        std::string line = "";
        for (int x = 0; x < renderWidth; x++) {
            char c = screenLines[y][x];
            // Add color to bullets and trails
            if (c == 'O') {
                line += "\033[1;31mO\033[0m"; // Bright red bullet
            } else if (c == '*') {
                line += "\033[1;33m*\033[0m"; // Yellow trail
            } else {
                line += c;
            }
        }
        std::cout << line << std::endl;
    }
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
        
        if (GetAsyncKeyState('S') &     ) {
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
        
        // Make input non-blocking for WSL2
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        
        // Check for input
        while (read(STDIN_FILENO, &c, 1) > 0) {
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
                    // Add a small delay to prevent multiple shots
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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