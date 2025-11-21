#include "esp_heap_caps.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "controls.h"
#include "values.h"
#include <vector>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// --- Game Constants ---
const float GRAVITY = 0.4f;
const float JUMP_FORCE = -6.5f; // Slightly floaty
const float MOVE_SPEED = 2.5f;
const float SPRINT_SPEED = 4.5f;
const float FRICTION = 0.8f;
const float MAX_FALL_SPEED = 8.0f;

const int PLAYER_SIZE = 16;
const int LEVEL_LENGTH = 4000; // Pixels

// Sprint mechanics
const float SPRINT_MAX = 100.0f;
const float SPRINT_DRAIN = 1.5f;
const float SPRINT_REGEN = 0.3f;

// --- Types ---
enum BoxType {
    PLATFORM = 0,
    LAVA = 1,
    GOAL = 2
};

struct Box {
    int x, y, w, h;
    BoxType type;
};

struct Player {
    float x, y;
    float vx, vy;
    bool grounded;
    int jumpsLeft;
    float sprintEnergy;
};

// --- Globals ---
Player player;
std::vector<Box> level;
ControlsState controls;
float camX = 0;
bool gameOver = false;
bool victory = false;

// --- Level Generation ---
void generateLevel() {
    level.clear();
    
    // Floor at y = 200
    int curX = 0;
    int floorY = 200;
    
    // Starting platform
    level.push_back({0, floorY, 300, 40, PLATFORM});
    curX += 300;

    // Procedural generation of segments
    while (curX < LEVEL_LENGTH) {
        int r = random(0, 10);
        
        if (r < 3) { 
            // Gap with lava
            int gap = random(40, 80);
            level.push_back({curX, floorY + 20, gap, 20, LAVA}); // Lava pit
            curX += gap;
            level.push_back({curX, floorY, random(100, 200), 40, PLATFORM});
            curX += level.back().w;
        } else if (r < 6) {
            // Stairsteps / High platforms
            int h = random(100, 200);
            int w = random(60, 100);
            int y = floorY - random(20, 60);
            level.push_back({curX, y, w, 40, PLATFORM});
            
            // Maybe lava below?
            level.push_back({curX, floorY + 20, w, 20, LAVA});
            
            curX += w + random(20, 50);
        } else if (r < 8) {
             // Wide gap for double jump/sprint
             int gap = random(80, 110);
             level.push_back({curX, floorY + 20, gap, 20, LAVA});
             curX += gap;
             level.push_back({curX, floorY, 150, 40, PLATFORM});
             curX += 150;
        } else {
            // Standard floor run
            int w = random(200, 400);
            level.push_back({curX, floorY, w, 40, PLATFORM});
            curX += w;
        }
    }

    // Goal
    level.push_back({curX + 50, floorY - 60, 40, 100, GOAL});
    // Final platform under goal
    level.push_back({curX, floorY, 200, 40, PLATFORM});
}

void resetGame() {
    player.x = 50;
    player.y = 100;
    player.vx = 0;
    player.vy = 0;
    player.grounded = false;
    player.jumpsLeft = 0;
    player.sprintEnergy = SPRINT_MAX;
    camX = 0;
    gameOver = false;
    victory = false;
    generateLevel();
}

bool checkCollision(float nx, float ny, Box &hitBox) {
    // Simple AABB
    for (const auto &b : level) {
        if (nx < b.x + b.w && nx + PLAYER_SIZE > b.x &&
            ny < b.y + b.h && ny + PLAYER_SIZE > b.y) {
            hitBox = b;
            return true;
        }
    }
    return false;
}

void updatePhysics() {
    if (gameOver || victory) {
        if (controls.a_pressed || controls.start) resetGame();
        return;
    }

    // Horizontal Movement
    float speed = MOVE_SPEED;
    if (controls.b && player.sprintEnergy > 0) {
        speed = SPRINT_SPEED;
        player.sprintEnergy -= SPRINT_DRAIN;
    } else {
        player.sprintEnergy += SPRINT_REGEN;
    }
    if (player.sprintEnergy > SPRINT_MAX) player.sprintEnergy = SPRINT_MAX;
    if (player.sprintEnergy < 0) player.sprintEnergy = 0;

    if (controls.left) player.vx = -speed;
    else if (controls.right) player.vx = speed;
    else player.vx *= FRICTION;

    // Jump
    if (controls.a_pressed) {
        if (player.grounded) {
            player.vy = JUMP_FORCE;
            player.grounded = false;
            player.jumpsLeft = 1; // Double jump available
        } else if (player.jumpsLeft > 0) {
            player.vy = JUMP_FORCE;
            player.jumpsLeft--;
        }
    }

    // Apply Gravity
    player.vy += GRAVITY;
    if (player.vy > MAX_FALL_SPEED) player.vy = MAX_FALL_SPEED;

    // Move X
    player.x += player.vx;
    Box hit;
    if (checkCollision(player.x, player.y, hit)) {
        // Resolve X collision
        if (player.vx > 0) player.x = hit.x - PLAYER_SIZE;
        else if (player.vx < 0) player.x = hit.x + hit.w;
        player.vx = 0;
        
        if (hit.type == LAVA) gameOver = true;
        if (hit.type == GOAL) victory = true;
    }

    // Move Y
    player.y += player.vy;
    if (checkCollision(player.x, player.y, hit)) {
        // Resolve Y collision
        if (player.vy > 0) { // Landing
            player.y = hit.y - PLAYER_SIZE;
            player.grounded = true;
            player.jumpsLeft = 1; // Reset jumps on landing
        } else if (player.vy < 0) { // Hitting head
            player.y = hit.y + hit.h;
        }
        player.vy = 0;

        if (hit.type == LAVA) gameOver = true;
        if (hit.type == GOAL) victory = true;
    } else {
        player.grounded = false;
    }

    // Fallen off world
    if (player.y > 300) gameOver = true;

    // Camera follow
    float targetCamX = player.x - 100;
    if (targetCamX < 0) targetCamX = 0;
    camX += (targetCamX - camX) * 0.1f;
}

void draw() {
    // With 8-bit color depth, we need to be careful with colors if not using palette.
    // TFT_eSPI maps 16-bit colors to best match in 3-3-2 format if we use standard fillRect etc.
    sprite.fillSprite(TFT_BLACK);

    // Draw Level
    for (const auto &b : level) {
        // Cull off-screen
        if (b.x + b.w < camX || b.x > camX + 320) continue;

        uint16_t color = TFT_GREEN;
        if (b.type == LAVA) color = TFT_RED;
        if (b.type == GOAL) color = TFT_GOLD;

        sprite.fillRect(b.x - (int)camX, b.y, b.w, b.h, color);
    }

    // Draw Player
    sprite.fillRect((int)player.x - (int)camX, (int)player.y, PLAYER_SIZE, PLAYER_SIZE, TFT_CYAN);

    // UI
    // Sprint Bar
    sprite.drawRect(10, 10, 102, 12, TFT_WHITE);
    sprite.fillRect(11, 11, (int)player.sprintEnergy, 10, TFT_YELLOW);
    
    if (gameOver) {
        sprite.setTextColor(TFT_RED);
        sprite.setTextSize(3);
        sprite.drawCentreString("GAME OVER", 160, 100, 1);
        sprite.setTextSize(1);
        sprite.setTextColor(TFT_WHITE);
        sprite.drawCentreString("Press A to Restart", 160, 140, 1);
    } else if (victory) {
        sprite.setTextColor(TFT_GOLD);
        sprite.setTextSize(3);
        sprite.drawCentreString("YOU WIN!", 160, 100, 1);
        sprite.setTextSize(1);
        sprite.setTextColor(TFT_WHITE);
        sprite.drawCentreString("Press A to Restart", 160, 140, 1);
    }

    sprite.pushSprite(0, 0);
}

void setup() {
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    control_init();
    tft.init();
    tft.setRotation(1);
    tft.initDMA();
    tft.fillScreen(TFT_BLACK);

    // Use 8-bit sprite to save RAM if PSRAM fails or always if we suspect no PSRAM
    // 320x240 * 2 bytes = 150KB (too big for internal RAM usually)
    // 320x240 * 1 byte = 75KB (fit easily)
    
    if (psramFound()) {
        sprite.setColorDepth(16);
        if (!sprite.createSprite(320, 240)) {
            Serial.println("16-bit Sprite failed in PSRAM. Falling back to 8-bit internal.");
            sprite.setColorDepth(8);
            sprite.createSprite(320, 240);
        }
    } else {
        Serial.println("No PSRAM. Using 8-bit Sprite.");
        sprite.setColorDepth(8);
        if (!sprite.createSprite(320, 240)) {
             Serial.println("8-bit Sprite failed! Trying 4-bit...");
             sprite.setColorDepth(4);
             sprite.createSprite(320, 240);
        }
    }
    
    // If using 4 or 8 bit, we rely on default palette or color mapping
    
    resetGame();
}

void loop() {
    control_read(controls);
    updatePhysics();
    draw();
    delay(16); // ~60 FPS
}
