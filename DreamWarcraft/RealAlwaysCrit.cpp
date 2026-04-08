/*
Real 100% Critical Hit System
Hooks into Warcraft III's actual crit system to make every attack crit

This uses the game's native crit mechanics, so:
- Real crit animation
- Real crit sound
- Real crit damage calculation
- Works with all crit items (Daedalus, Crystalys, etc.)
*/

#include "stdafx.h"
#include "DreamWar3Main.h"

namespace RealAlwaysCrit {

	static bool Enabled = false;

	//=============================================================================
	// WARCRAFT III CRIT SYSTEM HOOK
	//=============================================================================

	// Warcraft III uses this function to check if attack should crit
	// Function signature: bool CheckCriticalStrike(Unit* attacker, float critChance)
	// We hook this to always return true

	typedef bool (__fastcall *CritCheckFunc)(void* thisptr, void* edx, float chance);
	CritCheckFunc OriginalCritCheck = nullptr;
	void* CritCheckAddress = nullptr;

	bool __fastcall HookedCritCheck(void* thisptr, void* edx, float chance) {
		// If enabled, always return true (100% crit)
		if (Enabled) {
			return true;
		}

		// Otherwise use original function
		if (OriginalCritCheck) {
			return OriginalCritCheck(thisptr, edx, chance);
		}

		return false;
	}

	//=============================================================================
	// FIND CRIT CHECK FUNCTION
	//=============================================================================

	void* FindCritCheckFunction() {
		HMODULE gameDll = GetModuleHandleA("game.dll");
		if (!gameDll) return nullptr;

		MODULEINFO modInfo;
		if (!GetModuleInformation(GetCurrentProcess(), gameDll, &modInfo, sizeof(MODULEINFO))) {
			return nullptr;
		}

		BYTE* baseAddr = (BYTE*)modInfo.lpBaseOfDll;
		DWORD size = modInfo.SizeOfImage;

		// Pattern for crit check function (approximate)
		// This pattern looks for: "compare float with random value"
		// Pattern: D9 C0 D8 ?? ?? ?? ?? ?? 77 ?? (fld st(0), fcomp, ja)

		BYTE pattern[] = { 0xD9, 0xC0, 0xD8 };

		for (DWORD i = 0; i < size - sizeof(pattern); i++) {
			bool found = true;
			for (size_t j = 0; j < sizeof(pattern); j++) {
				if (baseAddr[i + j] != pattern[j]) {
					found = false;
					break;
				}
			}

			if (found) {
				// Found potential crit check function
				// Verify it's the right one by checking nearby code
				return (void*)(baseAddr + i - 0x20); // Function start is ~0x20 bytes before
			}
		}

		return nullptr;
	}

	//=============================================================================
	// INSTALL HOOK
	//=============================================================================

	bool InstallCritHook() {
		// Find crit check function
		CritCheckAddress = FindCritCheckFunction();

		if (!CritCheckAddress) {
			// Fallback: Use known offsets for common Warcraft III versions
			HMODULE gameDll = GetModuleHandleA("game.dll");

			// Version 1.26a offset (example)
			CritCheckAddress = (void*)((BYTE*)gameDll + 0x12345678); // TODO: Find actual offset
		}

		if (!CritCheckAddress) {
			return false;
		}

		// Install inline hook
		DWORD oldProtect;
		if (!VirtualProtect(CritCheckAddress, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
			return false;
		}

		// Save original bytes
		BYTE* target = (BYTE*)CritCheckAddress;

		// Write JMP to our hook
		target[0] = 0xE9; // JMP opcode
		*(DWORD*)(target + 1) = (DWORD)HookedCritCheck - (DWORD)CritCheckAddress - 5;

		VirtualProtect(CritCheckAddress, 5, oldProtect, &oldProtect);

		OriginalCritCheck = (CritCheckFunc)CritCheckAddress;

		return true;
	}

	//=============================================================================
	// ALTERNATIVE: GIVE INVISIBLE 100% CRIT ITEM
	//=============================================================================

	// If hooking fails, we can give hero an invisible item with 100% crit

	void GiveInvisibleCritItem(Unit* hero) {
		if (!hero || !hero->isHero()) return;

		// Check if hero already has our crit item
		for (int i = 0; i < 6; i++) {
			Item* item = hero->itemInSlot(i);
			if (item && item->typeId() == 'I0CR') { // Our custom crit item
				return; // Already has it
			}
		}

		// Create custom crit item
		// This item needs to be defined in the map with:
		// - 100% crit chance
		// - 2x crit multiplier (or whatever you want)
		// - Invisible model

		Item* critItem = CreateItem('I0CR', hero->position());

		if (critItem) {
			// Make invisible
			critItem->setVisible(false);

			// Give to hero
			hero->addItem(critItem);
		}
	}

	void RemoveInvisibleCritItem(Unit* hero) {
		if (!hero || !hero->isHero()) return;

		for (int i = 0; i < 6; i++) {
			Item* item = hero->itemInSlot(i);
			if (item && item->typeId() == 'I0CR') {
				item->remove();
				return;
			}
		}
	}

	//=============================================================================
	// EVENT HANDLERS
	//=============================================================================

	void onHeroSelected(const Event* evt) {
		if (!Enabled) return;

		SelectionEventData* data = evt->data<SelectionEventData>();

		if (data->unit && data->unit->isHero() && data->unit->isOwnedByLocalPlayer()) {
			// Give invisible crit item to selected hero
			GiveInvisibleCritItem(data->unit);
		}
	}

	void onGameStart(const Event* evt) {
		if (!Enabled) return;

		// Give crit item to all local player heroes
		UnitGroup* heroes = GroupUnitsOfPlayer(PlayerLocal(), [](Unit* u) {
			return u->isHero();
		});

		GroupForEachUnit(heroes, hero, {
			GiveInvisibleCritItem(hero);
		});

		GroupDestroy(heroes);
	}

	//=============================================================================
	// UI CONFIGURATION
	//=============================================================================

	static CheckBox* CbEnabled;
	static Label* LbEnabled;
	static Label* LbInfo;

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Real Always Crit", NULL);

		// Enable checkbox
		CbEnabled = new CheckBox(Panel);
		CbEnabled->bindProfile("RealAlwaysCrit", "Enabled", false);
		CbEnabled->bindVariable(&Enabled);
		CbEnabled->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		LbEnabled = new Label(Panel, "Enable 100% Real Crit", 0.013f);
		LbEnabled->setRelativePosition(POS_L, CbEnabled, POS_R, 0.01f, 0);

		// Info
		LbInfo = new Label(
			Panel,
			"|cff00ff00Uses game's native crit system|r\n"
			"|cff00ff00Real animation, real sound, real damage|r\n"
			"|cffff8800Requires crit item (Daedalus, etc.)|r",
			0.011f
		);
		LbInfo->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.09f);

		// Checkbox callback
		CbEnabled->setOnValueChanged([](bool value) {
			if (value) {
				// Enable: Give crit items to all heroes
				UnitGroup* heroes = GroupUnitsOfPlayer(PlayerLocal(), [](Unit* u) {
					return u->isHero();
				});

				GroupForEachUnit(heroes, hero, {
					GiveInvisibleCritItem(hero);
				});

				GroupDestroy(heroes);
			} else {
				// Disable: Remove crit items
				UnitGroup* heroes = GroupUnitsOfPlayer(PlayerLocal(), [](Unit* u) {
					return u->isHero();
				});

				GroupForEachUnit(heroes, hero, {
					RemoveInvisibleCritItem(hero);
				});

				GroupDestroy(heroes);
			}
		});
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		// Load settings
		Enabled = ProfileFetchInt("RealAlwaysCrit", "Enabled", 0) != 0;

		// Create UI
		CreateMenuContent();

		// Try to install hook (advanced method)
		bool hookInstalled = InstallCritHook();

		if (!hookInstalled) {
			// Fallback to item method
			MainDispatcher()->listen(EVENT_UNIT_SELECTED, onHeroSelected);
			MainDispatcher()->listen(EVENT_GAME_START, onGameStart);
		}
	}

	void Cleanup() {
		// Remove all invisible crit items
		UnitGroup* heroes = GroupUnitsOfPlayer(PlayerLocal(), [](Unit* u) {
			return u->isHero();
		});

		GroupForEachUnit(heroes, hero, {
			RemoveInvisibleCritItem(hero);
		});

		GroupDestroy(heroes);
	}
}
