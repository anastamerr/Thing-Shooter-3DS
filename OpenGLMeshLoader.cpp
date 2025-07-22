#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <glut.h>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Window settings
int WIDTH = 1280;
int HEIGHT = 720;
GLuint tex;
GLuint tex1;
char title[] = "3D Game";

// Camera settings
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 200;

// sound implementation
class SimpleSound {
private:
    bool soundEnabled;

public:
    SimpleSound() : soundEnabled(true) {}

    void playSound(const char* soundFile) {
        if (soundEnabled) {
            PlaySound(soundFile, NULL, SND_ASYNC | SND_FILENAME);
        }
    }

    void enableSound() { soundEnabled = true; }
    void disableSound() { soundEnabled = false; }
};

// Global sound system instance
SimpleSound* soundSystem = nullptr;
// Vector class definition
class Vector {
public:
    GLdouble x, y, z;
    Vector() {}
    Vector(GLdouble _x, GLdouble _y, GLdouble _z) : x(_x), y(_y), z(_z) {}
    void operator +=(float value) {
        x += value;
        y += value;
        z += value;
    }
};

struct Particle {
    float x, y, z;        // Position
    float dx, dy, dz;     // Velocity
    float lifetime;       // Current lifetime
    float maxLifetime;    // Maximum lifetime
    bool active;          // Whether particle is active
    float size;          // Particle size
    float r, g, b;       // Particle color
};

const float AMMO_BOX_RESPAWN_TIME = 30.0f;  // Seconds until ammo box respawns
struct AmmoBox {
    float x, y, z;
    float rotation;
    float scale;
    float rotationSpeed;  // Add this new variable

    bool active;
    bool isHighDamage;    // Renamed but kept for compatibility with level 1
    bool isExplosive;     // New flag for explosive ammo
    bool isFastFire;      // New flag for fast fire ammo
    float respawnTimer;
};

struct Explosion {
    float x, y, z;              // Explosion center
    std::vector<Particle> particles;
    float lifetime;             // Current lifetime
    float maxLifetime;          // Maximum lifetime
    bool active;                // Whether explosion is active
};

std::vector<Explosion> explosions;
const int PARTICLES_PER_EXPLOSION = 30;
const float EXPLOSION_LIFETIME = 0.5f;  // Half a second
const float PARTICLE_SIZE = 0.05f;

// Structure definitions
struct Bullet {
    float x, y, z;           // Position
    float dx, dy, dz;        // Direction
    float rotation;          // Bullet rotation to face direction
    float speed;             // Speed
    bool active;             // Whether bullet is currently in flight
    float lifetime;          // How long the bullet has existed
    float maxLifetime;       // Maximum time before bullet disappears
    float scale;             // Bullet scale
};

struct Door {
    float x, y, z;
    float rotation;
    float scale;
    bool active;
    const float INTERACTION_RADIUS = 2.0f;  // Distance at which player can interact with door
};

Door levelDoor;
bool doorSpawned = false;

struct Target {
    float x, y, z;           // Position
    float rotation;          // Rotation
    float scale;             // Scale
    bool active;             // Whether target is still active/visible
    int treeIndex;          // Index of the tree this target is attached to
};

struct SceneObject {
    float x, z;
    float scale;
    float rotation;
};

//flashlight
GLfloat flashlightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat flashlightDirection[] = { 0.0f, 0.0f, -1.0f };
const float FLASHLIGHT_CUTOFF = 45.0f;
bool flashlightEnabled = true;



float sunsetProgress = 0.0f;  // 0.0 = high noon, 1.0 = sunset
const float SUNSET_SPEED = 0.2f;  // Speed of the sunset cycle
const float MAX_SUN_HEIGHT = 100.0f;
const float MIN_SUN_HEIGHT = 5.0f;
const float SUN_DISTANCE = 100.0f;
GLUquadricObj* sunQuadric = nullptr;  // For rendering the sun sphere
const float SUN_VISUAL_SIZE = 10.0f;   // Size of the sun sphere
float currentSunPosition[4] = { 0.0f, MAX_SUN_HEIGHT, -SUN_DISTANCE, 1.0f };
//Light 
float lightAnimationTime = 0.0f;
float lampRotationAngle = 0.0f;
float lightIntensityFactor = 1.0f;
bool lightIntensityIncreasing = true;
const float LIGHT_ANIMATION_SPEED = 0.5f;
const float LAMP_ROTATION_SPEED = 0.2f;
const float MIN_LIGHT_INTENSITY = 0.6f;
const float MAX_LIGHT_INTENSITY = 1.0f;
const float LIGHT_INTENSITY_CHANGE_SPEED = 0.003f;


// Camera vectors
Vector Eye(0, 2, 5);
Vector At(0, 0, 0);
Vector Up(0, 1, 0);

// Models
Model_3DS model_tree;
Model_3DS model_rock;

Model_3DS player_model;
Model_3DS bullet_model;
Model_3DS target_model;
Model_3DS model_wall;
Model_3DS Chair;

std::vector<Target> targets;
const int NUM_TARGETS = 7;



// Ammo boxes
std::vector<AmmoBox> ammoBoxes;
const float AMMO_BOX_COLLECTION_RADIUS = 1.0f;
const int AMMO_BOX_REGULAR_AMOUNT = 3;
const int AMMO_BOX_HIGH_DAMAGE_AMOUNT = 2;
bool hasHighDamageAmmo = false;
const float REGULAR_DAMAGE = 1.0f;
const float HIGH_DAMAGE = 2.0f;
float currentBulletDamage = REGULAR_DAMAGE;
// Add with other global variables
int ammoReserves = 0;  // Total ammo available for reloading
const int MAX_AMMO_RESERVES = 30;  // Maximum ammo you can carry
const float EXPLOSIVE_DAMAGE = 2.5f;
const float FAST_FIRE_COOLDOWN = 0.1f;  // Faster fire rate
const float NORMAL_FIRE_COOLDOWN = 0.2f; // Original fire rate


// Player variables
float playerX = 0.0f;
float playerY = 0.0f;
float playerZ = 0.0f;
float playerRotation = 0.0f;
float moveSpeed = 0.2f;
float jumpForce = 0.3f;
float gravity = 0.01f;
bool isJumping = false;
float jumpVelocity = 0.0f;
float playerScale = 0.9f;
bool isSpacePressed = false;
float playerHealth = 100.0f;
int playerScore = 0;
bool isFirstPerson = false;

// Camera variables
float cameraDistance = 3.0f;
float cameraHeight = 1.5f;
float cameraSmoothness = 0.2f;
Vector currentCameraPos(0, 2, 5);

//screen shake for hussiens eyes
float shakeAmplitude = 0.0f;    // Current shake intensity
float shakeDecay = 0.9f;        // How quickly the shake effect fades
float maxShakeAmplitude = 0.2f; // Maximum shake intensity
float shakeOffsetX = 0.0f;      // Current X offset from shake
float shakeOffsetY = 0.0f;      // Current Y offset from shake

//Collsions 
const float TREE_COLLISION_RADIUS = 1.0f;  // Radius for tree collision
const float ROCK_COLLISION_RADIUS = 1.0f;  // Radius for rock collision
const float ROCK_DAMAGE = 25.0f;           // Damage taken from stepping on rocks
const float DAMAGE_COOLDOWN = 1.0f;

float lastDamageTime = 0.0f;


// Timer and game state variables
const float GAME_TIME_LIMIT = 120.0f;  // 2 minutes in seconds
float gameTimer = GAME_TIME_LIMIT;
bool gameOver = false;
bool playerWon = false;
const int WINNING_SCORE = 5;


//relod variables
const int MAX_AMMO = 5;  // Maximum bullets in magazine
int currentAmmo = MAX_AMMO;  // Current bullets in magazine
bool isReloading = false;    // Reloading state
float reloadTime = 2.0f;     // Time to reload in seconds
float currentReloadTime = 0.0f;

// Aiming variables
bool isAiming = false;
float normalFOV = 45.0f;

float zoomFOV = 15.0f;  // Increased zoom for more precise aiming
float zoomSpeed = 0.15f;  // Slightly increased for smoother transition
float currentFOV = normalFOV;

// Add these variables at the top with other camera variables
float verticalAngle = 0.0f;  // Looking up/down angle
float mouseSensitivity = 0.2f;  // Adjust this to change mouse sensitivity
int lastX = WIDTH / 2;
int lastY = HEIGHT / 2;
bool firstMouse = true;

int currentLevel = 1;
const int MAX_LEVEL = 2;
bool levelCompleted = false;

// Bullet variables
std::vector<Bullet> bullets;
const float BULLET_SPEED = 12.0f;
const float BULLET_SCALE = 0.01f;
const float BULLET_MAX_LIFETIME = 3.5f;
bool isShooting = false;
float lastShotTime = 0.0f;
float shootingCooldown = 0.2f;

// Scene objects
std::vector<SceneObject> treePositions;
std::vector<SceneObject> rockPositions;

// Timer interval (60 FPS)
const int TIMER_INTERVAL = 16;

// Textures
GLTexture tex_ground;
GLTexture tex_rock;
GLTexture tex_sniper;
GLTexture tex_map2ground;
GLTexture tex_map2sky;
GLTexture tex_target;  // Add this with other texture declarations
GLTexture tex_mtarget;
GLTexture tex_wall;  // Add with other GLTexture declarations


// Function declarations
void updateCamera();
void updateScreenShake();
void UpdateBullets();
void ShootBullet();
void RenderBullets();
void RenderGround();
void RenderUI();
void RenderCrosshair();
void LoadAssets();

//sound
void playGameSounds(const char* soundType) {
    if (soundSystem) {
        if (strcmp(soundType, "shoot") == 0) {
            soundSystem->playSound("Sounds/shoot.wav");
        }
        else if (strcmp(soundType, "hit") == 0) {
            soundSystem->playSound("Sounds/hit.wav");
        }
        else if (strcmp(soundType, "reload") == 0) {
            soundSystem->playSound("Sounds/reload.wav");
        }
        else if (strcmp(soundType, "jump") == 0) {
            soundSystem->playSound("Sounds/jump.wav");
        }
        else if (strcmp(soundType, "damage") == 0) {
            soundSystem->playSound("Sounds/hurt.wav");
        }
    }
}

void UpdateSunPosition(float deltaTime) {
    if (currentLevel == 1) {
        // Update sunset progress
        sunsetProgress += deltaTime * SUNSET_SPEED;
        if (sunsetProgress >= 1.0f) sunsetProgress = 0.0f;

        float angle = sunsetProgress * 3.14159f;
        float sunHeight = MAX_SUN_HEIGHT * cos(angle * 0.5f);
        if (sunHeight < MIN_SUN_HEIGHT) sunHeight = MIN_SUN_HEIGHT;

        float horizontalOffset = SUN_DISTANCE * sin(angle);

        // Store current sun position for rendering
        currentSunPosition[0] = horizontalOffset;
        currentSunPosition[1] = sunHeight;
        currentSunPosition[2] = -SUN_DISTANCE * cos(angle);
        currentSunPosition[3] = 1.0f;

        // Calculate lighting colors...
        float intensity = 1.0f - (sunsetProgress * 0.7f);
        float redComponent = 1.0f;
        float greenComponent = 1.0f - (sunsetProgress * 0.5f);
        float blueComponent = 1.0f - (sunsetProgress * 0.8f);

        GLfloat sunAmbient[] = {
            0.3f * intensity * redComponent,
            0.3f * intensity * greenComponent,
            0.3f * intensity * blueComponent,
            1.0f
        };

        GLfloat sunDiffuse[] = {
            1.0f * intensity * redComponent,
            1.0f * intensity * greenComponent,
            1.0f * intensity * blueComponent,
            1.0f
        };

        GLfloat sunSpecular[] = {
            0.7f * intensity * redComponent,
            0.7f * intensity * greenComponent,
            0.7f * intensity * blueComponent,
            1.0f
        };

        // Update light properties
        glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
        glLightfv(GL_LIGHT0, GL_POSITION, currentSunPosition);

        GLfloat modelAmbient[] = {
            0.2f * intensity,
            0.2f * intensity * greenComponent,
            0.2f * intensity * blueComponent,
            1.0f
        };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);
    }
}
void updateScreenShake() {
    if (shakeAmplitude > 0.001f) {
        // Generate random offsets
        shakeOffsetX = (((rand() % 200) - 100) / 100.0f) * shakeAmplitude;
        shakeOffsetY = (((rand() % 200) - 100) / 100.0f) * shakeAmplitude;

        // Decay the shake effect
        shakeAmplitude *= shakeDecay;
    }
    else {
        shakeAmplitude = 0.0f;
        shakeOffsetX = 0.0f;
        shakeOffsetY = 0.0f;
    }
}

void SpawnDoor() {
    levelDoor.x = 0.0f;  // Spawn door at origin
    levelDoor.y = 1.0f;
    levelDoor.z = -10.0f;  // Some distance in front of starting position
    levelDoor.rotation = 0.0f;
    levelDoor.scale = 2.0f;
    levelDoor.active = true;
    doorSpawned = true;
}

void UpdateLightEffects(float deltaTime) {
    // Update animation time
    lightAnimationTime += deltaTime;
    lampRotationAngle += LAMP_ROTATION_SPEED * deltaTime;

    // Update light intensity with smooth transition
    if (lightIntensityIncreasing) {
        lightIntensityFactor += LIGHT_INTENSITY_CHANGE_SPEED;
        if (lightIntensityFactor >= MAX_LIGHT_INTENSITY) {
            lightIntensityFactor = MAX_LIGHT_INTENSITY;
            lightIntensityIncreasing = false;
        }
    }
    else {
        lightIntensityFactor -= LIGHT_INTENSITY_CHANGE_SPEED;
        if (lightIntensityFactor <= MIN_LIGHT_INTENSITY) {
            lightIntensityFactor = MIN_LIGHT_INTENSITY;
            lightIntensityIncreasing = true;
        }
    }
}

void UpdateFlashlight() {
    // Calculate flashlight position (at player's eye level)
    float horizontalRad = playerRotation * 3.14159f / 180.0f;
    float verticalRad = verticalAngle * 3.14159f / 180.0f;

    // Position the flashlight at the player's view position
    flashlightPosition[0] = Eye.x;
    flashlightPosition[1] = Eye.y;
    flashlightPosition[2] = Eye.z;

    // Calculate the direction vector based on player's rotation
    flashlightDirection[0] = sin(horizontalRad) * cos(verticalRad);
    flashlightDirection[1] = sin(verticalRad);
    flashlightDirection[2] = cos(horizontalRad) * cos(verticalRad);
}

// Bullet functions
void UpdateBullets() {
    float deltaTime = TIMER_INTERVAL / 1000.0f;

    for (auto& bullet : bullets) {
        if (!bullet.active) continue;

        bullet.x += bullet.dx * bullet.speed * deltaTime;
        bullet.y += bullet.dy * bullet.speed * deltaTime;
        bullet.z += bullet.dz * bullet.speed * deltaTime;

        bullet.lifetime += deltaTime;

        if (bullet.lifetime >= bullet.maxLifetime) {
            bullet.active = false;
        }
    }
}

void ShootBullet() {
    if (gameOver) return;
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    if (currentTime - lastShotTime < shootingCooldown) {
        return;
    }

    // Check if we have ammo and not reloading
    if (currentAmmo <= 0 || isReloading) {
        return;  // Can't shoot without ammo or while reloading
    }

    // Calculate the exact forward vector from the camera when aiming
    float horizontalRad = playerRotation * 3.14159f / 180.0f;
    float verticalRad = verticalAngle * 3.14159f / 180.0f;

    // Calculate bullet direction
    float dx = sin(horizontalRad) * cos(verticalRad);
    float dy = sin(verticalRad);
    float dz = cos(horizontalRad) * cos(verticalRad);

    // Normalize direction vector
    float length = sqrt(dx * dx + dy * dy + dz * dz);
    dx /= length;
    dy /= length;
    dz /= length;

    // Set bullet spawn position
    float spawnX, spawnY, spawnZ;
    float spawnDistance = 1.0f;

    if (isAiming || isFirstPerson) {
        // Spawn from camera position when aiming or in first person
        spawnX = Eye.x;
        spawnY = Eye.y;
        spawnZ = Eye.z;
    }
    else {
        // Spawn from player position
        spawnX = playerX + dx * spawnDistance;
        spawnY = playerY + 1.7f; // Eye level
        spawnZ = playerZ + dz * spawnDistance;
    }

    // Find an inactive bullet or create a new one
    bool bulletFound = false;
    for (auto& bullet : bullets) {
        if (!bullet.active) {
            bullet.active = true;
            bullet.lifetime = 0.0f;
            bullet.x = spawnX;
            bullet.y = spawnY;
            bullet.z = spawnZ;
            bullet.dx = dx;
            bullet.dy = dy;
            bullet.dz = dz;
            bullet.speed = BULLET_SPEED;
            bullet.maxLifetime = BULLET_MAX_LIFETIME;
            bullet.scale = BULLET_SCALE;
            bullet.rotation = playerRotation;
            bulletFound = true;
            break;
        }
    }

    if (!bulletFound) {
        Bullet newBullet;
        newBullet.active = true;
        newBullet.lifetime = 0.0f;
        newBullet.x = spawnX;
        newBullet.y = spawnY;
        newBullet.z = spawnZ;
        newBullet.dx = dx;
        newBullet.dy = dy;
        newBullet.dz = dz;
        newBullet.speed = BULLET_SPEED;
        newBullet.maxLifetime = BULLET_MAX_LIFETIME;
        newBullet.scale = BULLET_SCALE;
        newBullet.rotation = playerRotation;
        bullets.push_back(newBullet);
    }
    playGameSounds("shoot");
    currentAmmo--;
    lastShotTime = currentTime;
}

// Add reload function
void StartReload() {
    if (!isReloading && currentAmmo < MAX_AMMO && ammoReserves > 0) {
        isReloading = true;
        currentReloadTime = 0.0f;
        playGameSounds("reload");

    }
}

// Update reload progress in updateScene
void UpdateReload() {
    if (isReloading) {
        float deltaTime = TIMER_INTERVAL / 1000.0f;
        currentReloadTime += deltaTime;

        if (currentReloadTime >= reloadTime) {
            int ammoNeeded = MAX_AMMO - currentAmmo;
            int ammoToAdd = min(ammoNeeded, ammoReserves);

            currentAmmo += ammoToAdd;
            ammoReserves -= ammoToAdd;

            isReloading = false;
            currentReloadTime = 0.0f;
        }
    }
}

void CreateExplosion(float x, float y, float z) {
    Explosion explosion;
    explosion.x = x;
    explosion.y = y;
    explosion.z = z;
    explosion.lifetime = 0.0f;
    explosion.maxLifetime = EXPLOSION_LIFETIME;
    explosion.active = true;

    // Create particles
    for (int i = 0; i < PARTICLES_PER_EXPLOSION; i++) {
        Particle particle;
        particle.x = x;
        particle.y = y;
        particle.z = z;

        // Random velocity in all directions
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float elevation = (rand() % 180) * 3.14159f / 180.0f;
        float speed = 2.0f + (rand() % 100) / 100.0f;

        particle.dx = sin(angle) * cos(elevation) * speed;
        particle.dy = sin(elevation) * speed;
        particle.dz = cos(angle) * cos(elevation) * speed;

        particle.lifetime = 0.0f;
        particle.maxLifetime = EXPLOSION_LIFETIME;
        particle.active = true;
        particle.size = PARTICLE_SIZE;

        // Orange/red colors for explosion
        particle.r = 1.0f;
        particle.g = 0.5f + (rand() % 50) / 100.0f;  // Random orange variation
        particle.b = 0.0f;

        explosion.particles.push_back(particle);
    }

    explosions.push_back(explosion);
}
void UpdateAmmoBoxRotations() {
    float deltaTime = TIMER_INTERVAL / 1000.0f;

    for (auto& box : ammoBoxes) {
        if (box.active) {
            box.rotation += box.rotationSpeed * deltaTime;
            if (box.rotation >= 360.0f) {
                box.rotation -= 360.0f;
            }
        }
    }
}
void SpawnAmmoBoxes() {
    ammoBoxes.clear();
    int numBoxes = 5;

    for (int i = 0; i < numBoxes; i++) {
        AmmoBox box;
        box.x = (rand() % 60) - 30;
        box.y = 0.5f;
        box.z = (rand() % 60) - 30;
        box.rotation = rand() % 360;
        box.rotationSpeed = 50.0f + (rand() % 50);  // Random speed between 50-100 degrees per second

        box.scale = 0.3f;
        box.active = true;
        box.respawnTimer = 0.0f;

        if (currentLevel == 1) {
            box.isHighDamage = (i < 2);
            box.isExplosive = false;
            box.isFastFire = false;
        }
        else {
            box.isHighDamage = false;
            box.isExplosive = (i < 2);  // First 2 boxes are explosive
            box.isFastFire = !box.isExplosive;  // Rest are fast fire
        }

        // Ensure boxes aren't too close to spawn point
        if (sqrt(box.x * box.x + box.z * box.z) < 5.0f) {
            box.x += (box.x < 0) ? -5.0f : 5.0f;
            box.z += (box.z < 0) ? -5.0f : 5.0f;
        }

        ammoBoxes.push_back(box);
    }
}

void RenderSun() {
    if (currentLevel != 1) return;  // Only render sun in first map

    glPushMatrix();
    glDisable(GL_LIGHTING);  // Disable lighting for the sun
    glDisable(GL_DEPTH_TEST);  // Make sure sun is always visible

    // Use current sun position
    glTranslatef(currentSunPosition[0], currentSunPosition[1], currentSunPosition[2]);

    // Calculate sun color based on sunset progress
    float redComponent = 1.0f;
    float greenComponent = 1.0f - (sunsetProgress * 0.5f);
    float blueComponent = 1.0f - (sunsetProgress * 0.8f);

    // Make the sun glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Draw outer glow
    float glowSize = SUN_VISUAL_SIZE * 1.2f;
    glColor4f(redComponent, greenComponent * 0.7f, blueComponent * 0.3f, 0.2f);
    gluSphere(sunQuadric, glowSize, 32, 32);

    // Draw main sun body
    glColor3f(redComponent, greenComponent, blueComponent);
    gluSphere(sunQuadric, SUN_VISUAL_SIZE, 32, 32);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
void RenderWall(const SceneObject& wall) {
    glPushMatrix();
    glTranslatef(wall.x, 0, wall.z);
    glRotatef(wall.rotation, 0, 1, 0);
    glScalef(wall.scale, wall.scale * 3.0f, wall.scale); // Scale y to make walls taller

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_wall.texture[0]);

    glBegin(GL_QUADS);
    // Front face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.0f, 0.0f, 0.5f);
    glTexCoord2f(2.0f, 0.0f); glVertex3f(4.0f, 0.0f, 0.5f);
    glTexCoord2f(2.0f, 2.0f); glVertex3f(4.0f, 4.0f, 0.5f);
    glTexCoord2f(0.0f, 2.0f); glVertex3f(-4.0f, 4.0f, 0.5f);

    // Back face
    glTexCoord2f(2.0f, 0.0f); glVertex3f(-4.0f, 0.0f, -0.5f);
    glTexCoord2f(2.0f, 2.0f); glVertex3f(-4.0f, 4.0f, -0.5f);
    glTexCoord2f(0.0f, 2.0f); glVertex3f(4.0f, 4.0f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(4.0f, 0.0f, -0.5f);

    // Top face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.0f, 4.0f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-4.0f, 4.0f, 0.5f);
    glTexCoord2f(2.0f, 1.0f); glVertex3f(4.0f, 4.0f, 0.5f);
    glTexCoord2f(2.0f, 0.0f); glVertex3f(4.0f, 4.0f, -0.5f);

    // Right face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(4.0f, 0.0f, -0.5f);
    glTexCoord2f(0.0f, 2.0f); glVertex3f(4.0f, 4.0f, -0.5f);
    glTexCoord2f(1.0f, 2.0f); glVertex3f(4.0f, 4.0f, 0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(4.0f, 0.0f, 0.5f);

    // Left face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.0f, 0.0f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-4.0f, 0.0f, 0.5f);
    glTexCoord2f(1.0f, 2.0f); glVertex3f(-4.0f, 4.0f, 0.5f);
    glTexCoord2f(0.0f, 2.0f); glVertex3f(-4.0f, 4.0f, -0.5f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void RenderGameOverScreen() {
    // Switch to orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Draw darkened overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Semi-transparent black overlay
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();

    // Draw game over message
    std::string gameOverText;
    if (playerWon) {
        glColor3f(0.0f, 1.0f, 0.0f);  // Green for win
        gameOverText = "YOU WON!";
    }
    else {
        glColor3f(1.0f, 0.0f, 0.0f);  // Red for loss
        gameOverText = "YOU LOST!";
    }

    // Calculate center position for text
    void* font = GLUT_BITMAP_TIMES_ROMAN_24;
    float textWidth = 0;
    for (char c : gameOverText) {
        textWidth += glutBitmapWidth(font, c);
    }
    float textX = (WIDTH - textWidth) / 2;
    float textY = HEIGHT / 2;

    // Draw main message
    glRasterPos2f(textX, textY);
    for (char c : gameOverText) {
        glutBitmapCharacter(font, c);
    }

    // Draw "Press ESC to exit" message
    std::string exitText = "Press ESC to exit";
    void* smallFont = GLUT_BITMAP_HELVETICA_18;
    float exitTextWidth = 0;
    for (char c : exitText) {
        exitTextWidth += glutBitmapWidth(smallFont, c);
    }
    float exitTextX = (WIDTH - exitTextWidth) / 2;

    glColor3f(1.0f, 1.0f, 1.0f);  // White color for exit message
    glRasterPos2f(exitTextX, textY + 40);  // Position below main message
    for (char c : exitText) {
        glutBitmapCharacter(smallFont, c);
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void RenderAmmoBox(const AmmoBox& box) {
    if (!box.active) return;

    glPushMatrix();
    glTranslatef(box.x, box.y, box.z);
    glRotatef(box.rotation, 0, 1, 0);
    glScalef(box.scale, box.scale, box.scale);

    // Set color based on level and ammo type
    if (currentLevel == 2) {
        if (box.isExplosive) {
            glColor3f(1.0f, 1.0f, 0.0f);  // Yellow for explosive ammo
        }
        else {
            glColor3f(0.0f, 0.8f, 0.0f);  // Green for fast fire ammo
        }
    }
    else {
        if (box.isHighDamage) {
            glColor3f(0.45f, 0.05f, 0.05f);  // Darker rusty red
        }
        else {
            glColor3f(0.05f, 0.05f, 0.35f);  // Darker rusty blue
        }
    }

    // Draw main box body
    glBegin(GL_QUADS);
    // Front face
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);

    // Back face
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    // Top face
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    // Bottom face
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);

    // Right face
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);

    // Left face
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glEnd();

    // Draw metallic edges/trim with darker, rustier color
    glColor3f(0.4f, 0.4f, 0.4f);  // Darker metallic color for rusty look
    float trim = 0.1f;

    glBegin(GL_QUADS);
    // Front trim
    glVertex3f(-1.0f - trim, -1.0f - trim, 1.0f + trim);
    glVertex3f(1.0f + trim, -1.0f - trim, 1.0f + trim);
    glVertex3f(1.0f + trim, -1.0f, 1.0f + trim);
    glVertex3f(-1.0f - trim, -1.0f, 1.0f + trim);

    // Top trim
    glVertex3f(-1.0f - trim, 1.0f, 1.0f + trim);
    glVertex3f(1.0f + trim, 1.0f, 1.0f + trim);
    glVertex3f(1.0f + trim, 1.0f + trim, 1.0f + trim);
    glVertex3f(-1.0f - trim, 1.0f + trim, 1.0f + trim);
    glEnd();

    // Draw text for ammo type
    if (currentLevel == 2) {
        if (box.isExplosive) {
            // Draw "EXPLOSIVE" text
            glRasterPos3f(-0.8f, 0.0f, 1.02f);
            std::string text = "EXPLOSIVE";
            for (char c : text) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }
        }
        else {
            // Draw "FAST FIRE" text
            glRasterPos3f(-0.6f, 0.0f, 1.02f);
            std::string text = "FAST FIRE";
            for (char c : text) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }
        }
    }

    // Draw bullet symbol
    glBegin(GL_QUADS);
    float symbolSize = 0.3f;
    glVertex3f(-symbolSize, -0.5f, 1.01f);
    glVertex3f(symbolSize, -0.5f, 1.01f);
    glVertex3f(symbolSize, 0.5f, 1.01f);
    glVertex3f(-symbolSize, 0.5f, 1.01f);

    // Draw bullet tip
    glVertex3f(-symbolSize * 0.7f, 0.5f, 1.01f);
    glVertex3f(symbolSize * 0.7f, 0.5f, 1.01f);
    glVertex3f(0.0f, 0.8f, 1.01f);
    glVertex3f(0.0f, 0.8f, 1.01f);
    glEnd();

    // Add highlights with darker color for rusty appearance
    glColor3f(0.7f, 0.7f, 0.7f);  // Darker highlights
    glBegin(GL_LINES);
    glVertex3f(-1.0f, -1.0f, 1.01f);
    glVertex3f(-1.0f, 1.0f, 1.01f);

    glVertex3f(1.0f, -1.0f, 1.01f);
    glVertex3f(1.0f, 1.0f, 1.01f);

    glVertex3f(-1.0f, 1.0f, 1.01f);
    glVertex3f(1.0f, 1.0f, 1.01f);

    glVertex3f(-1.0f, -1.0f, 1.01f);
    glVertex3f(1.0f, -1.0f, 1.01f);
    glEnd();

    glPopMatrix();
}

void CheckGameState() {
    if (gameOver) return;

    // Check win condition for final level
    if (currentLevel == MAX_LEVEL && playerScore >= WINNING_SCORE) {
        gameOver = true;
        playerWon = true;
    }
    // Check level completion and door spawning
    else if (currentLevel < MAX_LEVEL && playerScore >= WINNING_SCORE && !doorSpawned) {
        SpawnDoor();
    }
    // Check door interaction
    else if (doorSpawned && levelDoor.active) {
        float dx = playerX - levelDoor.x;
        float dz = playerZ - levelDoor.z;
        float distanceSquared = dx * dx + dz * dz;

        if (distanceSquared < levelDoor.INTERACTION_RADIUS * levelDoor.INTERACTION_RADIUS) {
            levelCompleted = true;
            currentLevel++;
            playerScore = 0;
            doorSpawned = false;
            levelDoor.active = false;
            SpawnAmmoBoxes();

            // Reset bullets and ammo when entering level 2
            currentAmmo = MAX_AMMO;  // Reset to 5 bullets
            ammoReserves = 0;        // Reset ammo reserves
            isReloading = false;      // Cancel any reload in progress
            currentReloadTime = 0.0f; // Reset reload timer

            // Reset timer for level 2
            gameTimer = GAME_TIME_LIMIT;

            // Reset player position for new level
            playerX = 0.0f;
            playerY = 0.0f;
            playerZ = 0.0f;
            playerRotation = 0.0f;
        }
    }
    // Check lose condition
    else if (gameTimer <= 0 || playerHealth <= 0) {
        gameOver = true;
        playerWon = false;
    }
}
void RenderDoor() {
    if (!doorSpawned || !levelDoor.active) return;

    glPushMatrix();
    glTranslatef(levelDoor.x, levelDoor.y, levelDoor.z);
    glRotatef(levelDoor.rotation, 0, 1, 0);
    glScalef(levelDoor.scale, levelDoor.scale, levelDoor.scale);

    // Door frame
    glColor3f(0.5f, 0.3f, 0.1f);  // Brown color for wooden frame
    glBegin(GL_QUADS);
    // Left post
    glVertex3f(-1.2f, -1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glVertex3f(-1.0f, 2.0f, 0.0f);
    glVertex3f(-1.2f, 2.0f, 0.0f);

    // Right post
    glVertex3f(1.0f, -1.0f, 0.0f);
    glVertex3f(1.2f, -1.0f, 0.0f);
    glVertex3f(1.2f, 2.0f, 0.0f);
    glVertex3f(1.0f, 2.0f, 0.0f);

    // Top beam
    glVertex3f(-1.2f, 2.0f, 0.0f);
    glVertex3f(1.2f, 2.0f, 0.0f);
    glVertex3f(1.2f, 2.2f, 0.0f);
    glVertex3f(-1.2f, 2.2f, 0.0f);
    glEnd();

    // Door itself
    glColor3f(0.3f, 0.2f, 0.1f);  // Darker brown for the door
    glBegin(GL_QUADS);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glVertex3f(1.0f, 2.0f, 0.0f);
    glVertex3f(-1.0f, 2.0f, 0.0f);
    glEnd();

    // Door handle
    glColor3f(0.8f, 0.8f, 0.8f);  // Metallic color
    glPushMatrix();
    glTranslatef(0.7f, 0.0f, 0.1f);
    glBegin(GL_QUADS);
    glVertex3f(-0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, 0.1f, 0.0f);
    glVertex3f(-0.1f, 0.1f, 0.0f);
    glEnd();
    glPopMatrix();

    // Add a glowing effect when player is near
    float dx = playerX - levelDoor.x;
    float dz = playerZ - levelDoor.z;
    float distanceSquared = dx * dx + dz * dz;
    if (distanceSquared < (levelDoor.INTERACTION_RADIUS * 2) * (levelDoor.INTERACTION_RADIUS * 2)) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float alpha = 0.3f * (1.0f - (sqrt(distanceSquared) / (levelDoor.INTERACTION_RADIUS * 2)));
        glColor4f(0.0f, 1.0f, 1.0f, alpha);  // Cyan glow
        glBegin(GL_QUADS);
        glVertex3f(-1.5f, -1.5f, 0.0f);
        glVertex3f(1.5f, -1.5f, 0.0f);
        glVertex3f(1.5f, 2.5f, 0.0f);
        glVertex3f(-1.5f, 2.5f, 0.0f);
        glEnd();
        glDisable(GL_BLEND);
    }

    glPopMatrix();
}

void CheckAmmoBoxCollection() {
    for (auto& box : ammoBoxes) {
        if (!box.active) continue;

        float dx = playerX - box.x;
        float dz = playerZ - box.z;
        float distanceSquared = dx * dx + dz * dz;

        if (distanceSquared < AMMO_BOX_COLLECTION_RADIUS * AMMO_BOX_COLLECTION_RADIUS) {
            box.active = false;

            if (currentLevel == 1) {
                if (box.isHighDamage) {
                    currentBulletDamage = HIGH_DAMAGE;
                    ammoReserves = min(ammoReserves + AMMO_BOX_HIGH_DAMAGE_AMOUNT, MAX_AMMO_RESERVES);
                }
                else {
                    currentBulletDamage = REGULAR_DAMAGE;
                    ammoReserves = min(ammoReserves + AMMO_BOX_REGULAR_AMOUNT, MAX_AMMO_RESERVES);
                }
            }
            else {
                if (box.isExplosive) {
                    currentBulletDamage = EXPLOSIVE_DAMAGE;
                    shootingCooldown = NORMAL_FIRE_COOLDOWN;
                    ammoReserves = min(ammoReserves + AMMO_BOX_HIGH_DAMAGE_AMOUNT, MAX_AMMO_RESERVES);
                }
                else if (box.isFastFire) {
                    currentBulletDamage = REGULAR_DAMAGE;
                    shootingCooldown = FAST_FIRE_COOLDOWN;
                    ammoReserves = min(ammoReserves + AMMO_BOX_REGULAR_AMOUNT, MAX_AMMO_RESERVES);
                }
            }
        }
    }
}

void SpawnTargets() {
    targets.clear();

    if (currentLevel == 1) {
        // Original Level 1 tree-based target spawning
        std::vector<int> availableTrees;
        for (int i = 0; i < treePositions.size(); i++) {
            availableTrees.push_back(i);
        }

        for (int i = 0; i < NUM_TARGETS && !availableTrees.empty(); i++) {
            int randomIndex = rand() % availableTrees.size();
            int treeIndex = availableTrees[randomIndex];
            availableTrees.erase(availableTrees.begin() + randomIndex);

            Target target;
            const SceneObject& tree = treePositions[treeIndex];

            float randomAngle = (rand() % 360) * 3.14159f / 180.0f;
            float offsetRadius = 0.7f;

            target.x = tree.x + cos(randomAngle) * offsetRadius;
            target.y = tree.scale * 3.0f;
            target.z = tree.z + sin(randomAngle) * offsetRadius;
            target.rotation = (atan2(target.z - tree.z, target.x - tree.x) * 180.0f / 3.14159f) + 90;
            target.scale = 0.005f;
            target.active = true;
            target.treeIndex = treeIndex;

            targets.push_back(target);
        }
    }
    else {
        // Level 2: Wall-mounted targets
        std::vector<int> availableWalls;
        for (int i = 0; i < treePositions.size(); i++) {
            availableWalls.push_back(i);
        }

        for (int i = 0; i < NUM_TARGETS && !availableWalls.empty(); i++) {
            int randomIndex = rand() % availableWalls.size();
            int wallIndex = availableWalls[randomIndex];
            availableWalls.erase(availableWalls.begin() + randomIndex);

            Target target;
            const SceneObject& wall = treePositions[wallIndex];

            // Position target on the wall face
            float wallRotation = wall.rotation * 3.14159f / 180.0f;
            float targetHeight = 1.0f + (rand() % 100) / 100.0f * 2.0f; // Random height between 1-3 units

            // Offset from wall center
            float offsetAlongWall = (rand() % 100 - 50) / 100.0f; // -0.5 to 0.5 units

            // Position based on wall rotation
            if (wall.rotation == 0) { // Wall along Z axis
                target.x = wall.x + offsetAlongWall;
                target.z = wall.z + 0.6f; // Slight offset from wall
            }
            else { // Wall along X axis
                target.x = wall.x + 0.6f; // Slight offset from wall
                target.z = wall.z + offsetAlongWall;
            }

            target.y = targetHeight;
            target.rotation = wall.rotation + 90; // Face perpendicular to wall
            target.scale = 0.008f; // Slightly larger for metal targets
            target.active = true;
            target.treeIndex = wallIndex;

            targets.push_back(target);
        }
    }
}

void RenderTarget(const Target& target) {
    if (!target.active) return;

    glPushMatrix();
    glTranslatef(target.x, target.y, target.z);
    glRotatef(target.rotation, 0, 1, 0);

    glEnable(GL_TEXTURE_2D);

    // Use different textures based on level
    if (currentLevel == 1) {
        glBindTexture(GL_TEXTURE_2D, tex_target.texture[0]);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, tex_mtarget.texture[0]);
    }

    // Draw target with adjusted size based on level
    float size = (currentLevel == 1) ? 0.5f : 0.4f;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -size, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -size, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(size, size, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, size, 0.0f);
    glEnd();

    // Add metallic frame for level 2 targets
    if (currentLevel == 2) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.7f, 0.7f, 0.7f); // Metallic color

        float frameWidth = 0.05f;
        glBegin(GL_QUADS);
        // Top frame
        glVertex3f(-size - frameWidth, size, 0.0f);
        glVertex3f(size + frameWidth, size, 0.0f);
        glVertex3f(size + frameWidth, size + frameWidth, 0.0f);
        glVertex3f(-size - frameWidth, size + frameWidth, 0.0f);

        // Bottom frame
        glVertex3f(-size - frameWidth, -size - frameWidth, 0.0f);
        glVertex3f(size + frameWidth, -size - frameWidth, 0.0f);
        glVertex3f(size + frameWidth, -size, 0.0f);
        glVertex3f(-size - frameWidth, -size, 0.0f);

        // Left frame
        glVertex3f(-size - frameWidth, -size - frameWidth, 0.0f);
        glVertex3f(-size, -size - frameWidth, 0.0f);
        glVertex3f(-size, size + frameWidth, 0.0f);
        glVertex3f(-size - frameWidth, size + frameWidth, 0.0f);

        // Right frame
        glVertex3f(size, -size - frameWidth, 0.0f);
        glVertex3f(size + frameWidth, -size - frameWidth, 0.0f);
        glVertex3f(size + frameWidth, size + frameWidth, 0.0f);
        glVertex3f(size, size + frameWidth, 0.0f);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f); // Reset color
    }

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

bool checkCollision(float newX, float newZ) {
    for (const auto& obj : treePositions) {
        float dx = newX - obj.x;
        float dz = newZ - obj.z;

        if (currentLevel == 1) {
            // Level 1: Tree collisions (unchanged)
            float distanceSquared = dx * dx + dz * dz;
            float collisionRadius = TREE_COLLISION_RADIUS * obj.scale;
            if (distanceSquared < collisionRadius * collisionRadius) {
                return true;
            }
        }
        else {
            // Wall collision
            float wallRotation = obj.rotation * 3.14159f / 180.0f;

            // Transform position relative to wall
            float localX = dx * cos(wallRotation) + dz * sin(wallRotation);
            float localZ = -dx * sin(wallRotation) + dz * cos(wallRotation);

            // Use exact same dimensions as in RenderWall
            float wallHalfLength = 4.0f * obj.scale;  // Half of wall length (+-4.0f in render)
            float wallHalfThickness = 0.5f * obj.scale;  // Half of wall thickness (+-0.5f in render)

            // Add small buffer for smoother collision
            float buffer = 0.3f;

            // Simple AABB collision check
            if (abs(localX) < wallHalfLength + buffer &&
                abs(localZ) < wallHalfThickness + buffer) {
                return true;
            }
        }
    }
    return false;
}

void checkRockDamage() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // Only check for damage if enough time has passed since last damage
    if (currentTime - lastDamageTime < DAMAGE_COOLDOWN) {
        return;
    }

    // Check rock collisions
    for (const auto& rock : rockPositions) {
        float dx = playerX - rock.x;
        float dz = playerZ - rock.z;
        float distanceSquared = dx * dx + dz * dz;
        float minDistance = ROCK_COLLISION_RADIUS * rock.scale;

        if (distanceSquared < minDistance * minDistance && playerY <= 0.1f) {
            playGameSounds("damage");  // Move this up, before changing health
            playerHealth -= ROCK_DAMAGE;
            if (playerHealth < 0) {
                playerHealth = 0;
            }
            lastDamageTime = currentTime;
            shakeAmplitude = maxShakeAmplitude;
            break;
        }
    }
}
void CheckBulletTargetCollisions() {
    const float COLLISION_DISTANCE = 0.5f;

    for (auto& bullet : bullets) {
        if (!bullet.active) continue;

        for (auto& target : targets) {
            if (!target.active) continue;

            float dx = bullet.x - target.x;
            float dy = bullet.y - target.y;
            float dz = bullet.z - target.z;
            float distanceSquared = dx * dx + dy * dy + dz * dz;
            float collisionDistanceSquared = COLLISION_DISTANCE * COLLISION_DISTANCE;

            if (distanceSquared <= collisionDistanceSquared) {
                // Create explosion at target position
                CreateExplosion(target.x, target.y, target.z);

                target.active = false;
                bullet.active = false;
                playerScore++;
                playGameSounds("hit");

                bool allTargetsHit = true;
                for (const auto& t : targets) {
                    if (t.active) {
                        allTargetsHit = false;
                        break;
                    }
                }

                if (allTargetsHit) {
                    SpawnTargets();
                }

                return;
            }
        }
    }
}

void UpdateAmmoBoxes() {
    float deltaTime = TIMER_INTERVAL / 1000.0f;

    for (auto& box : ammoBoxes) {
        if (!box.active) {
            box.respawnTimer += deltaTime;
            if (box.respawnTimer >= AMMO_BOX_RESPAWN_TIME) {
                box.active = true;
                box.respawnTimer = 0.0f;
                // Randomize new position
                box.x = (rand() % 60) - 30;
                box.z = (rand() % 60) - 30;
                box.rotation = rand() % 360;
                box.rotationSpeed = 50.0f + (rand() % 50);  // Add this line

            }
        }
    }
}


// Add this function to update explosions
void UpdateExplosions() {
    float deltaTime = TIMER_INTERVAL / 1000.0f;

    for (auto& explosion : explosions) {
        if (!explosion.active) continue;

        explosion.lifetime += deltaTime;
        if (explosion.lifetime >= explosion.maxLifetime) {
            explosion.active = false;
            continue;
        }

        for (auto& particle : explosion.particles) {
            if (!particle.active) continue;

            // Update position
            particle.x += particle.dx * deltaTime;
            particle.y += particle.dy * deltaTime;
            particle.z += particle.dz * deltaTime;

            // Add gravity effect
            particle.dy -= 5.0f * deltaTime;

            // Update lifetime
            particle.lifetime += deltaTime;
            if (particle.lifetime >= particle.maxLifetime) {
                particle.active = false;
            }
        }
    }
}

// Add this function to render explosions
void RenderExplosions() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& explosion : explosions) {
        if (!explosion.active) continue;

        for (const auto& particle : explosion.particles) {
            if (!particle.active) continue;

            // Calculate alpha based on lifetime
            float alpha = 1.0f - (particle.lifetime / particle.maxLifetime);

            glPushMatrix();
            glTranslatef(particle.x, particle.y, particle.z);

            // Make particles always face the camera
            float dx = particle.x - Eye.x;
            float dy = particle.y - Eye.y;
            float dz = particle.z - Eye.z;
            float angle = atan2(dx, dz) * 180.0f / 3.14159f;
            glRotatef(angle, 0, 1, 0);

            glColor4f(particle.r, particle.g, particle.b, alpha);

            // Draw particle as a quad
            glBegin(GL_QUADS);
            glVertex3f(-particle.size, -particle.size, 0);
            glVertex3f(particle.size, -particle.size, 0);
            glVertex3f(particle.size, particle.size, 0);
            glVertex3f(-particle.size, particle.size, 0);
            glEnd();

            glPopMatrix();
        }
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}
void RenderUI() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Health bar background
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(20, 20);
    glVertex2f(220, 20);
    glVertex2f(220, 40);
    glVertex2f(20, 40);
    glEnd();

    // Health bar
    float healthWidth = playerHealth * 2;
    if (playerHealth >= 75) {
        glColor3f(0.0f, 1.0f, 0.0f);
    }
    else if (playerHealth >= 30) {
        glColor3f(1.0f, 1.0f, 0.0f);
    }
    else {
        glColor3f(1.0f, 0.0f, 0.0f);
    }

    glBegin(GL_QUADS);
    glVertex2f(20, 20);
    glVertex2f(20 + healthWidth, 20);
    glVertex2f(20 + healthWidth, 40);
    glVertex2f(20, 40);
    glEnd();

    // Score
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, 60);
    std::string scoreText = "Score: " + std::to_string(playerScore) + "/" + std::to_string(WINNING_SCORE);
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, 120);
    std::string levelText = "Level: " + std::to_string(currentLevel);
    for (char c : levelText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Level completion message
    if (levelCompleted) {
        std::string completionText = "Level " + std::to_string(currentLevel - 1) +
            " Complete! Starting Level " + std::to_string(currentLevel);

        // Calculate text position to center it
        float textWidth = completionText.length() * 9.0f;
        float textX = (WIDTH - textWidth) / 2;
        float textY = HEIGHT / 2;

        glColor3f(0.0f, 1.0f, 0.0f);
        glRasterPos2f(textX, textY);
        for (char c : completionText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        // Reset level completion flag after a short delay
        if (glutGet(GLUT_ELAPSED_TIME) % 3000 > 2000) {  // Show message for 2 seconds
            levelCompleted = false;
        }
    }

    // Ammo display
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, 80);
    std::string ammoText = "Ammo: " + std::to_string(currentAmmo) + "/" + std::to_string(MAX_AMMO);
    ammoText += " (Reserves: " + std::to_string(ammoReserves) + ")";

    // Add ammo type indicator
    if (currentLevel == 2) {
        if (currentBulletDamage == EXPLOSIVE_DAMAGE) {
            ammoText += " [EXPLOSIVE]";
        }
        else if (shootingCooldown == FAST_FIRE_COOLDOWN) {
            ammoText += " [FAST FIRE]";
        }
        else {
            ammoText += " [NORMAL]";
        }
    }
    else {
        ammoText += (currentBulletDamage == HIGH_DAMAGE) ? " [High Damage]" : " [Regular]";
    }

    if (isReloading) {
        ammoText += " (Reloading...)";
    }

    for (char c : ammoText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }


    // Timer display
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, 100);
    int minutes = static_cast<int>(gameTimer) / 60;
    int seconds = static_cast<int>(gameTimer) % 60;
    std::string timerText = "Time: " + std::to_string(minutes) + ":" +
        (seconds < 10 ? "0" : "") + std::to_string(seconds);
    for (char c : timerText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Game over message (centered on screen)
    if (gameOver) {
        RenderGameOverScreen();
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void RenderGround() {
    glDisable(GL_LIGHTING);
    glColor3f(0.6, 0.6, 0.6);
    glEnable(GL_TEXTURE_2D);

    if (currentLevel == 1) {
        glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, tex_map2ground.texture[0]);
    }

    glPushMatrix();
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);
    glVertex3f(-50, 0, -50);
    glTexCoord2f(10, 0);
    glVertex3f(50, 0, -50);
    glTexCoord2f(10, 10);
    glVertex3f(50, 0, 50);
    glTexCoord2f(0, 10);
    glVertex3f(-50, 0, 50);
    glEnd();
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);
}

void InitLightSource() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);  // Main light (Sun/Moon)
    glEnable(GL_LIGHT1);  // Flashlight
    glEnable(GL_LIGHT2);  // Additional light for map 2
    glEnable(GL_LIGHT3);  // Additional light for map 2

    if (currentLevel == 1) {
        // Initial sun setup - will be updated by UpdateSunPosition
        GLfloat sunAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        GLfloat sunDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat sunSpecular[] = { 0.7f, 0.7f, 0.7f, 1.0f };
        GLfloat sunPosition[] = { 0.0f, MAX_SUN_HEIGHT, -SUN_DISTANCE, 1.0f };

        glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
        glLightfv(GL_LIGHT0, GL_POSITION, sunPosition);

        // Disable night-specific lights
        glDisable(GL_LIGHT2);
        glDisable(GL_LIGHT3);
    }
    else {

        // Map 2: Dynamic night setup with multiple light sources

        // Moonlight with subtle color variation
        float moonIntensity = 0.3f + 0.1f * sin(lightAnimationTime * 0.5f);
        GLfloat moonAmbient[] = { 0.1f * moonIntensity, 0.1f * moonIntensity, 0.15f * moonIntensity, 1.0f };
        GLfloat moonDiffuse[] = { 0.2f * moonIntensity, 0.2f * moonIntensity, 0.3f * moonIntensity, 1.0f };
        GLfloat moonSpecular[] = { 0.1f * moonIntensity, 0.1f * moonIntensity, 0.1f * moonIntensity, 1.0f };

        // Animate moon position
        GLfloat moonPosition[] = {
            -50.0f * cos(lightAnimationTime * 0.05f),
            80.0f + 5.0f * sin(lightAnimationTime * 0.1f),
            50.0f * sin(lightAnimationTime * 0.05f),
            1.0f
        };

        glLightfv(GL_LIGHT0, GL_AMBIENT, moonAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, moonDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, moonSpecular);
        glLightfv(GL_LIGHT0, GL_POSITION, moonPosition);

        // Enable and set up rotating spotlights
        glEnable(GL_LIGHT2);
        glEnable(GL_LIGHT3);

        // Light 2: Warm spotlight with color pulsing
        float warmFactor = 0.7f + 0.3f * sin(lightAnimationTime * 2.0f);
        GLfloat lamp1Ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat lamp1Diffuse[] = { 1.0f * warmFactor, 0.8f * warmFactor, 0.4f * warmFactor, 1.0f };
        GLfloat lamp1Specular[] = { 0.5f * warmFactor, 0.4f * warmFactor, 0.2f * warmFactor, 1.0f };

        // Rotating position for first spotlight
        GLfloat lamp1Position[] = {
            20.0f * cos(lampRotationAngle),
            10.0f,
            20.0f * sin(lampRotationAngle),
            1.0f
        };

        GLfloat lamp1Direction[] = { -cos(lampRotationAngle), -1.0f, -sin(lampRotationAngle) };

        glLightfv(GL_LIGHT2, GL_AMBIENT, lamp1Ambient);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, lamp1Diffuse);
        glLightfv(GL_LIGHT2, GL_SPECULAR, lamp1Specular);
        glLightfv(GL_LIGHT2, GL_POSITION, lamp1Position);
        glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, lamp1Direction);
        glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 45.0f);
        glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 2.0f);
        glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05f);

        // Light 3: Cool-toned spotlight with opposite rotation
        float coolFactor = 0.7f + 0.3f * sin(lightAnimationTime * 1.5f + 3.14159f);
        GLfloat lamp2Ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat lamp2Diffuse[] = { 0.4f * coolFactor, 0.4f * coolFactor, 0.8f * coolFactor, 1.0f };
        GLfloat lamp2Specular[] = { 0.2f * coolFactor, 0.2f * coolFactor, 0.4f * coolFactor, 1.0f };

        // Rotating position for second spotlight (opposite direction)
        GLfloat lamp2Position[] = {
            -20.0f * cos(lampRotationAngle * 0.7f),
            10.0f,
            -20.0f * sin(lampRotationAngle * 0.7f),
            1.0f
        };

        GLfloat lamp2Direction[] = { cos(lampRotationAngle * 0.7f), -1.0f, sin(lampRotationAngle * 0.7f) };

        glLightfv(GL_LIGHT3, GL_AMBIENT, lamp2Ambient);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, lamp2Diffuse);
        glLightfv(GL_LIGHT3, GL_SPECULAR, lamp2Specular);
        glLightfv(GL_LIGHT3, GL_POSITION, lamp2Position);
        glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, lamp2Direction);
        glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 45.0f);
        glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 2.0f);
        glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.05f);
    }

    // Dynamic flashlight intensity
    float flashlightPulse = 0.9f + 0.1f * sin(lightAnimationTime * 4.0f);
    GLfloat flashlightAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat flashlightDiffuse[] = { 1.0f * flashlightPulse, 1.0f * flashlightPulse, 0.9f * flashlightPulse, 1.0f };
    GLfloat flashlightSpecular[] = { 1.0f * flashlightPulse, 1.0f * flashlightPulse, 1.0f * flashlightPulse, 1.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, flashlightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, flashlightDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, flashlightSpecular);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, FLASHLIGHT_CUTOFF);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 20.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.05f);
}

void InitMaterial() {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

    GLfloat shininess[] = { 96.0f };
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void myInit(void) {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, aspectRatio, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    InitLightSource();
    InitMaterial();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
}
bool isWithinMapBounds(float x, float z) {
    const float MAP_BOUNDS = 45.0f;  

    return (x >= -MAP_BOUNDS && x <= MAP_BOUNDS &&
        z >= -MAP_BOUNDS && z <= MAP_BOUNDS);
}
void myKeyboard(unsigned char button, int x, int y) {
    if (gameOver) return;
    if (gameOver && button == 27) {  // ESC key
        exit(0);
        return;
    }
    float dx = 0.0f;
    float dz = 0.0f;
    float rotationRad = playerRotation * 3.14159f / 180.0f;

    switch (button) {
    case 'r':
    case 'R':
        StartReload();
        break;
    case 'f':
    case 'F':
        isFirstPerson = true;
        break;
    case 'g':
    case 'G':
        isFirstPerson = false;
        break;
    case 'w':
    case 'W':
        dx = moveSpeed * sin(rotationRad);
        dz = moveSpeed * cos(rotationRad);
        if (playerY >= 0.0f) {
            float newX = playerX + dx;
            float newZ = playerZ + dz;
            if (!checkCollision(newX, newZ) && isWithinMapBounds(newX,newZ)) {
                playerX = newX;
                playerZ = newZ;
            }
        }
        break;
    case 's':
    case 'S':
        dx = moveSpeed * sin(rotationRad);
        dz = moveSpeed * cos(rotationRad);
        if (playerY >= 0.0f) {
            float newX = playerX - dx;
            float newZ = playerZ - dz;
            if (!checkCollision(newX, newZ) && isWithinMapBounds(newX, newZ)) {
                playerX = newX;
                playerZ = newZ;
            }
        }
        break;
    case 'a':
    case 'A':
        playerRotation += 2.0f;
        if (playerRotation >= 360.0f) playerRotation -= 360.0f;
        break;
    case 'd':
    case 'D':
        playerRotation -= 2.0f;
        if (playerRotation < 0.0f) playerRotation += 360.0f;
        break;
    case ' ':
        if (!isJumping && playerY <= 0.0f) {
            isJumping = true;
            jumpVelocity = jumpForce;
            playGameSounds("jump");

        }
        isSpacePressed = true;
        break;
    case 27:
        bullets.clear();
        exit(0);
        break;
    }
}

void myKeyboardUp(unsigned char button, int x, int y) {
    switch (button) {
    case ' ':
        isSpacePressed = false;
        break;
    }
}
void myMotion(int x, int y) {
    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
        return;
    }

    if (isAiming) {
        float xOffset = (x - lastX) * mouseSensitivity;
        float yOffset = (lastY - y) * mouseSensitivity;  // Changed the sign here (removed the minus)

        playerRotation -= xOffset;

        // Keep vertical angle within reasonable limits
        verticalAngle += yOffset;  // Moving mouse up will now look up
        if (verticalAngle > 45.0f) verticalAngle = 45.0f;
        if (verticalAngle < -45.0f) verticalAngle = -45.0f;

        // Normalize rotation
        if (playerRotation >= 360.0f) playerRotation -= 360.0f;
        if (playerRotation < 0.0f) playerRotation += 360.0f;
    }

    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

void myReshape(int w, int h) {
    if (h == 0) h = 1;

    WIDTH = w;
    HEIGHT = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, (GLdouble)WIDTH / (GLdouble)HEIGHT, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RenderBullets() {
    // Set copper/dark yellow color
    glColor3f(0.85f, 0.65f, 0.13f);  // RGB values for copper/dark yellow

    for (const auto& bullet : bullets) {
        if (!bullet.active) continue;

        glPushMatrix();
        glTranslatef(bullet.x, bullet.y, bullet.z);

        float angle = atan2(bullet.dx, bullet.dz) * 180.0f / 3.14159f;
        glRotatef(angle, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        glRotatef(90, 0, 0, 1);

        glScalef(bullet.scale, bullet.scale, bullet.scale);
        bullet_model.Draw();
        glPopMatrix();
    }

    // Reset color back to default
    glColor3f(1.0f, 1.0f, 1.0f);
}

void RenderCrosshair() {
    if (!isAiming) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);

    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;
    int size = 10;
    int thickness = 2;

    // Horizontal line
    glBegin(GL_QUADS);
    glVertex2f(centerX - size, centerY - thickness / 2);
    glVertex2f(centerX + size, centerY - thickness / 2);
    glVertex2f(centerX + size, centerY + thickness / 2);
    glVertex2f(centerX - size, centerY + thickness / 2);
    glEnd();

    // Vertical line
    glBegin(GL_QUADS);
    glVertex2f(centerX - thickness / 2, centerY - size);
    glVertex2f(centerX + thickness / 2, centerY - size);
    glVertex2f(centerX + thickness / 2, centerY + size);
    glVertex2f(centerX - thickness / 2, centerY + size);
    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void updateCamera() {
    float horizontalRad = playerRotation * 3.14159f / 180.0f;
    float verticalRad = verticalAngle * 3.14159f / 180.0f;

    if (isFirstPerson) {
        float eyeHeight = 1.7f;
        Eye.x = playerX;
        Eye.y = playerY + eyeHeight;
        Eye.z = playerZ;

        float lookDistance = 10.0f;
        At.x = playerX + sin(horizontalRad) * lookDistance;
        At.y = playerY + eyeHeight + sin(verticalRad) * lookDistance;
        At.z = playerZ + cos(horizontalRad) * lookDistance;
    }
    else {
        float actualCameraDistance = isAiming ? cameraDistance * 0.3f : cameraDistance;
        float baseHeight = 1.7f;

        float targetX = playerX - actualCameraDistance * sin(horizontalRad);
        float targetZ = playerZ - actualCameraDistance * cos(horizontalRad);

        float targetY;
        if (isAiming) {
            targetY = playerY + baseHeight + actualCameraDistance * sin(verticalRad);
        }
        else {
            targetY = playerY + cameraHeight + actualCameraDistance * sin(verticalRad);
        }

        Eye.x += (targetX - Eye.x) * cameraSmoothness;
        Eye.y += (targetY - Eye.y) * cameraSmoothness;
        Eye.z += (targetZ - Eye.z) * cameraSmoothness;

        if (isAiming) {
            float lookDistance = 10.0f;
            At.x = playerX + sin(horizontalRad) * lookDistance;
            At.y = playerY + baseHeight + sin(verticalRad) * lookDistance;
            At.z = playerZ + cos(horizontalRad) * lookDistance;
        }
        else {
            At.x = playerX;
            At.y = playerY + baseHeight;
            At.z = playerZ;
        }
    }

    // Apply screen shake
    Eye.x += shakeOffsetX;
    Eye.y += shakeOffsetY;
    At.x += shakeOffsetX;
    At.y += shakeOffsetY;
}

void updateScene(int value) {
    if (!gameOver) {
        float deltaTime = TIMER_INTERVAL / 1000.0f;
        
        UpdateLightEffects(deltaTime); 
        UpdateSunPosition(deltaTime);// Add this line
        UpdateAmmoBoxRotations();  // Add this line

        gameTimer -= deltaTime;

        // Check game state
        CheckGameState();

        // Only update game logic if game is not over
        if (!gameOver) {
            // Update jumping physics
            if (isJumping) {
                float nextY = playerY + jumpVelocity;
                jumpVelocity -= gravity;

                if (nextY <= 0.0f) {
                    playerY = 0.0f;
                    isJumping = false;
                    jumpVelocity = 0.0f;
                }
                else {
                    playerY = nextY;
                }
            }
            else {
                if (playerY < 0.0f) {
                    playerY = 0.0f;
                }
            }

            // Update gameplay systems
            updateScreenShake();
            checkRockDamage();
            UpdateBullets();
            UpdateExplosions();
            UpdateReload();
            CheckBulletTargetCollisions();
            UpdateAmmoBoxes();
            CheckAmmoBoxCollection();

            // Process shooting
            if (isShooting) {
                ShootBullet();
            }
        }
    }

    // Camera and FOV updates (continue even if game is over)
    if (isAiming) {
        currentFOV += (zoomFOV - currentFOV) * zoomSpeed;
    }
    else {
        currentFOV += (normalFOV - currentFOV) * zoomSpeed;
    }

    // Update perspective
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(currentFOV, (GLdouble)WIDTH / (GLdouble)HEIGHT, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);

    // Update camera position and view
    updateCamera();

    // Request next frame
    glutPostRedisplay();
    glutTimerFunc(TIMER_INTERVAL, updateScene, 0);
}

void myDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);

    // Update main light
    GLfloat lightIntensity[] = { 0.7, 0.7, 0.7, 1.0f };
    GLfloat lightPosition[] = { 0.0f, 100.0f, 0.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

    // Update and enable/disable flashlight based on level
    if (currentLevel == 2) {
        glEnable(GL_LIGHT1);
        UpdateFlashlight();
        glLightfv(GL_LIGHT1, GL_POSITION, flashlightPosition);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, flashlightDirection);
    }
    else {
        glDisable(GL_LIGHT1);
    }

    RenderGround();

    // Draw trees/walls based on level
    for (const auto& obj : treePositions) {
        if (currentLevel == 1) {
            glPushMatrix();
            glTranslatef(obj.x, 0, obj.z);
            glRotatef(obj.rotation, 0, 1, 0);
            glScalef(obj.scale, obj.scale, obj.scale);
            model_tree.Draw();
            glPopMatrix();
        }
        else {
            RenderWall(obj);
        }
    }

    // Draw rocks
    for (const auto& obj : rockPositions) {
        glPushMatrix();
        if (currentLevel == 1) {
            glColor3f(0.25f, 0.25f, 0.25f);  // Dark grey color for rocks
            glTranslatef(obj.x, 0, obj.z);
            glRotatef(obj.rotation, 0, 1, 0);
            glScalef(obj.scale, obj.scale, obj.scale);
            model_rock.Draw();
        }
        else {
            glColor3f(0.6f, 0.4f, 0.2f);  // Brown color for chairs
            glTranslatef(obj.x, 0.1f, obj.z);  // Lowered y position slightly
            glRotatef(obj.rotation, 0, 1, 0);
            glScalef(obj.scale * 0.15f, obj.scale * 0.15f, obj.scale * 0.15f);  // Much smaller scale for chairs
            Chair.Draw();
        }
        glPopMatrix();
    }

    for (const auto& target : targets) {
        RenderTarget(target);
    }

    // Draw ammo boxes
    for (const auto& box : ammoBoxes) {
        RenderAmmoBox(box);
    }

    // Only draw player model if not in first-person view
    if (!isFirstPerson) {
        glPushMatrix();
        glTranslatef(playerX, playerY, playerZ);
        glRotatef(playerRotation, 0, 1, 0);
        glScalef(playerScale, playerScale, playerScale);
        player_model.Draw();
        glPopMatrix();
    }

    // Draw skybox
    glPushMatrix();
    GLUquadricObj* qobj = gluNewQuadric();
    glTranslated(50, 0, 0);
    glRotated(90, 1, 0, 1);
    if (currentLevel == 1) {
        glBindTexture(GL_TEXTURE_2D, tex);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, tex1);
    }
    gluQuadricTexture(qobj, true);
    gluQuadricNormals(qobj, GL_SMOOTH);
    gluSphere(qobj, 150, 100, 100);
    gluDeleteQuadric(qobj);
    glPopMatrix();
    RenderSun();  // Render the sun after skybox

    // Render remaining game elements
    RenderDoor();
    RenderBullets();
    RenderExplosions();
    RenderUI();
    RenderCrosshair();

    glutSwapBuffers();
}

void myMouse(int button, int state, int x, int y) {
    if (gameOver) return;
    switch (button) {
    case GLUT_LEFT_BUTTON:
        if (state == GLUT_DOWN) {
            isShooting = true;
        }
        else if (state == GLUT_UP) {
            isShooting = false;
        }
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN) {
            isAiming = true;
            firstMouse = true;  // Reset mouse position when starting to aim
        }
        else if (state == GLUT_UP) {
            isAiming = false;
            verticalAngle = 0.0f;  // Reset vertical angle when stopping aim
        }
        break;
    }
    glutPostRedisplay();
}

void LoadAssets() {
    model_tree.Load("Models/tree/Tree1.3ds");
    model_rock.Load("Models/rock/rock.3ds");
    model_wall.Load("Models/wall/wall.3ds");  // Load wall model
    player_model.Load("Models/sniper/sniper.3ds");
    bullet_model.Load("Models/bullet/f715b0fef46a4629b2ddc35614cc727c.3ds");
    tex_rock.Load("textures/rock.bmp");
    tex_target.Load("Textures/target.bmp");
    Chair.Load("Models/chair/chair.3ds");
    tex_mtarget.Load("Textures/metal-target.bmp");
    sunQuadric = gluNewQuadric();
 
    tex_wall.Load("Textures/wall.bmp");  // Make sure you have a wall texture file

    //map1
    tex_ground.Load("Textures/ground.bmp");
    loadBMP(&tex, "Textures/blu-sky-3.bmp", true);
    //map2
    tex_map2ground.Load("Textures/floor.bmp");
    loadBMP(&tex1, "Textures/nightSky.bmp", true);

    if (currentLevel == 1) {
        // Generate random trees for level 1
        for (int i = 0; i < 15; i++) {
            SceneObject tree;
            tree.x = (rand() % 80) - 40;
            tree.z = (rand() % 80) - 40;
            tree.scale = 0.5f + (rand() % 50) / 100.0f;
            tree.rotation = rand() % 360;
            treePositions.push_back(tree);
        }
    }
    else {
        // Generate walls for level 2
        treePositions.clear();

        // Create a maze-like pattern with walls
        for (int i = 0; i < 15; i++) {
            SceneObject wall;
            wall.x = (rand() % 80) - 40;
            wall.z = (rand() % 80) - 40;
            wall.scale = 1.0f;  // Use consistent scale
            wall.rotation = (rand() % 2) * 90;  // Only 0 or 90 degrees

            // Ensure walls aren't too close to spawn point
            if (sqrt(wall.x * wall.x + wall.z * wall.z) < 10.0f) {
                continue;
            }

            // Ensure walls aren't too close to each other
            bool tooClose = false;
            for (const auto& existingWall : treePositions) {
                float dx = wall.x - existingWall.x;
                float dz = wall.z - existingWall.z;
                if (sqrt(dx * dx + dz * dz) < 12.0f) {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose) {
                treePositions.push_back(wall);
            }
        }
    }

    rockPositions.clear();
    if (currentLevel == 1) {
        // Generate rocks for level 1
        for (int i = 0; i < 10; i++) {
            SceneObject rock;
            rock.x = (rand() % 80) - 40;
            rock.z = (rand() % 80) - 40;
            rock.scale = 0.3f + (rand() % 40) / 100.0f;
            rock.rotation = rand() % 360;
            rockPositions.push_back(rock);
        }
    }
    else {
        // Generate chairs for level 2
        for (int i = 0; i < 15; i++) {
            SceneObject chair;
            chair.x = (rand() % 80) - 40;
            chair.z = (rand() % 80) - 40;
            chair.scale = 0.2f + (rand() % 10) / 100.0f;  // Reduced scale range
            chair.rotation = rand() % 360;

            // Ensure chairs aren't too close to spawn point
            if (sqrt(chair.x * chair.x + chair.z * chair.z) < 8.0f) {
                continue;
            }

            rockPositions.push_back(chair);
        }
    }

    SpawnTargets();
    SpawnAmmoBoxes();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitWindowPosition(100, 150);
    glutCreateWindow(title);

    glutDisplayFunc(myDisplay);
    glutKeyboardFunc(myKeyboard);
    glutKeyboardUpFunc(myKeyboardUp);
    glutMotionFunc(myMotion);
    glutMouseFunc(myMouse);
    glutReshapeFunc(myReshape);

    soundSystem = new SimpleSound();
    myInit();
    LoadAssets();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);

    glutTimerFunc(TIMER_INTERVAL, updateScene, 0);
    glutMainLoop();

    return 0;
}
