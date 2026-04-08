/*
Speed Hack System
- Movement speed boost (up to 522 max)
- Attack speed boost (up to 5.0x)
- Cast animation speed
- Turn rate boost
*/

#include "stdafx.h"
#include "DreamWar3Main.h"

namespace SpeedHack {

	static bool EnableMovementSpeed = false;
	static bool EnableAttackSpeed = false;
	static bool EnableCastSpeed = false;
	static float MovementSpeedMultiplier = 2.0f;
	static float AttackSpeedMultiplier = 2.0f;

	//=============================================================================
	// SPEED BOOST
	//=============================================================================

	void onGameLoop(const Event* evt) {
		UnitGroup* myUnits = GroupUnitsOfPlayer(PlayerLocal(), nullptr);

		GroupForEachUnit(myUnits, unit, {
			// Movement Speed
			if (EnableMovementSpeed) {
				float baseSpeed = unit->baseMovementSpeed();
				float targetSpeed = std::min(522.0f, baseSpeed * MovementSpeedMultiplier);

				if (unit->movementSpeed() < targetSpeed) {
					unit->setMovementSpeed(targetSpeed);
				}
			}

			// Attack Speed
			if (EnableAttackSpeed) {
				float baseAS = unit->baseAttackSpeed();
				float targetAS = baseAS * AttackSpeedMultiplier;

				if (unit->attackSpeed() < targetAS) {
					unit->setAttackSpeed(targetAS);
				}
			}

			// Cast Speed (reduce cast point)
			if (EnableCastSpeed) {
				for (int i = 0; i < unit->abilityCount(); i++) {
					Ability* ability = unit->abilityByIndex(i);
					if (ability) {
						// Reduce cast point to instant
						ability->setCastPoint(0.0f);
					}
				}
			}
		});

		GroupDestroy(myUnits);
	}

	//=============================================================================
	// UI CONFIGURATION
	//=============================================================================

	static CheckBox* CbMovement;
	static CheckBox* CbAttack;
	static CheckBox* CbCast;
	static Slider* SliderMovement;
	static Slider* SliderAttack;
	static Label* LbMovement;
	static Label* LbAttack;

	void CreateMenuContent() {
		UISimpleFrame* Panel = DefaultOptionMenuGet()->category("Speed Hack", NULL);

		// Movement Speed
		CbMovement = new CheckBox(Panel);
		CbMovement->bindProfile("SpeedHack", "MovementSpeed", false);
		CbMovement->bindVariable(&EnableMovementSpeed);
		CbMovement->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.05f);

		Label* lbMoveTitle = new Label(Panel, "Movement Speed Boost", 0.013f);
		lbMoveTitle->setRelativePosition(POS_L, CbMovement, POS_R, 0.01f, 0);

		LbMovement = new Label(Panel, "Multiplier: 2.0x (Max: 522)", 0.012f);
		LbMovement->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.08f);

		SliderMovement = new Slider(Panel);
		SliderMovement->setRange(1.0f, 5.0f);
		SliderMovement->setValue(MovementSpeedMultiplier);
		SliderMovement->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.11f);
		SliderMovement->setSize(0.15f, 0.02f);

		SliderMovement->setOnValueChanged([](float value) {
			MovementSpeedMultiplier = value;
			ProfileStoreFloat("SpeedHack", "MovementMultiplier", value);

			char text[64];
			sprintf(text, "Multiplier: %.1fx (Max: 522)", value);
			LbMovement->setText(text);
		});

		// Attack Speed
		CbAttack = new CheckBox(Panel);
		CbAttack->bindProfile("SpeedHack", "AttackSpeed", false);
		CbAttack->bindVariable(&EnableAttackSpeed);
		CbAttack->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.15f);

		Label* lbAttackTitle = new Label(Panel, "Attack Speed Boost", 0.013f);
		lbAttackTitle->setRelativePosition(POS_L, CbAttack, POS_R, 0.01f, 0);

		LbAttack = new Label(Panel, "Multiplier: 2.0x", 0.012f);
		LbAttack->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.18f);

		SliderAttack = new Slider(Panel);
		SliderAttack->setRange(1.0f, 5.0f);
		SliderAttack->setValue(AttackSpeedMultiplier);
		SliderAttack->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.21f);
		SliderAttack->setSize(0.15f, 0.02f);

		SliderAttack->setOnValueChanged([](float value) {
			AttackSpeedMultiplier = value;
			ProfileStoreFloat("SpeedHack", "AttackMultiplier", value);

			char text[64];
			sprintf(text, "Multiplier: %.1fx", value);
			LbAttack->setText(text);
		});

		// Cast Speed
		CbCast = new CheckBox(Panel);
		CbCast->bindProfile("SpeedHack", "CastSpeed", false);
		CbCast->bindVariable(&EnableCastSpeed);
		CbCast->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.25f);

		Label* lbCast = new Label(Panel, "Instant Cast (No Cast Point)", 0.013f);
		lbCast->setRelativePosition(POS_L, CbCast, POS_R, 0.01f, 0);

		// Info
		Label* lbInfo = new Label(
			Panel,
			"|cff00ff00Movement speed capped at 522|r\n"
			"|cffff8800High values may look unnatural|r",
			0.011f
		);
		lbInfo->setRelativePosition(POS_UL, Panel, POS_UL, 0.03f, -0.29f);
	}

	//=============================================================================
	// INITIALIZATION
	//=============================================================================

	void Init() {
		// Load settings
		EnableMovementSpeed = ProfileFetchInt("SpeedHack", "MovementSpeed", 0) != 0;
		EnableAttackSpeed = ProfileFetchInt("SpeedHack", "AttackSpeed", 0) != 0;
		EnableCastSpeed = ProfileFetchInt("SpeedHack", "CastSpeed", 0) != 0;
		MovementSpeedMultiplier = ProfileFetchFloat("SpeedHack", "MovementMultiplier", 2.0f);
		AttackSpeedMultiplier = ProfileFetchFloat("SpeedHack", "AttackMultiplier", 2.0f);

		// Create UI
		CreateMenuContent();

		// Event listener
		MainDispatcher()->listen(EVENT_GAME_LOOP, onGameLoop);
	}

	void Cleanup() {
		// Nothing to cleanup
	}
}
