/*
Professional Auto Attack AI - Enhanced OptAttack
Features:
1. Smart target selection (priority system)
2. Kiting (attack-move-attack)
3. Orb walking
4. Focus fire coordination
5. Threat assessment
6. Escape detection
*/

#include "stdafx.h"
#include "DreamWar3Main.h"
#include <algorithm>
#include <cmath>

namespace ProAutoAttack {

	//=============================================================================
	// CONFIGURATION
	//=============================================================================

	static bool EnableSmartTarget = true;
	static bool EnableKiting = true;
	static bool EnableOrbWalking = true;
	static bool EnableFocusFire = true;
	static float KitingDistance = 50.0f;
	static float SearchRange = 1200.0f;

	//=============================================================================
	// TARGET PRIORITY SYSTEM
	//=============================================================================

	enum TargetPriority {
		PRIORITY_HERO_LOW_HP = 1000,
		PRIORITY_HERO_ESCAPING = 800,
		PRIORITY_HERO_CASTING = 700,
		PRIORITY_HERO_NORMAL = 500,
		PRIORITY_SIEGE_UNIT = 400,
		PRIORITY_RANGED_UNIT = 300,
		PRIORITY_MELEE_UNIT = 200,
		PRIORITY_BUILDING = 100
	};

	struct TargetScore {
		Unit* unit;
		float score;

		bool operator<(const TargetScore& other) const {
			return score > other.score; // Higher score = better target
		}
	};

	//=============================================================================
	// SMART TARGET SELECTION
	//=============================================================================

	float CalculateTargetScore(Unit* attacker, Unit* target) {
		if (!attacker || !target) return -999999.0f;
		if (!target->isEnemyTo(attacker)) return -999999.0f;
		if (target->isDead()) return -999999.0f;

		float score = 0.0f;

		// Base priority by unit type
		if (target->isHero()) {
			score += PRIORITY_HERO_NORMAL;

			// Low HP heroes are high priority
			float hpPercent = target->life() / target->lifeMax();
			if (hpPercent < 0.3f) {
				score += PRIORITY_HERO_LOW_HP * (1.0f - hpPercent);
			}

			// Escaping heroes (moving away)
			Point attackerPos = attacker->position();
			Point targetPos = target->position();
			Point targetVel = target->velocity();

			float dx = targetPos.x - attackerPos.x;
			float dy = targetPos.y - attackerPos.y;
			float distance = sqrt(dx*dx + dy*dy);

			// Check if moving away
			float dotProduct = (dx * targetVel.x + dy * targetVel.y) / distance;
			if (dotProduct > 0 && hpPercent < 0.5f) {
				score += PRIORITY_HERO_ESCAPING;
			}

			// Casting heroes (channeling)
			if (target->isChanneling()) {
				score += PRIORITY_HERO_CASTING;
			}
		}
		else if (target->isSiegeUnit()) {
			score += PRIORITY_SIEGE_UNIT;
		}
		else if (target->isRangedUnit()) {
			score += PRIORITY_RANGED_UNIT;
		}
		else if (target->isMeleeUnit()) {
			score += PRIORITY_MELEE_UNIT;
		}
		else if (target->isBuilding()) {
			score += PRIORITY_BUILDING;
		}

		// Distance penalty (closer = better)
		Point attackerPos = attacker->position();
		Point targetPos = target->position();
		float dx = targetPos.x - attackerPos.x;
		float dy = targetPos.y - attackerPos.y;
		float distance = sqrt(dx*dx + dy*dy);

		score -= distance * 0.5f;

		// Already being attacked by team (focus fire bonus)
		if (EnableFocusFire) {
			int attackerCount = CountAlliesAttacking(attacker, target);
			if (attackerCount > 0) {
				score += attackerCount * 100.0f;
			}
		}

		// In attack range bonus
		if (distance <= attacker->attackRange()) {
			score += 200.0f;
		}

		return score;
	}

	Unit* SelectBestTarget(Unit* attacker) {
		if (!attacker) return nullptr;

		UnitGroup* enemies = GroupUnitsInRange(
			attacker->position(),
			SearchRange,
			[attacker](Unit* u) {
				return u->isEnemyTo(attacker) &&
				       !u->isDead() &&
				       u->isVisible();
			}
		);

		if (!enemies || enemies->size() == 0) {
			GroupDestroy(enemies);
			return nullptr;
		}

		std::vector<TargetScore> scores;

		GroupForEachUnit(enemies, target, {
			TargetScore ts;
			ts.unit = target;
			ts.score = CalculateTargetScore(attacker, target);
			scores.push_back(ts);
		});

		GroupDestroy(enemies);

		if (scores.empty()) return nullptr;

		// Sort by score (highest first)
		std::sort(scores.begin(), scores.end());

		return scores[0].unit;
	}

	//=============================================================================
	// KITING SYSTEM (Attack-Move-Attack)
	//=============================================================================

	void PerformKiting(Unit* attacker, Unit* target) {
		if (!attacker || !target) return;
		if (!EnableKiting) {
			// Normal attack
			attacker->sendAction(ACTION_ATTACK, Target, None, NULL, target->position(), target, true);
			return;
		}

		// Check if attack is on cooldown
		float attackCooldown = attacker->attackCooldownRemaining();

		if (attackCooldown > 0.1f) {
			// Move away from target while waiting for cooldown
			Point attackerPos = attacker->position();
			Point targetPos = target->position();

			float dx = attackerPos.x - targetPos.x;
			float dy = attackerPos.y - targetPos.y;
			float distance = sqrt(dx*dx + dy*dy);

			if (distance > 0) {
				// Move away
				float moveX = attackerPos.x + (dx / distance) * KitingDistance;
				float moveY = attackerPos.y + (dy / distance) * KitingDistance;

				attacker->sendAction(ACTION_MOVE, TargetPoint, None, NULL, Point(moveX, moveY), nullptr, true);
			}
		}
		else {
			// Attack
			attacker->sendAction(ACTION_ATTACK, Target, None, NULL, target->position(), target, true);
		}
	}

	//=============================================================================
	// ORB WALKING (Manual cast + move)
	//=============================================================================

	void PerformOrbWalking(Unit* attacker, Unit* target, Ability* orbAbility) {
		if (!attacker || !target || !orbAbility) return;

		// Cast orb ability
		orbAbility->castOnTarget(target);

		// Immediately move (cancel backswing animation)
		Point attackerPos = attacker->position();
		Point targetPos = target->position();

		float dx = targetPos.x - attackerPos.x;
		float dy = targetPos.y - attackerPos.y;
		float distance = sqrt(dx*dx + dy*dy);

		if (distance > attacker->attackRange() * 0.8f) {
			// Move closer
			attacker->sendAction(ACTION_MOVE, TargetPoint, None, NULL, targetPos, nullptr, true);
		}
	}

	//=============================================================================
	// MAIN AUTO ATTACK LOGIC
	//=============================================================================

	typedef std::map<Unit*, bool> AutoStateMapType;
	static AutoStateMapType StateMap;

	typedef std::map<Unit*, Unit*> TargetMapType;
	static TargetMapType CurrentTargets;

	void AutoStateSet(Unit* u, bool flag) {
		StateMap[u] = flag;
	}

	void ProcessAutoAttack(Unit* attacker) {
		if (!attacker) return;
		if (!StateMap[attacker]) return; // Not in auto mode
		if (attacker->isDead()) return;

		// Check if already attacking
		if (attacker->currentOrder() == ACTION_ATTACK && attacker->orderTarget()) {
			// Continue current attack
			return;
		}

		// Select best target
		Unit* target = SelectBestTarget(attacker);

		if (!target) {
			CurrentTargets[attacker] = nullptr;
			return;
		}

		CurrentTargets[attacker] = target;

		// Check for orb ability
		Ability* orbAbility = attacker->getOrbAbility();

		if (EnableOrbWalking && orbAbility && orbAbility->isReady()) {
			PerformOrbWalking(attacker, target, orbAbility);
		}
		else {
			PerformKiting(attacker, target);
		}
	}

	//=============================================================================
	// EVENT HANDLERS
	//=============================================================================

	void onActionSent(const Event* evt) {
		ActionEventData* data = evt->data<ActionEventData>();

		if (data->byProgramm) return;
		if (data->flag & Concurrent) return;
		if (data->flag & Queued) return;

		// A-click or Patrol = auto mode
		if ((data->id == ACTION_ATTACK || data->id == ACTION_PATROL) && data->target == NULL) {
			UnitGroup* g = GroupUnitsOfPlayerSelected(PlayerLocal(), (data->flag & Subgroup) != 0);
			GroupForEachUnit(g, u, AutoStateSet(u, true););
			GroupDestroy(g);
		}
		// Stop/Hold = auto mode
		else if (data->id == ACTION_STOP || data->id == ACTION_HOLD) {
			UnitGroup* g = GroupUnitsOfPlayerSelected(PlayerLocal(), (data->flag & Subgroup) != 0);
			GroupForEachUnit(g, u, AutoStateSet(u, true););
			GroupDestroy(g);
		}
		// Any other command = manual mode
		else {
			UnitGroup* g = GroupUnitsOfPlayerSelected(PlayerLocal(), (data->flag & Subgroup) != 0);
			GroupForEachUnit(g, u, AutoStateSet(u, false););
			GroupDestroy(g);
		}
	}

	void onGameLoop(const Event* evt) {
		// Process all units in auto mode
		for (auto& pair : StateMap) {
			if (pair.second) { // If in auto mode
				ProcessAutoAttack(pair.first);
			}
		}
	}

	//=============================================================================
	// UI CONFIGURATION
	//=============================================================================

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Pro Auto Attack", NULL);

		CheckBox* cbSmartTarget = new CheckBox(Panel);
		cbSmartTarget->bindProfile("ProAutoAttack", "SmartTarget", true);
		cbSmartTarget->bindVariable(&EnableSmartTarget);
		cbSmartTarget->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		Label* lbSmartTarget = new Label(Panel, "Smart Target Selection", 0.013f);
		lbSmartTarget->setRelativePosition(POS_L, cbSmartTarget, POS_R, 0.01f, 0);

		CheckBox* cbKiting = new CheckBox(Panel);
		cbKiting->bindProfile("ProAutoAttack", "Kiting", true);
		cbKiting->bindVariable(&EnableKiting);
		cbKiting->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.08f);

		Label* lbKiting = new Label(Panel, "Enable Kiting", 0.013f);
		lbKiting->setRelativePosition(POS_L, cbKiting, POS_R, 0.01f, 0);

		CheckBox* cbOrbWalking = new CheckBox(Panel);
		cbOrbWalking->bindProfile("ProAutoAttack", "OrbWalking", true);
		cbOrbWalking->bindVariable(&EnableOrbWalking);
		cbOrbWalking->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.11f);

		Label* lbOrbWalking = new Label(Panel, "Enable Orb Walking", 0.013f);
		lbOrbWalking->setRelativePosition(POS_L, cbOrbWalking, POS_R, 0.01f, 0);

		CheckBox* cbFocusFire = new CheckBox(Panel);
		cbFocusFire->bindProfile("ProAutoAttack", "FocusFire", true);
		cbFocusFire->bindVariable(&EnableFocusFire);
		cbFocusFire->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.14f);

		Label* lbFocusFire = new Label(Panel, "Enable Focus Fire", 0.013f);
		lbFocusFire->setRelativePosition(POS_L, cbFocusFire, POS_R, 0.01f, 0);
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		CreateMenuContent();
		MainDispatcher()->listen(EVENT_LOCAL_ACTION, onActionSent);
		MainDispatcher()->listen(EVENT_GAME_LOOP, onGameLoop);
	}

	void Cleanup() {
		StateMap.clear();
		CurrentTargets.clear();
	}
}
