/**
 * @file FlecsSmokeTest.cpp
 * @brief REAL integration test - compiles or fails
 * 
 * This is NOT documentation. This is REAL CODE that will either:
 * - Compile successfully (proving Flecs works)
 * - Fail to compile (proving Flecs is NOT integrated)
 */

#include <flecs.h>
#include <iostream>

// Simple component
struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

int main() {
    std::cout << "=== FLECS SMOKE TEST ===" << std::endl;
    std::cout << "If this compiles and runs, Flecs is REAL." << std::endl;
    
    // Create world
    flecs::world world;
    std::cout << "✅ Created world" << std::endl;
    
    // Create entity
    auto entity = world.entity("TestEntity")
        .set<Position>({10.0f, 20.0f})
        .set<Velocity>({1.0f, 2.0f});
    
    std::cout << "✅ Created entity with components" << std::endl;
    
    // Query entities
    auto query = world.query<const Position, const Velocity>();
    
    int count = 0;
    query.each([&](const Position& p, const Velocity& v) {
        count++;
        std::cout << "Entity at (" << p.x << ", " << p.y << ")" << std::endl;
        std::cout << "Velocity: (" << v.dx << ", " << v.dy << ")" << std::endl;
    });
    
    std::cout << "✅ Queried " << count << " entities" << std::endl;
    
    // Test hierarchy
    auto parent = world.entity("Parent");
    auto child = world.entity("Child").child_of(parent);
    
    std::cout << "✅ Created hierarchy" << std::endl;
    
    int childCount = 0;
    parent.children([&](flecs::entity e) {
        childCount++;
        std::cout << "Child: " << e.name().c_str() << std::endl;
    });
    
    std::cout << "✅ Found " << childCount << " children" << std::endl;
    
    std::cout << "\n=== ✅ FLECS IS REAL AND WORKING ===" << std::endl;
    std::cout << "Flecs version: " << FLECS_VERSION_MAJOR << "." 
              << FLECS_VERSION_MINOR << "." 
              << FLECS_VERSION_PATCH << std::endl;
    
    return 0;
}
