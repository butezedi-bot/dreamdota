/*
Vision Hack System
- True sight (see invisible units)
- Extended vision range
- Fog of war removal
- See through trees
*/

#include "stdafx.h"
#include "DreamWar3Main.h"

namespace VisionHack {

	static bool EnableTrueSight = false;
	static bool EnableExtendedVision = false;
	static bool EnableFogRemoval = false;
	static float VisionMultiplier = 2.0f;

	//=============================================================================
	// TRUE SIGHT (See Invisible)
	//=============================================================================

	void onGameLoop(const Event* evt) {
		UnitGroup* myUnits = GroupUnitsOfPlayer(PlayerLocal(), nullptr);

		GroupForEachUnit(myUnits, unit, {
			// True Sight
			if (EnableTrueSight) {
				// Give unit true sight ability
				if (!unit->hasAbility('Atru')) { // True Sight ability
					unit->addAbility('Atru');
				}
			}

			// Extended Vision
			if (EnableExtendedVision) {
				float baseVision = unit->baseVisionRange();
				float targetVision = baseVision * VisionMultiplier;

				if (unit->visionRange() < targetVision) {
					unit->setVisionRange(targetVision);
				}
			}
		});

		GroupDestroy(myUnits);

		// Fog of War Removal
		if (EnableFogRemoval) {
			// Remove fog for local player
			Player* player = PlayerLocal();
			if (player) {
				// Set fog state to visible for entire map
				SetFogStateForPlayer(player, FOG_OF_WAR_VISIBLE, GetEntireMapRect(), true);
			}
		}
	}

	//=============================================================================
	// UI CONFIGURATION
	//=============================================================================

	static CheckBox* CbTrueSight;
	static CheckBox* CbExtendedVision;
	static CheckBox* CbFogRemoval;
	static Slider* SliderVision;
	static Label* LbVision;

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Vision Hack", NULL);

		// True Sight
		CbTrueSight = new CheckBox(Panel);
		CbTrueSight->bindProfile("VisionHack", "TrueSight", false);
		CbTrueSight->bindVariable(&EnableTrueSight);
		CbTrueSight->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		Label* lbTrueSight = new Label(Panel, "True Sight (See Invisible)", 0.013f);
		lbTrueSight->setRelativePosition(POS_L, CbTrueSight, POS_R, 0.01f, 0);

		// Extended Vision
		CbExtendedVision = new CheckBox(Panel);
		CbExtendedVision->bindProfile("VisionHack", "ExtendedVision", false);
		CbExtendedVision->bindVariable(&EnableExtendedVision);
		CbExtendedVision->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.09f);

		Label* lbExtendedTitle = new Label(Panel, "Extended Vision Range", 0.013f);
		lbExtendedTitle->setRelativePosition(POS_L, CbExtendedVision, POS_R, 0.01f, 0);

		LbVision = new Label(Panel, "Multiplier: 2.0x", 0.012f);
		LbVision->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.12f);

		SliderVision = new Slider(Panel);
		SliderVision->setRange(1.0f, 5.0f);
		SliderVision->setValue(VisionMultiplier);
		SliderVision->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.15f);
		SliderVision->setSize(0.15f, 0.02f);

		SliderVision->setOnValueChanged([](float value) {
			VisionMultiplier = value;
			ProfileStoreFloat("VisionHack", "VisionMultiplier", value);

			char text[64];
			sprintf(text, "Multiplier: %.1fx", value);
			LbVision->setText(text);
		});

		// Fog of War Removal
		CbFogRemoval = new CheckBox(Panel);
		CbFogRemoval->bindProfile("VisionHack", "FogRemoval", false);
		CbFogRemoval->bindVariable(&EnableFogRemoval);
		CbFogRemoval->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.19f);

		Label* lbFog = new Label(Panel, "Remove Fog of War (Full Map Vision)", 0.013f);
		lbFog->setRelativePosition(POS_L, CbFogRemoval, POS_R, 0.01f, 0);

		// Info
		Label* lbInfo = new Label(
			Panel,
			"|cff00ff00True Sight reveals invisible units|r\n"
			"|cff00ff00Extended Vision increases sight range|r\n"
			"|cffff0000Fog Removal = Full Maphack!|r",
			0.011f
		);
		lbInfo->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.23f);
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		// Load settings
		EnableTrueSight = ProfileFetchInt("VisionHack", "TrueSight", 0) != 0;
		EnableExtendedVision = ProfileFetchInt("VisionHack", "ExtendedVision", 0) != 0;
		EnableFogRemoval = ProfileFetchInt("VisionHack", "FogRemoval", 0) != 0;
		VisionMultiplier = ProfileFetchFloat("VisionHack", "VisionMultiplier", 2.0f);

		// Create UI
		CreateMenuContent();

		// Event listener
		MainDispatcher()->listen(EVENT_GAME_LOOP, onGameLoop);
	}

	void Cleanup() {
		// Remove true sight from all units
		UnitGroup* myUnits = GroupUnitsOfPlayer(PlayerLocal(), nullptr);

		GroupForEachUnit(myUnits, unit, {
			if (unit->hasAbility('Atru')) {
				unit->removeAbility('Atru');
			}
		});

		GroupDestroy(myUnits);
	}
}
