/*
Super Power System
- Invulnerability (cannot take damage)
- Infinite HP/Mana
- No cooldowns
- Infinite gold
- One-hit kill
*/

#include "stdafx.h"
#include "DreamWar3Main.h"

namespace SuperPower {

	static bool EnableInvulnerable = false;
	static bool EnableInfiniteHP = false;
	static bool EnableInfiniteMana = false;
	static bool EnableNoCooldown = false;
	static bool EnableInfiniteGold = false;
	static bool EnableOneHitKill = false;

	//=============================================================================
	// INVULNERABILITY
	//=============================================================================

	void onDamageReceived(const Event* evt) {
		if (!EnableInvulnerable) return;

		DamageEventData* data = evt->data<DamageEventData>();

		// Block all damage to local player units
		if (data->target && data->target->isOwnedByLocalPlayer()) {
			data->damage = 0.0f;
			data->discard();
		}
	}

	//=============================================================================
	// INFINITE HP/MANA
	//=============================================================================

	void onGameLoop(const Event* evt) {
		UnitGroup* myUnits = GroupUnitsOfPlayer(PlayerLocal(), nullptr);

		GroupForEachUnit(myUnits, unit, {
			// Infinite HP
			if (EnableInfiniteHP && unit->life() < unit->lifeMax()) {
				unit->setLife(unit->lifeMax());
			}

			// Infinite Mana
			if (EnableInfiniteMana && unit->mana() < unit->manaMax()) {
				unit->setMana(unit->manaMax());
			}

			// No Cooldown
			if (EnableNoCooldown) {
				for (int i = 0; i < unit->abilityCount(); i++) {
					Ability* ability = unit->abilityByIndex(i);
					if (ability && ability->cooldownRemaining() > 0) {
						ability->setCooldown(0.0f);
					}
				}

				// Reset attack cooldown
				if (unit->attackCooldownRemaining() > 0) {
					unit->setAttackCooldown(0.0f);
				}

				// Reset item cooldowns
				for (int i = 0; i < 6; i++) {
					Item* item = unit->itemInSlot(i);
					if (item && item->cooldownRemaining() > 0) {
						item->setCooldown(0.0f);
					}
				}
			}
		});

		GroupDestroy(myUnits);

		// Infinite Gold
		if (EnableInfiniteGold) {
			Player* player = PlayerLocal();
			if (player && player->gold() < 999999) {
				player->setGold(999999);
			}
		}
	}

	//=============================================================================
	// ONE HIT KILL
	//=============================================================================

	void onDamageDealt(const Event* evt) {
		if (!EnableOneHitKill) return;

		DamageEventData* data = evt->data<DamageEventData>();

		// If our unit is attacking
		if (data->source && data->source->isOwnedByLocalPlayer()) {
			// Set damage to target's max HP (instant kill)
			if (data->target) {
				data->damage = data->target->lifeMax() * 10.0f; // Overkill
			}
		}
	}

	//=============================================================================
	// UI CONFIGURATION
	//=============================================================================

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Super Power", NULL);

		// Invulnerable
		CheckBox* cbInvuln = new CheckBox(Panel);
		cbInvuln->bindProfile("SuperPower", "Invulnerable", false);
		cbInvuln->bindVariable(&EnableInvulnerable);
		cbInvuln->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		Label* lbInvuln = new Label(Panel, "Invulnerable (No Damage)", 0.013f);
		lbInvuln->setRelativePosition(POS_L, cbInvuln, POS_R, 0.01f, 0);

		// Infinite HP
		CheckBox* cbHP = new CheckBox(Panel);
		cbHP->bindProfile("SuperPower", "InfiniteHP", false);
		cbHP->bindVariable(&EnableInfiniteHP);
		cbHP->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.08f);

		Label* lbHP = new Label(Panel, "Infinite HP", 0.013f);
		lbHP->setRelativePosition(POS_L, cbHP, POS_R, 0.01f, 0);

		// Infinite Mana
		CheckBox* cbMana = new CheckBox(Panel);
		cbMana->bindProfile("SuperPower", "InfiniteMana", false);
		cbMana->bindVariable(&EnableInfiniteMana);
		cbMana->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.11f);

		Label* lbMana = new Label(Panel, "Infinite Mana", 0.013f);
		lbMana->setRelativePosition(POS_L, cbMana, POS_R, 0.01f, 0);

		// No Cooldown
		CheckBox* cbCD = new CheckBox(Panel);
		cbCD->bindProfile("SuperPower", "NoCooldown", false);
		cbCD->bindVariable(&EnableNoCooldown);
		cbCD->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.14f);

		Label* lbCD = new Label(Panel, "No Cooldowns", 0.013f);
		lbCD->setRelativePosition(POS_L, cbCD, POS_R, 0.01f, 0);

		// Infinite Gold
		CheckBox* cbGold = new CheckBox(Panel);
		cbGold->bindProfile("SuperPower", "InfiniteGold", false);
		cbGold->bindVariable(&EnableInfiniteGold);
		cbGold->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.17f);

		Label* lbGold = new Label(Panel, "Infinite Gold (999999)", 0.013f);
		lbGold->setRelativePosition(POS_L, cbGold, POS_R, 0.01f, 0);

		// One Hit Kill
		CheckBox* cbOneHit = new CheckBox(Panel);
		cbOneHit->bindProfile("SuperPower", "OneHitKill", false);
		cbOneHit->bindVariable(&EnableOneHitKill);
		cbOneHit->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.20f);

		Label* lbOneHit = new Label(Panel, "One Hit Kill", 0.013f);
		lbOneHit->setRelativePosition(POS_L, cbOneHit, POS_R, 0.01f, 0);

		// Warning
		Label* lbWarn = new Label(
			Panel,
			"|cffff0000WARNING: Single player only!|r",
			0.012f
		);
		lbWarn->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.24f);
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		// Load settings
		EnableInvulnerable = ProfileFetchInt("SuperPower", "Invulnerable", 0) != 0;
		EnableInfiniteHP = ProfileFetchInt("SuperPower", "InfiniteHP", 0) != 0;
		EnableInfiniteMana = ProfileFetchInt("SuperPower", "InfiniteMana", 0) != 0;
		EnableNoCooldown = ProfileFetchInt("SuperPower", "NoCooldown", 0) != 0;
		EnableInfiniteGold = ProfileFetchInt("SuperPower", "InfiniteGold", 0) != 0;
		EnableOneHitKill = ProfileFetchInt("SuperPower", "OneHitKill", 0) != 0;

		// Create UI
		CreateMenuContent();

		// Event listeners
		MainDispatcher()->listen(EVENT_UNIT_DAMAGE_RECEIVED, onDamageReceived);
		MainDispatcher()->listen(EVENT_UNIT_DAMAGE_DEALT, onDamageDealt);
		MainDispatcher()->listen(EVENT_GAME_LOOP, onGameLoop);
	}

	void Cleanup() {
		// Nothing to cleanup
	}
}
