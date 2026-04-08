/*
Always Critical Hit System
Makes every attack a critical hit (like having 100% crit chance)

Methods:
1. Damage Multiplier - Multiply all damage by 2x-3x
2. Pseudo-Random Distribution Override - Force crit on every hit
3. Item Simulation - Give invisible crit item
*/

#include "stdafx.h"
#include "DreamWar3Main.h"

namespace AlwaysCrit {

	static bool Enabled = false;
	static float CritMultiplier = 2.0f; // 2x damage (normal crit)

	//=============================================================================
	// METHOD 1: DAMAGE MULTIPLIER (Simplest)
	//=============================================================================

	void onDamageDealt(const Event* evt) {
		if (!Enabled) return;

		DamageEventData* data = evt->data<DamageEventData>();

		// Only for local player's units
		if (!data->source || !data->source->isOwnedByLocalPlayer()) return;

		// Only for attack damage (not spell damage)
		if (data->damageType != DAMAGE_TYPE_NORMAL) return;

		// Multiply damage
		data->damage *= CritMultiplier;

		// Show crit text
		ShowFloatingText(
			data->target->position(),
			"CRITICAL!",
			255, 0, 0, // Red color
			0.03f,     // Size
			2.0f       // Duration
		);
	}

	//=============================================================================
	// METHOD 2: PSEUDO-RANDOM OVERRIDE (Advanced)
	//=============================================================================

	// Warcraft III uses PRD (Pseudo-Random Distribution) for crit
	// We can hook into this and force it to always return true

	typedef bool (*PRDCheckFunc)(Unit* unit, float chance);
	PRDCheckFunc OriginalPRDCheck = nullptr;

	bool __stdcall HookedPRDCheck(Unit* unit, float chance) {
		// If it's our unit and checking for crit
		if (Enabled && unit && unit->isOwnedByLocalPlayer()) {
			return true; // Always crit!
		}

		// Otherwise use original function
		return OriginalPRDCheck ? OriginalPRDCheck(unit, chance) : false;
	}

	void InstallPRDHook() {
		// Find PRD function in game.dll
		// Address varies by Warcraft III version
		// This is a placeholder - actual address needs to be found

		HMODULE gameDll = GetModuleHandleA("game.dll");
		if (!gameDll) return;

		// TODO: Find actual PRD function address
		// For now, use Method 1 (damage multiplier)
	}

	//=============================================================================
	// METHOD 3: INVISIBLE CRIT ITEM (Most Realistic)
	//=============================================================================

	void GiveInvisibleCritItem(Unit* hero) {
		if (!hero || !hero->isHero()) return;

		// Create invisible item with 100% crit
		// This makes the game engine handle crit naturally

		Item* critItem = CreateItem(
			'I000', // Custom item ID (needs to be created in map)
			hero->position()
		);

		if (critItem) {
			// Make item invisible
			critItem->setVisible(false);

			// Give to hero
			hero->addItem(critItem);
		}
	}

	//=============================================================================
	// METHOD 4: ATTACK DAMAGE HOOK (Most Reliable)
	//=============================================================================

	typedef float (*GetAttackDamageFunc)(Unit* attacker, Unit* target);
	GetAttackDamageFunc OriginalGetAttackDamage = nullptr;

	float __stdcall HookedGetAttackDamage(Unit* attacker, Unit* target) {
		float baseDamage = OriginalGetAttackDamage ?
			OriginalGetAttackDamage(attacker, target) : 0.0f;

		// If enabled and our unit
		if (Enabled && attacker && attacker->isOwnedByLocalPlayer()) {
			// Show crit effect
			CreateEffect(
				"Abilities\\Spells\\Other\\Stampede\\StampedeMissileDeath.mdl",
				target->position()
			);

			// Play crit sound
			PlaySound("Sound\\Interface\\PickUpItem.wav");

			return baseDamage * CritMultiplier;
		}

		return baseDamage;
	}

	//=============================================================================
	// VISUAL EFFECTS
	//=============================================================================

	void ShowCritEffect(Unit* target) {
		if (!target) return;

		// Red damage numbers
		ShowFloatingText(
			target->position(),
			"CRITICAL HIT!",
			255, 0, 0,
			0.04f,
			2.5f
		);

		// Crit effect model
		CreateEffect(
			"Abilities\\Spells\\Other\\Stampede\\StampedeMissileDeath.mdl",
			target->position()
		);

		// Screen shake (if enabled)
		if (ProfileFetchInt("AlwaysCrit", "ScreenShake", 1)) {
			CameraShake(0.5f, 0.3f);
		}
	}

	//=============================================================================
	// CONFIGURATION
	//=============================================================================

	void SetCritMultiplier(float multiplier) {
		CritMultiplier = std::max(1.0f, std::min(10.0f, multiplier));
		ProfileStoreFloat("AlwaysCrit", "Multiplier", CritMultiplier);
	}

	float GetCritMultiplier() {
		return CritMultiplier;
	}

	//=============================================================================
	// UI MENU
	//=============================================================================

	static CheckBox* CbEnabled;
	static Label* LbEnabled;
	static Slider* SliderMultiplier;
	static Label* LbMultiplier;

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Always Crit", NULL);

		// Enable checkbox
		CbEnabled = new CheckBox(Panel);
		CbEnabled->bindProfile("AlwaysCrit", "Enabled", false);
		CbEnabled->bindVariable(&Enabled);
		CbEnabled->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		LbEnabled = new Label(Panel, "Enable Always Crit", 0.013f);
		LbEnabled->setRelativePosition(POS_L, CbEnabled, POS_R, 0.01f, 0);

		// Multiplier slider
		LbMultiplier = new Label(Panel, "Crit Multiplier: 2.0x", 0.013f);
		LbMultiplier->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.09f);

		SliderMultiplier = new Slider(Panel);
		SliderMultiplier->setRange(1.0f, 10.0f);
		SliderMultiplier->setValue(CritMultiplier);
		SliderMultiplier->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.12f);
		SliderMultiplier->setSize(0.15f, 0.02f);

		SliderMultiplier->setOnValueChanged([](float value) {
			SetCritMultiplier(value);
			char text[64];
			sprintf(text, "Crit Multiplier: %.1fx", value);
			LbMultiplier->setText(text);
		});

		// Info label
		Label* LbInfo = new Label(
			Panel,
			"|cffff8800Warning: High multiplier may be detected!|r",
			0.011f
		);
		LbInfo->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.16f);
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		// Load settings
		Enabled = ProfileFetchInt("AlwaysCrit", "Enabled", 0) != 0;
		CritMultiplier = ProfileFetchFloat("AlwaysCrit", "Multiplier", 2.0f);

		// Create UI
		CreateMenuContent();

		// Hook damage event (Method 1 - simplest and most reliable)
		MainDispatcher()->listen(EVENT_UNIT_DAMAGE, onDamageDealt);

		// TODO: Install PRD hook (Method 2) if needed
		// InstallPRDHook();
	}

	void Cleanup() {
		// Nothing to cleanup
	}

	//=============================================================================
	// EXPORT FUNCTIONS
	//=============================================================================

	extern "C" {
		__declspec(dllexport) void EnableAlwaysCrit(bool enable) {
			Enabled = enable;
		}

		__declspec(dllexport) void SetCritDamage(float multiplier) {
			SetCritMultiplier(multiplier);
		}

		__declspec(dllexport) bool IsAlwaysCritEnabled() {
			return Enabled;
		}
	}
}
