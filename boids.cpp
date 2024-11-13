//
// By Benjamin Bassett (benonymity) on 9.1.24
//
// https://github.com/benonymity/boids
//

#include "tigr.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include <random>

// #define NS_PRIVATE_IMPLEMENTATION
// #define CA_PRIVATE_IMPLEMENTATION
// #define MTL_PRIVATE_IMPLEMENTATION
// #include "Metal.hpp"

// Boid structure
struct Boid {
    float x, y;
    float dx, dy;
    std::vector<std::pair<float, float>> history;
    TPixel color;
};

// Predator structure
struct Predator {
    float x, y;
    float dx, dy;
    TPixel color;
    bool show;
};

// Constants
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;
int NUM_BOIDS = 100;
const float VISUAL_RANGE = 75.0f;
const float PREDATOR_FEAR_FACTOR = 0.15f; // Factor for boids to avoid predator

// Adjustable parameters (controlled by sliders)
float CENTERING_FACTOR = 0.005f;
float AVOID_FACTOR = 0.05f;
float MATCHING_FACTOR = 0.05f;
float MARGIN = 120.0f;
float TURN_FACTOR = 0.3f;
float SPEED_LIMIT = 7.5f;
float TRAIL_LENGTH = 50.0f;
float HUE = 0.5f;
float SIZE = 3.0f;

// Slider structure
struct Slider {
    float x, y, width, height;
    float minValue, maxValue, currentValue;
    const char* label;
};

// Function prototypes
void initBoids(std::vector<Boid>& boids);
void updateBoid(Boid& boid, const std::vector<Boid>& boids, const std::vector<Predator>& predators);
void drawBoid(Tigr* screen, const Boid& boid);
void drawSlider(Tigr* screen, Slider& slider);
void updateSlider(Slider& slider, int mouseX, int mouseY, bool mouseDown);
void addBoid(std::vector<Boid>& boids, float x, float y);
void handleHotkeys(Tigr* screen, std::vector<Boid>& boids, std::vector<Predator>& predators);
void resetSimulation(std::vector<Boid>& boids, std::vector<Predator>& predators);
void nudgeBoids(Tigr* screen, std::vector<Boid>& boids, float dx, float dy);
TPixel hsvToRgb(float h, float s, float v);
void addPredator(std::vector<Predator>& predators, float x, float y);
void updatePredator(Predator& predator, const std::vector<Boid>& boids, const std::vector<Predator>& predators);
void drawPredator(Tigr* screen, const Predator& predator);

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);


// 
// !!! Highly experimental Metal code below !!!
// 
// // Global Metal objects
// id<MTLDevice> device;
// id<MTLCommandQueue> commandQueue;
// id<MTLBuffer> boidsBuffer;

// const char* shaderSource = R"(
// #include <metal_stdlib>
// using namespace metal;

// struct Boid {
//     float2 position;
//     float2 velocity;
//     // Add other necessary boid properties
// };

// kernel void updateBoidsKernel(
//     device Boid* boids [[ buffer(0) ]],
//     uint id [[ thread_position_in_grid ]],
//     constant float& deltaTime [[ buffer(1) ]],
//     uint numBoids [[ buffer(2) ]]) 
// {
//     // Example logic to update boid position
//     Boid& b = boids[id];
//     b.position += b.velocity * deltaTime;
//     // Add more complex behavior based on your existing C++ logic
// }
// )";

// void setupMetal() {
//     device = MTLCreateSystemDefaultDevice();
//     commandQueue = [device newCommandQueue];
// }

// id<MTLLibrary> compileShader(id<MTLDevice> device, const char* source) {
//     NSError* error = nil;
//     NSString* sourceString = [NSString stringWithUTF8String:source];
//     id<MTLLibrary> library = [device newLibraryWithSource:sourceString options:nil error:&error];
//     if (!library) {
//         NSLog(@"Error compiling shader: %@", error);
//         return nil;
//     }
//     return library;
// }

// id<MTLFunction> getKernelFunction(id<MTLLibrary> library, const char* functionName) {
//     NSString* functionNameString = [NSString stringWithUTF8String:functionName];
//     return [library newFunctionWithName:functionNameString];
// }

// void createBuffers(std::vector<Boid>& boids) {
//     boidsBuffer = [device newBufferWithBytes:boids.data() length:boids.size() * sizeof(Boid) options:MTLResourceStorageModeShared];
// }

// void updateBoidsWithGPU(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, id<MTLBuffer> boidsBuffer, float deltaTime, size_t numBoids) {
//     id<MTLLibrary> library = compileShader(device, shaderSource);
//     id<MTLFunction> function = getKernelFunction(library, "updateBoidsKernel");
//     id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:function error:nil];

//     id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
//     id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

//     [computeEncoder setComputePipelineState:pipelineState];
//     [computeEncoder setBuffer:boidsBuffer offset:0 atIndex:0];
//     [computeEncoder setBytes:&deltaTime length:sizeof(deltaTime) atIndex:1];
//     [computeEncoder setBytes:&numBoids length:sizeof(numBoids) atIndex:2];

//     MTLSize threadgroupSize = MTLSizeMake(256, 1, 1);
//     MTLSize numThreadgroups = MTLSizeMake((numBoids + 255) / 256, 1, 1);
//     [computeEncoder dispatchThreadgroups:numThreadgroups threadsPerThreadgroup:threadgroupSize];
//     [computeEncoder endEncoding];

//     [commandBuffer commit];
//     [commandBuffer waitUntilCompleted];
// }

int main() {
    // Initialize TIGR window
    Tigr* screen = tigrWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Boids Simulation", 1);
    
    // setupMetal();
    // createBuffers(boids);

    // Initialize boids
    std::vector<Boid> boids(NUM_BOIDS);
    initBoids(boids);

    // Initialize predators
    std::vector<Predator> predators(0);

    // Initialize sliders
    std::vector<Slider> sliders = {
        {10, 10, 200, 20, 0.0f, 0.02f, CENTERING_FACTOR, "Centering Factor"},
        {10, 40, 200, 20, 0.0f, 0.2f, AVOID_FACTOR, "Avoid Factor"},
        {10, 70, 200, 20, 0.0f, 0.2f, MATCHING_FACTOR, "Matching Factor"},
        {10, 100, 200, 20, 5.0f, 30.0f, SPEED_LIMIT, "Speed Limit"},
        {10, 130, 200, 20, 0.0f, 100.0f, TRAIL_LENGTH, "Trail Length"},
        {10, 160, 200, 20, 0.0f, 1.0f, HUE, "Color"},
        {10, 190, 200, 20, 50.0f, 400.0f, MARGIN, "Margin"},
        {10, 220, 200, 20, 0.1f, 3.0f, TURN_FACTOR, "Turn Factor"},
        {10, 250, 200, 20, 1.0f, 10.0f, SIZE, "Size"}
    };

    bool lastMouseDown = false;
    bool animationRunning = true;
    bool lastSpaceState = false;

    // Main loop
    while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
        // Clear screen
        tigrClear(screen, tigrRGB(0, 0, 0));

        SCREEN_WIDTH = screen->w;
        SCREEN_HEIGHT = screen->h;

        // Get mouse state
        int mouseX, mouseY, buttons;
        tigrMouse(screen, &mouseX, &mouseY, &buttons);
        bool mouseDown = (buttons & 1) != 0;
        bool rightMouseDown = (buttons & 2) != 0;

        // Handle hotkeys
        handleHotkeys(screen, boids, predators);

        // Check space key to toggle animation
        bool currentSpaceState = tigrKeyDown(screen, TK_SPACE);
        if (currentSpaceState && !lastSpaceState) {
            animationRunning = !animationRunning;
        }
        lastSpaceState = currentSpaceState;

        // Update and draw sliders
        bool onSlider = false;
        for (auto& slider : sliders) {
            updateSlider(slider, mouseX, mouseY, mouseDown);
            drawSlider(screen, slider);
            if (mouseX >= slider.x - 20 && mouseX <= slider.x + slider.width + 20 &&
                mouseY >= slider.y - 20 && mouseY <= slider.y + slider.height + 20) {
                onSlider = true;
            }
        }

        // Draw instructions in the top right corner
        const char* instructions[] = {
            "Hotkeys:",
            "R: Reset simulation",
            "Arrow keys: Nudge boids",
            "Space: Pause/Resume",
            "Left click: Add boid",
            "Right click: Add predator",
            "Esc: Quit"
        };

        int instructionX = SCREEN_WIDTH - 10;  // Start from right edge
        int instructionY = 10;
        TPixel textColor = tigrRGB(255, 255, 255);

        for (const char* instruction : instructions) {
            int textWidth = tigrTextWidth(tfont, instruction);
            tigrPrint(screen, tfont, instructionX - textWidth, instructionY, textColor, instruction);
            instructionY += 20;
        }

        // Add boids on left click/drag
        if (mouseDown && !onSlider) {
            addBoid(boids, mouseX, mouseY);
        }

        // Add predator on right click
        if (rightMouseDown) {
            addPredator(predators, mouseX, mouseY);
        }

        // Update parameters from sliders
        CENTERING_FACTOR = sliders[0].currentValue;
        AVOID_FACTOR = sliders[1].currentValue;
        MATCHING_FACTOR = sliders[2].currentValue;
        SPEED_LIMIT = sliders[3].currentValue;
        TRAIL_LENGTH = sliders[4].currentValue;
        HUE = sliders[5].currentValue;
        MARGIN = sliders[6].currentValue;
        TURN_FACTOR = sliders[7].currentValue;
        SIZE = sliders[8].currentValue;

        // Update boid colors
        TPixel boidColor = hsvToRgb(HUE, 1.0f, 1.0f);
        for (auto& boid : boids) {
            boid.color = boidColor;
        }

        // Update predator color to be opposite of boid color
        float oppositePredatorHue = std::fmod(HUE + 0.5f, 1.0f);  // Add 0.5 to get the opposite hue, wrap around if > 1
        for (auto& predator : predators) {
            predator.color = hsvToRgb(oppositePredatorHue, 1.0f, 1.0f);
        }

        // Update and draw boids
        if (animationRunning) {
            // I need to work out a deltaTime method for the GPU to handle things
            // updateBoidsWithGPU(device, commandQueue, boidsBuffer, deltaTime, boids.size());
            // for now, let's just do it on the CPU
            for (auto& boid : boids) {
                updateBoid(boid, boids, predators);
            }
            for (auto& predator : predators) {
                updatePredator(predator, boids, predators);
            }
        }
        for (auto& boid : boids) {
            drawBoid(screen, boid);
        }
        for (auto& predator : predators) {
            drawPredator(screen, predator);
        }

        // Update display
        tigrUpdate(screen);

        lastMouseDown = mouseDown;
    }

    // Clean up
    tigrFree(screen);
    return 0;
}

void initBoids(std::vector<Boid>& boids) {
    for (auto& boid : boids) {
        boid.x = dis(gen) * SCREEN_WIDTH;
        boid.y = dis(gen) * SCREEN_HEIGHT;
        boid.dx = dis(gen) * 10 - 5;
        boid.dy = dis(gen) * 10 - 5;
        boid.color = hsvToRgb(HUE, 1.0f, 1.0f);
    }
}

void addBoid(std::vector<Boid>& boids, float x, float y) {
    Boid newBoid;
    newBoid.x = x;
    newBoid.y = y;
    newBoid.dx = dis(gen) * 10 - 5;
    newBoid.dy = dis(gen) * 10 - 5;
    newBoid.color = hsvToRgb(HUE, 1.0f, 1.0f);
    boids.push_back(newBoid);
}

void addPredator(std::vector<Predator>& predators, float x, float y) {
    Predator newPredator;
    newPredator.x = x;
    newPredator.y = y;
    newPredator.dx = dis(gen) * 10 - 5;
    newPredator.dy = dis(gen) * 10 - 5;
    predators.push_back(newPredator);
}

float distance(const Boid& b1, const Boid& b2) {
    float dx = b1.x - b2.x;
    float dy = b1.y - b2.y;
    return std::sqrt(dx * dx + dy * dy);
}

float distance(const Predator& p1, const Predator& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

float distance(const Boid& boid, const Predator& predator) {
    float dx = boid.x - predator.x;
    float dy = boid.y - predator.y;
    return std::sqrt(dx * dx + dy * dy);
}

void keepWithinBounds(Boid& boid) {
    if (boid.x < MARGIN) boid.dx += TURN_FACTOR;
    if (boid.x > SCREEN_WIDTH - MARGIN) boid.dx -= TURN_FACTOR;
    if (boid.y < MARGIN) boid.dy += TURN_FACTOR;
    if (boid.y > SCREEN_HEIGHT - MARGIN) boid.dy -= TURN_FACTOR;
}

void flyTowardsCenter(Boid& boid, const std::vector<Boid>& boids) {
    float centerX = 0, centerY = 0;
    int numNeighbors = 0;

    for (const auto& otherBoid : boids) {
        if (distance(boid, otherBoid) < VISUAL_RANGE) {
            centerX += otherBoid.x;
            centerY += otherBoid.y;
            numNeighbors++;
        }
    }

    if (numNeighbors) {
        centerX /= numNeighbors;
        centerY /= numNeighbors;
        boid.dx += (centerX - boid.x) * CENTERING_FACTOR;
        boid.dy += (centerY - boid.y) * CENTERING_FACTOR;
    }
}

void avoidOthers(Boid& boid, const std::vector<Boid>& boids) {
    const float minDistance = 20;
    float moveX = 0, moveY = 0;

    for (const auto& otherBoid : boids) {
        if (&otherBoid != &boid) {
            if (distance(boid, otherBoid) < minDistance) {
                moveX += boid.x - otherBoid.x;
                moveY += boid.y - otherBoid.y;
            }
        }
    }

    boid.dx += moveX * AVOID_FACTOR;
    boid.dy += moveY * AVOID_FACTOR;
}

void avoidPredator(Boid& boid, const std::vector<Predator>& predators) {
    float moveX = 0, moveY = 0;

    for (const auto& predator : predators) {
        if (distance(boid, predator) < VISUAL_RANGE) {
            moveX += boid.x - predator.x;
            moveY += boid.y - predator.y;
        }
    }

    boid.dx += moveX * PREDATOR_FEAR_FACTOR;
    boid.dy += moveY * PREDATOR_FEAR_FACTOR;
}

void matchVelocity(Boid& boid, const std::vector<Boid>& boids) {
    float avgDX = 0, avgDY = 0;
    int numNeighbors = 0;

    for (const auto& otherBoid : boids) {
        if (distance(boid, otherBoid) < VISUAL_RANGE) {
            avgDX += otherBoid.dx;
            avgDY += otherBoid.dy;
            numNeighbors++;
        }
    }

    if (numNeighbors) {
        avgDX /= numNeighbors;
        avgDY /= numNeighbors;
        boid.dx += (avgDX - boid.dx) * MATCHING_FACTOR;
        boid.dy += (avgDY - boid.dy) * MATCHING_FACTOR;
    }
}

void limitSpeed(Boid& boid) {
    float speed = std::sqrt(boid.dx * boid.dx + boid.dy * boid.dy);
    if (speed > SPEED_LIMIT) {
        boid.dx = (boid.dx / speed) * SPEED_LIMIT;
        boid.dy = (boid.dy / speed) * SPEED_LIMIT;
    }
}

void updateBoid(Boid& boid, const std::vector<Boid>& boids, const std::vector<Predator>& predators) {
    flyTowardsCenter(boid, boids);
    avoidOthers(boid, boids);
    avoidPredator(boid, predators);
    matchVelocity(boid, boids);
    limitSpeed(boid);
    keepWithinBounds(boid);

    boid.x += boid.dx;
    boid.y += boid.dy;

    boid.history.push_back({boid.x, boid.y});
    while (boid.history.size() > static_cast<size_t>(TRAIL_LENGTH)) {
        boid.history.erase(boid.history.begin());
    }
}

void updatePredator(Predator& predator, const std::vector<Boid>& boids, const std::vector<Predator>& predators) {
    if (boids.empty()) return;

    // Calculate the center of mass of nearby boids
    float centerX = 0, centerY = 0;
    int nearbyCount = 0;
    float detectionRange = 150.0f; // Adjust this value as needed

    for (const auto& boid : boids) {
        float dist = distance(boid, predator);
        if (dist < detectionRange) {
            centerX += boid.x;
            centerY += boid.y;
            nearbyCount++;
        }
    }

    if (nearbyCount > 0) {
        centerX /= nearbyCount;
        centerY /= nearbyCount;

        float chaseFactor = 0.05f; // Reduced from 0.1f to make predator slower
        predator.dx += (centerX - predator.x) * chaseFactor;
        predator.dy += (centerY - predator.y) * chaseFactor;
    }

    // Add randomness to predator movement
    float randomFactor = 0.3f; // Reduced from 0.5f to make movement less erratic
    predator.dx += (dis(gen) * 2 - 1) * randomFactor;
    predator.dy += (dis(gen) * 2 - 1) * randomFactor;

    // Limit predator speed
    float speed = std::sqrt(predator.dx * predator.dx + predator.dy * predator.dy);
    float predatorSpeedLimit = 3.0f; // Reduced from 10.0f to make predator much slower
    if (speed > predatorSpeedLimit) {
        predator.dx = (predator.dx / speed) * predatorSpeedLimit;
        predator.dy = (predator.dy / speed) * predatorSpeedLimit;
    }

    // Avoid other predators
    const float minDistance = 30.0f;
    for (const auto& otherPredator : predators) {
        if (&otherPredator != &predator) {
            float dist = distance(otherPredator, predator);
            if (dist < minDistance) {
                float avoidFactor = 0.1f;
                predator.dx += (predator.x - otherPredator.x) * avoidFactor;
                predator.dy += (predator.y - otherPredator.y) * avoidFactor;
            }
        }
    }

    // Update predator position
    predator.x += predator.dx;
    predator.y += predator.dy;

    // Keep predator within screen bounds
    if (predator.x < 0) {
        predator.x = 0;
        predator.dx *= -1;
    } else if (predator.x > SCREEN_WIDTH) {
        predator.x = SCREEN_WIDTH;
        predator.dx *= -1;
    }
    if (predator.y < 0) {
        predator.y = 0;
        predator.dy *= -1;
    } else if (predator.y > SCREEN_HEIGHT) {
        predator.y = SCREEN_HEIGHT;
        predator.dy *= -1;
    }
}

void drawBoid(Tigr* screen, const Boid& boid) {
    // Draw the boid as a solid rectangle
    float width = SIZE * 3; // Width of the rectangle (3:1 ratio)
    float height = SIZE; // Height of the rectangle (3:1 ratio)
    
    // Calculate the rotation angle
    float angle = std::atan2(boid.dy, boid.dx);
    
    // Calculate the corners of the rotated rectangle
    float cos_angle = std::cos(angle);
    float sin_angle = std::sin(angle);
    float corners[4][2] = {
        {-width/2, -height/2},
        {width/2, -height/2},
        {width/2, height/2},
        {-width/2, height/2}
    };
    
    for (int i = 0; i < 4; i++) {
        float x = corners[i][0];
        float y = corners[i][1];
        corners[i][0] = x * cos_angle - y * sin_angle + boid.x;
        corners[i][1] = x * sin_angle + y * cos_angle + boid.y;
    }
    
    // Draw the solid rotated rectangle
    for (int y = static_cast<int>(boid.y - height); y <= static_cast<int>(boid.y + height); y++) {
        for (int x = static_cast<int>(boid.x - width); x <= static_cast<int>(boid.x + width); x++) {
            float local_x = (x - boid.x) * cos_angle + (y - boid.y) * sin_angle;
            float local_y = -(x - boid.x) * sin_angle + (y - boid.y) * cos_angle;
            if (local_x >= -width/2 && local_x <= width/2 && local_y >= -height/2 && local_y <= height/2) {
                tigrPlot(screen, x, y, boid.color);
            }
        }
    }

    // Draw trail
    size_t trailSize = boid.history.size();
    for (size_t i = 1; i < trailSize; ++i) {
        TPixel trailColor = boid.color;
        trailColor.a = static_cast<unsigned char>(175 * (i / static_cast<float>(trailSize)));
        tigrLine(screen, 
                 static_cast<int>(boid.history[i-1].first), static_cast<int>(boid.history[i-1].second),
                 static_cast<int>(boid.history[i].first), static_cast<int>(boid.history[i].second),
                 trailColor);
    }
}

void drawPredator(Tigr* screen, const Predator& predator) {
    // Draw the predator as a rectangle with 3:1 ratio
    float width = SIZE * 2 * 3; // Width of the rectangle (3:1 ratio, doubled from SIZE)
    float height = SIZE * 2; // Height of the rectangle (3:1 ratio, doubled from SIZE)
    
    // Calculate the rotation angle
    float angle = std::atan2(predator.dy, predator.dx);
    
    // Calculate the corners of the rotated rectangle
    float cos_angle = std::cos(angle);
    float sin_angle = std::sin(angle);
    float corners[4][2] = {
        {-width/2, -height/2},
        {width/2, -height/2},
        {width/2, height/2},
        {-width/2, height/2}
    };
    
    for (int i = 0; i < 4; i++) {
        float x = corners[i][0];
        float y = corners[i][1];
        corners[i][0] = x * cos_angle - y * sin_angle + predator.x;
        corners[i][1] = x * sin_angle + y * cos_angle + predator.y;
    }
    
    // Draw the solid rotated rectangle
    for (int y = static_cast<int>(predator.y - height); y <= static_cast<int>(predator.y + height); y++) {
        for (int x = static_cast<int>(predator.x - width); x <= static_cast<int>(predator.x + width); x++) {
            float local_x = (x - predator.x) * cos_angle + (y - predator.y) * sin_angle;
            float local_y = -(x - predator.x) * sin_angle + (y - predator.y) * cos_angle;
            if (local_x >= -width/2 && local_x <= width/2 && local_y >= -height/2 && local_y <= height/2) {
                tigrPlot(screen, x, y, predator.color);
            }
        }
    }
}

void drawSlider(Tigr* screen, Slider& slider) {
    // Draw slider background
    tigrFillRect(screen, slider.x, slider.y, slider.width, slider.height, tigrRGB(50, 50, 50));
    
    // Draw slider handle
    float handlePos = slider.x + (slider.currentValue - slider.minValue) / (slider.maxValue - slider.minValue) * slider.width;
    tigrFillRect(screen, handlePos - 5, slider.y, 10, slider.height, tigrRGB(200, 200, 200));
    
    // Draw slider label
    tigrPrint(screen, tfont, slider.x + 5, slider.y + 7, tigrRGB(255, 255, 255), slider.label);
    
    // Draw current value
    char valueText[32];
    snprintf(valueText, sizeof(valueText), "%.3f", slider.currentValue);
    tigrPrint(screen, tfont, slider.x + slider.width + 10, slider.y, tigrRGB(255, 255, 255), valueText);
}

void updateSlider(Slider& slider, int mouseX, int mouseY, bool mouseDown) {
    if (mouseDown && mouseX >= slider.x && mouseX <= slider.x + slider.width &&
        mouseY >= slider.y && mouseY <= slider.y + slider.height) {
        float t = (mouseX - slider.x) / slider.width;
        slider.currentValue = slider.minValue + t * (slider.maxValue - slider.minValue);
        slider.currentValue = std::max(slider.minValue, std::min(slider.maxValue, slider.currentValue));
    }
}

void handleHotkeys(Tigr* screen, std::vector<Boid>& boids, std::vector<Predator>& predators) {
    if (tigrKeyDown(screen, 'R')) {
        resetSimulation(boids, predators);
    }
    if (tigrKeyDown(screen, TK_LEFT)) {
        nudgeBoids(screen, boids, -4, 0);
    }
    if (tigrKeyDown(screen, TK_RIGHT)) {
        nudgeBoids(screen, boids, 4, 0);
    }
    if (tigrKeyDown(screen, TK_UP)) {
        nudgeBoids(screen, boids, 0, -4);
    }
    if (tigrKeyDown(screen, TK_DOWN)) {
        nudgeBoids(screen, boids, 0, 4);
    }
}

void resetSimulation(std::vector<Boid>& boids, std::vector<Predator>& predators) {
    boids.clear();
    boids.resize(0);
    predators.clear();
    predators.resize(0);
}

void nudgeBoids(Tigr* screen, std::vector<Boid>& boids, float dx, float dy) {
    static float windAngle = 0.0f;
    static std::vector<std::pair<float, float>> windParticles;

    // Update wind angle
    windAngle += 0.05f;
    if (windAngle > 2 * M_PI) windAngle -= 2 * M_PI;

    // Calculate wind force
    float windForceX = std::cos(windAngle) * 0.2f + dx;
    float windForceY = std::sin(windAngle) * 0.2f + dy;

    // Apply wind force to boids
    for (auto& boid : boids) {
        boid.dx += windForceX;
        boid.dy += windForceY;
        
        // Add some turbulence
        boid.dx += (dis(gen) - 0.5f) * 0.1f;
        boid.dy += (dis(gen) - 0.5f) * 0.1f;
    }

    // Create new wind particles
    if (windParticles.size() < 100) {
        windParticles.push_back({dis(gen) * SCREEN_WIDTH, dis(gen) * SCREEN_HEIGHT});
    }

    // Update wind particles
    for (auto& particle : windParticles) {
        particle.first += windForceX * 5;
        particle.second += windForceY * 5;

        // Wrap particles around screen
        if (particle.first < 0) particle.first += SCREEN_WIDTH;
        if (particle.first > SCREEN_WIDTH) particle.first -= SCREEN_WIDTH;
        if (particle.second < 0) particle.second += SCREEN_HEIGHT;
        if (particle.second > SCREEN_HEIGHT) particle.second -= SCREEN_HEIGHT;
    }

    // Draw wind particles
    for (const auto& particle : windParticles) {
        float lineLength = 50.0f;  // Adjust this value to change the length of the wind lines
        float endX = particle.first + windForceX * lineLength;
        float endY = particle.second + windForceY * lineLength;
        tigrLine(screen, particle.first, particle.second, endX, endY, tigrRGBA(255, 255, 255, 100));
    }
}

TPixel hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - std::abs(std::fmod(h * 6, 2) - 1));
    float m = v - c;
    float r, g, b;
    if (h < 1.0f/6.0f) {
        r = c; g = x; b = 0;
    } else if (h < 2.0f/6.0f) {
        r = x; g = c; b = 0;
    } else if (h < 3.0f/6.0f) {
        r = 0; g = c; b = x;
    } else if (h < 4.0f/6.0f) {
        r = 0; g = x; b = c;
    } else if (h < 5.0f/6.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    return tigrRGB(
        static_cast<unsigned char>((r + m) * 255),
        static_cast<unsigned char>((g + m) * 255),
        static_cast<unsigned char>((b + m) * 255)
    );
}