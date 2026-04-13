// Transition system tests
// Verifies: checkTransition threshold, suppressedZone guard, cooldown behavior,
//           and no ping-pong loops using real map data from world_01/world_02.

#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "../src/Math/Types.h"
#include "../src/Map/TransitionZone.h"

// --------------------------------------------------------------------------
// Minimal Map reimplementation (only checkTransition — avoids Renderer/PhysX)
// --------------------------------------------------------------------------
class TestMap
{
public:
    void addTransitionZone(const TransitionZone& zone) { m_zones.push_back(zone); }
    const std::vector<TransitionZone>& getTransitionZones() const { return m_zones; }

    const TransitionZone* checkTransition(glm::vec2 position, glm::vec2 size) const
    {
        for (const auto& zone : m_zones)
        {
            if (zone.bounds.contains(position))
                return &zone;
        }
        return nullptr;
    }

private:
    std::vector<TransitionZone> m_zones;
};

// --------------------------------------------------------------------------
// Simulate the transition state machine from main.cpp
// --------------------------------------------------------------------------
struct TransitionSim
{
    TestMap   mapA;        // source map
    TestMap   mapB;        // destination map
    TestMap*  currentMap = nullptr;
    std::string currentMapStem;   // e.g. "world_01.json"

    glm::vec2 playerPos{0.f, 0.f};
    glm::vec2 playerSize{50.f, 50.f};

    float transitionCooldown = 0.f;
    std::string suppressedZone;   // zone name to suppress until player leaves

    bool  transitionActive   = false;
    float transitionTimer    = 0.f;
    float fadeDuration       = 0.4f;

    enum class FadeState { Idle, FadingOut, FadingIn };
    FadeState fadeState = FadeState::Idle;

    std::string lastFiredTarget;
    int         transitionCount = 0;

    // Spawn data for the arriving side
    glm::vec2 arrivalSpawn{0.f, 0.f};
    TestMap*  arrivalMap = nullptr;
    std::string arrivalSourceStem; // stem of the map we're leaving

    void startTransition(const std::string& targetMap, glm::vec2 spawnPos,
                         TestMap* destMap, const std::string& sourceStem)
    {
        if (fadeState != FadeState::Idle) return;
        lastFiredTarget = targetMap;
        transitionCount++;
        fadeState = FadeState::FadingOut;
        transitionTimer = 0.f;
        arrivalSpawn = spawnPos;
        arrivalMap = destMap;
        arrivalSourceStem = sourceStem;
    }

    static std::string extractStem(const std::string& path)
    {
        auto pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }

    // Simulate one frame of the game loop
    void tick(float dt)
    {
        // --- Transition animation ---
        if (fadeState != FadeState::Idle)
        {
            transitionTimer += dt;
            if (fadeState == FadeState::FadingOut && transitionTimer >= fadeDuration)
            {
                // Midpoint: load new map, reposition player
                currentMap = arrivalMap;
                currentMapStem = extractStem(lastFiredTarget);
                playerPos = arrivalSpawn;
                // Find the return zone and suppress it
                suppressedZone.clear();
                for (const auto& z : currentMap->getTransitionZones())
                {
                    if (extractStem(z.targetMap) == arrivalSourceStem)
                    {
                        suppressedZone = z.name;
                        break;
                    }
                }
                transitionCooldown = 0.3f;
                transitionTimer = 0.f;
                fadeState = FadeState::FadingIn;
            }
            else if (fadeState == FadeState::FadingIn && transitionTimer >= fadeDuration)
            {
                fadeState = FadeState::Idle;
            }
            return; // skip gameplay during transition (the continue in main.cpp)
        }

        // --- Gameplay: transition check ---
        if (transitionCooldown > 0.f)
        {
            transitionCooldown -= dt;
        }
        else
        {
            const TransitionZone* zone = currentMap->checkTransition(playerPos, playerSize);
            if (zone && zone->name != suppressedZone)
            {
                TestMap* dest = (currentMap == &mapA) ? &mapB : &mapA;
                glm::vec2 spawn(zone->targetBaseX, zone->targetBaseY);
                std::string srcStem = currentMapStem;
                startTransition(zone->targetMap, spawn, dest, srcStem);
            }
            else if (!zone)
            {
                suppressedZone.clear();
            }
        }
    }
};

// --------------------------------------------------------------------------
// Helper: build sim with real world_01 ↔ world_02 data
// --------------------------------------------------------------------------
void initWorld01_02Sim(TransitionSim& sim)
{
    // world_01 transition to world_02 (right edge, vertical strip)
    TransitionZone w1_to_w2;
    w1_to_w2.name = "to_world_02";
    w1_to_w2.bounds = Rect(3350.f, -500.f, 50.f, 1200.f);
    w1_to_w2.targetMap = "maps/world_02.json";
    w1_to_w2.targetSpawn = "default";
    w1_to_w2.edgeAxis = "vertical";
    w1_to_w2.targetBaseX = -140.f;
    w1_to_w2.targetBaseY = -500.f;
    sim.mapA.addTransitionZone(w1_to_w2);

    // world_02 transition to world_01 (left edge, vertical strip)
    TransitionZone w2_to_w1;
    w2_to_w1.name = "to_world_01";
    w2_to_w1.bounds = Rect(-200.f, -500.f, 50.f, 1200.f);
    w2_to_w1.targetMap = "maps/world_01.json";
    w2_to_w1.targetSpawn = "default";
    w2_to_w1.edgeAxis = "vertical";
    w2_to_w1.targetBaseX = 3340.f;
    w2_to_w1.targetBaseY = -500.f;
    sim.mapB.addTransitionZone(w2_to_w1);

    sim.currentMap = &sim.mapA;
    sim.currentMapStem = "world_01.json";
    sim.playerPos = {110.f, 240.f}; // world_01 spawn point
}

// --------------------------------------------------------------------------
// Tests
// --------------------------------------------------------------------------
int testsPassed = 0;
int testsFailed = 0;

#define TEST(name) \
    void name(); \
    struct name##_registrar { name##_registrar() { printf("  %-50s ", #name); name(); } } name##_instance; \
    void name()

#define ASSERT(expr) \
    do { if (!(expr)) { printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #expr); testsFailed++; return; } } while(0)

#define PASS() do { printf("PASS\n"); testsPassed++; } while(0)


TEST(checkTransition_returns_null_when_player_outside_zone)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    // Player at spawn (110, 240) — far from zone at x=3350
    const TransitionZone* z = sim.currentMap->checkTransition(sim.playerPos, sim.playerSize);
    ASSERT(z == nullptr);
    PASS();
}

TEST(checkTransition_returns_null_when_player_edge_touches_zone)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    // Player center at 3325 → right edge at 3350 (touching zone left edge)
    sim.playerPos = {3325.f, 240.f};
    const TransitionZone* z = sim.currentMap->checkTransition(sim.playerPos, sim.playerSize);
    ASSERT(z == nullptr);
    PASS();
}

TEST(checkTransition_returns_zone_when_center_inside)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    // Player center at 3350 — exactly on zone left edge (contains uses >=)
    sim.playerPos = {3350.f, 240.f};
    const TransitionZone* z = sim.currentMap->checkTransition(sim.playerPos, sim.playerSize);
    ASSERT(z != nullptr);
    ASSERT(z->name == "to_world_02");
    PASS();
}

TEST(checkTransition_returns_zone_when_center_deep_inside)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3375.f, 240.f}; // center of zone
    const TransitionZone* z = sim.currentMap->checkTransition(sim.playerPos, sim.playerSize);
    ASSERT(z != nullptr);
    PASS();
}

TEST(transition_fires_when_player_walks_into_zone)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3350.f, 240.f};
    sim.tick(1.f / 60.f);
    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.lastFiredTarget == "maps/world_02.json");
    PASS();
}

TEST(no_transition_fires_when_player_outside_zone)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3300.f, 240.f};
    for (int i = 0; i < 60; i++)
        sim.tick(1.f / 60.f);
    ASSERT(sim.transitionCount == 0);
    PASS();
}

TEST(transition_completes_and_player_arrives_in_dest_map)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Tick until transition completes (FadingOut=0.4s + FadingIn=0.4s = 0.8s)
    for (int i = 0; i < 60; i++) // 1 second
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);
    ASSERT(sim.playerPos.x == -140.f); // arrival spawn
    ASSERT(sim.fadeState == TransitionSim::FadeState::Idle);
    PASS();
}

TEST(no_retrigger_after_arrival_player_stationary)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Trigger and complete first transition
    for (int i = 0; i < 60; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);

    // Player stays at arrival position for 2 more seconds — should NOT re-trigger
    for (int i = 0; i < 120; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1); // still just 1
    PASS();
}

TEST(no_retrigger_after_arrival_player_moves_away)
{
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Complete transition to mapB
    for (int i = 0; i < 60; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);

    // Player walks right (away from the return zone at x=-200...-150)
    for (int i = 0; i < 120; i++)
    {
        sim.playerPos.x += 300.f * dt; // 300 px/s rightward
        sim.tick(dt);
    }

    ASSERT(sim.transitionCount == 1);
    PASS();
}

TEST(no_pingpong_player_walks_left_through_arrival_zone)
{
    // This is the critical ping-pong test:
    // Player arrives at x=-140 in mapB, then walks LEFT through the return
    // zone (x=-200...-150). The suppressedZone should prevent re-trigger
    // until the player has fully left the zone first.
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Complete first transition to mapB
    for (int i = 0; i < 60; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);
    ASSERT(sim.playerPos.x == -140.f);

    // Walk left slowly — player will enter the return zone
    // Zone is at x=-200 to x=-150.  Player starts at -140.
    // At 100px/s left, reaches zone edge -150 in 0.1s (but cooldown is 0.3s)
    for (int i = 0; i < 60; i++) // 1 second of walking left
    {
        sim.playerPos.x -= 100.f * dt;
        sim.tick(dt);
    }

    // Player is now at about -140 - 100 = -240 (past the zone)
    ASSERT(sim.transitionCount == 1); // should NOT have re-triggered
    PASS();
}

TEST(player_can_transition_after_leaving_spawn_zone)
{
    // After arriving in mapB, player walks away, then walks BACK into the zone.
    // This should trigger a transition back (intentional, not a bug).
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Complete first transition to mapB
    for (int i = 0; i < 60; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);

    // Walk RIGHT (away from zone) for 1 second
    for (int i = 0; i < 60; i++)
    {
        sim.playerPos.x += 300.f * dt;
        sim.tick(dt);
    }

    // suppressedZone should be cleared by now
    ASSERT(sim.suppressedZone.empty());

    // Now walk LEFT back into the zone (x=-200 to -150)
    // Player is at about -140 + 300 = 160. Walk left to -175 (inside zone).
    sim.playerPos = {-175.f, 240.f};
    sim.tick(dt);

    // Should fire a transition back to mapA
    ASSERT(sim.transitionCount == 2);
    ASSERT(sim.lastFiredTarget == "maps/world_01.json");
    PASS();
}

TEST(full_roundtrip_no_infinite_loop)
{
    // The real scenario: player goes A→B→A and we verify no extra transitions.
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f};
    const float dt = 1.f / 60.f;

    // Transition A→B
    for (int i = 0; i < 60; i++)
        sim.tick(dt);
    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);

    // Walk away from spawn zone, then walk back into zone to trigger B→A
    for (int i = 0; i < 60; i++)
    {
        sim.playerPos.x += 300.f * dt;
        sim.tick(dt);
    }
    // Now walk into the return zone
    sim.playerPos = {-175.f, 240.f};
    sim.tick(dt);
    ASSERT(sim.transitionCount == 2);

    // Let transition B→A complete
    for (int i = 0; i < 60; i++)
        sim.tick(dt);
    ASSERT(sim.currentMap == &sim.mapA);

    // Stay put for 2 seconds — should NOT re-trigger
    for (int i = 0; i < 120; i++)
        sim.tick(dt);
    ASSERT(sim.transitionCount == 2);

    // Walk away
    for (int i = 0; i < 60; i++)
    {
        sim.playerPos.x -= 300.f * dt;
        sim.tick(dt);
    }
    ASSERT(sim.transitionCount == 2);
    PASS();
}

TEST(spawn_inside_zone_does_not_trigger_during_cooldown)
{
    // Simulate arrival where spawn IS inside the return zone
    TransitionSim sim;

    TransitionZone zoneA;
    zoneA.name = "to_B";
    zoneA.bounds = Rect(900.f, 0.f, 100.f, 100.f);
    zoneA.targetMap = "mapB";
    zoneA.targetBaseX = 50.f;  // spawn INSIDE zone B's return
    zoneA.targetBaseY = 50.f;
    sim.mapA.addTransitionZone(zoneA);

    TransitionZone zoneB;
    zoneB.name = "to_A";
    zoneB.bounds = Rect(0.f, 0.f, 100.f, 100.f); // return zone
    zoneB.targetMap = "mapA";
    zoneB.targetBaseX = 950.f;
    zoneB.targetBaseY = 50.f;
    sim.mapB.addTransitionZone(zoneB);

    sim.currentMap = &sim.mapA;
    sim.currentMapStem = "mapA";
    sim.playerPos = {950.f, 50.f}; // inside zone A

    const float dt = 1.f / 60.f;

    // Trigger A→B
    sim.tick(dt);
    ASSERT(sim.transitionCount == 1);

    // Complete transition (player spawns at 50,50 — INSIDE zone B)
    for (int i = 0; i < 60; i++)
        sim.tick(dt);

    ASSERT(sim.currentMap == &sim.mapB);
    ASSERT(sim.playerPos.x == 50.f);

    // Player is inside zone B's return zone. Run for 3 seconds — should NOT trigger.
    for (int i = 0; i < 180; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);

    // Walk out of zone
    sim.playerPos = {200.f, 50.f};
    sim.tick(dt);
    ASSERT(sim.suppressedZone.empty());

    // Walk back in — now it SHOULD trigger
    sim.playerPos = {50.f, 50.f};
    sim.tick(dt);
    ASSERT(sim.transitionCount == 2);
    PASS();
}

TEST(physics_pushes_player_into_return_zone_no_retrigger)
{
    // Simulates PhysX pushing the player INTO the return zone after spawn.
    // The zone-name suppression should prevent re-trigger as long as the
    // player stays inside the suppressed zone.
    TransitionSim sim;
    initWorld01_02Sim(sim);
    sim.playerPos = {3360.f, 240.f}; // inside world_01 → world_02 zone
    const float dt = 1.f / 60.f;

    // Transition fires and completes (arrive at mapB, x=-140)
    for (int i = 0; i < 60; i++)
        sim.tick(dt);
    ASSERT(sim.transitionCount == 1);
    ASSERT(sim.currentMap == &sim.mapB);
    ASSERT(!sim.suppressedZone.empty());

    // PhysX pushes player INTO the return zone (x=-175, inside [-200,-150])
    sim.playerPos = {-175.f, 240.f};

    // Run for 3 seconds — should NOT trigger because zone is suppressed
    for (int i = 0; i < 180; i++)
        sim.tick(dt);

    ASSERT(sim.transitionCount == 1);
    ASSERT(!sim.suppressedZone.empty()); // still suppressed

    // Now walk fully out
    sim.playerPos = {500.f, 240.f};
    sim.tick(dt);
    ASSERT(sim.suppressedZone.empty());

    // Walk back in — NOW should trigger
    sim.playerPos = {-175.f, 240.f};
    sim.tick(dt);
    ASSERT(sim.transitionCount == 2);
    PASS();
}

// --------------------------------------------------------------------------
int main()
{
    printf("Running transition tests...\n\n");
    // Tests auto-register via static constructors above
    printf("\n%d passed, %d failed\n", testsPassed, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
