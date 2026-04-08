// PerfectLastHit.cpp - Advanced Last Hit System
// Improves Dream.mix LastHit with AI prediction

#include <windows.h>
#include <vector>
#include <algorithm>
#include <cmath>

//=============================================================================
// GAME STRUCTURES (Warcraft III memory structures)
//=============================================================================

struct Unit {
    void* vtable;
    // ... other fields
    float x;
    float y;
    float hp;
    float maxHp;
    int unitType;
    int owner;
};

struct Hero {
    Unit base;
    float attackDamageMin;
    float attackDamageMax;
    float attackSpeed;
    float projectileSpeed;
    float attackRange;
};

//=============================================================================
// PERFECT LAST HIT CALCULATOR
//=============================================================================

class PerfectLastHit {
private:
    Hero* myHero;
    std::vector<Unit*> creeps;

    // Calculate exact damage to unit
    float CalculateDamage(Hero* attacker, Unit* target) {
        float baseDamage = (attacker->attackDamageMin + attacker->attackDamageMax) / 2.0f;

        // TODO: Add armor calculation
        // TODO: Add damage type (normal, pierce, siege, etc.)

        return baseDamage;
    }

    // Calculate time for projectile to reach target
    float CalculateProjectileTime(Hero* attacker, Unit* target) {
        if (attacker->projectileSpeed == 0) {
            return 0.0f; // Melee
        }

        float dx = target->x - attacker->base.x;
        float dy = target->y - attacker->base.y;
        float distance = sqrt(dx*dx + dy*dy);

        return distance / attacker->projectileSpeed;
    }

    // Predict when creep will die from other sources
    float PredictDeathTime(Unit* creep) {
        // Check if other units are attacking this creep
        // TODO: Scan for nearby enemy units
        // TODO: Calculate their DPS
        // TODO: Predict death time

        return 999.0f; // No threat
    }

    // Calculate optimal attack time
    float CalculateOptimalAttackTime(Unit* creep) {
        float damage = CalculateDamage(myHero, creep);
        float projectileTime = CalculateProjectileTime(myHero, creep);
        float attackAnimationTime = 0.3f; // TODO: Get from hero data

        // Time when creep will be in last-hit range
        float timeToLastHit = (creep->hp - damage) / 10.0f; // Assuming 10 DPS from other sources

        // Account for projectile travel time
        float optimalTime = timeToLastHit - projectileTime - attackAnimationTime;

        return optimalTime;
    }

public:
    PerfectLastHit(Hero* hero) : myHero(hero) {}

    // Main last hit logic
    Unit* GetBestLastHitTarget() {
        Unit* bestTarget = nullptr;
        float bestScore = -999.0f;

        for (Unit* creep : creeps) {
            if (!creep || creep->hp <= 0) continue;

            float damage = CalculateDamage(myHero, creep);

            // Skip if we can't kill it in one hit
            if (damage < creep->hp * 0.8f) continue;

            // Skip if it's too healthy
            if (creep->hp > damage * 1.5f) continue;

            float optimalTime = CalculateOptimalAttackTime(creep);
            float deathTime = PredictDeathTime(creep);

            // Priority score
            float score = 0.0f;

            // Higher priority for creeps about to die
            if (creep->hp < damage * 1.2f) {
                score += 100.0f;
            }

            // Higher priority for deny (own creeps)
            if (creep->owner == myHero->base.owner) {
                score += 50.0f;
            }

            // Lower priority if someone else will kill it
            if (deathTime < optimalTime) {
                score -= 200.0f;
            }

            // Distance penalty
            float dx = creep->x - myHero->base.x;
            float dy = creep->y - myHero->base.y;
            float distance = sqrt(dx*dx + dy*dy);
            score -= distance * 0.1f;

            if (score > bestScore) {
                bestScore = score;
                bestTarget = creep;
            }
        }

        return bestTarget;
    }

    // Check if we should attack now
    bool ShouldAttackNow(Unit* target) {
        if (!target) return false;

        float damage = CalculateDamage(myHero, target);
        float projectileTime = CalculateProjectileTime(myHero, target);

        // Predict HP when projectile arrives
        float predictedHP = target->hp - (10.0f * projectileTime); // Assuming 10 DPS

        // Attack if predicted HP is in kill range
        return (predictedHP > 0 && predictedHP <= damage * 1.1f);
    }

    // Update creep list
    void UpdateCreeps(std::vector<Unit*> newCreeps) {
        creeps = newCreeps;
    }
};

//=============================================================================
// ADVANCED FEATURES
//=============================================================================

class SmartDeny {
private:
    Hero* myHero;

public:
    SmartDeny(Hero* hero) : myHero(hero) {}

    // Check if we should deny (attack own creep)
    bool ShouldDeny(Unit* creep) {
        if (!creep || creep->owner != myHero->base.owner) {
            return false;
        }

        // Deny when HP < 50%
        float hpPercent = creep->hp / creep->maxHp;
        if (hpPercent > 0.5f) {
            return false;
        }

        // Check if under tower
        // TODO: Detect nearby towers

        return true;
    }
};

class TowerAggro {
public:
    // Check if attacking will draw tower aggro
    static bool WillDrawTowerAggro(Hero* hero, Unit* target) {
        // TODO: Find nearby enemy towers
        // TODO: Check if we're in tower range
        // TODO: Check if attacking enemy hero near tower

        return false;
    }

    // Calculate safe attack window under tower
    static float GetSafeAttackWindow(Hero* hero) {
        // TODO: Calculate tower attack cooldown
        // TODO: Return time window when safe to attack

        return 0.0f;
    }
};

//=============================================================================
// EXPORT FUNCTIONS
//=============================================================================

extern "C" {

PerfectLastHit* g_LastHitAI = nullptr;

__declspec(dllexport) void InitLastHitAI(Hero* hero) {
    if (g_LastHitAI) {
        delete g_LastHitAI;
    }
    g_LastHitAI = new PerfectLastHit(hero);
}

__declspec(dllexport) Unit* GetBestLastHitTarget() {
    if (!g_LastHitAI) return nullptr;
    return g_LastHitAI->GetBestLastHitTarget();
}

__declspec(dllexport) bool ShouldAttackNow(Unit* target) {
    if (!g_LastHitAI) return false;
    return g_LastHitAI->ShouldAttackNow(target);
}

__declspec(dllexport) void UpdateCreepList(Unit** creeps, int count) {
    if (!g_LastHitAI) return;

    std::vector<Unit*> creepList;
    for (int i = 0; i < count; i++) {
        if (creeps[i]) {
            creepList.push_back(creeps[i]);
        }
    }

    g_LastHitAI->UpdateCreeps(creepList);
}

} // extern "C"
