#include "stdafx.h"
#include "DreamDotaMain.h"

static bool GameDebugPaused;
#ifndef _VMP
void FastPauseGame (const Event *evt) {
	KeyboardEventData *data = evt->data<KeyboardEventData>();
	if (data->code == ProfileFetchInt("Debug", "PauseGameHotkey", KEYCODE::KEY_SPACE)
		&& !data->alt
		&& !data->ctrl
		&& !data->shift	)
	{
		data->discard();	 DiscardCurrentEvent();
		GameDebugPaused = !GameDebugPaused;
		Jass::PauseGame(GameDebugPaused);
	}
}
#endif

void DreamDota_Init() {
#ifndef _VMP
	if (ProfileFetchInt("Debug", "PauseGameOnHotkey", 0)>=1){
		MainDispatcher()->listen(EVENT_KEY_DOWN, FastPauseGame);//debugĿ�ģ�������ͣ��Ϸ
	}
#endif

#ifdef _DREAMDOTA
	//����
	CustomCamera::Init();
	MapHack::Init();
	LastHit::Init();
	PerfectLastHit::Init();
	ProAutoAttack::Init();
	MinimapPingEnemyHero::Init();


	//�����е㿨
	RuneNotify::Init();
	DamageDisplay::Init();
	DirectionMove::Init();


	SmartDeny::Init();

	// Pro Features
	AlwaysCrit::Init();
	RealAlwaysCrit::Init();
	SuperPower::Init();
	SpeedHack::Init();
	VisionHack::Init();

	//�ܿ�
	ShowCooldown::Init();

	//δ���Կ�����
	CommandThrough::Init();
	Invoker::Init();
	InvisibleDisplay::Init();
#endif

#ifdef _DREAMWAR3
	CustomCamera::Init();
#endif

#ifdef _FREEPLUGIN
	CustomCamera::Init();
	ShowCooldown::Init();
#endif

}

void DreamDota_Cleanup() {

#ifdef _DREAMDOTA
	InvisibleDisplay::Cleanup();
	Invoker::Cleanup();
	CommandThrough::Cleanup();
	ShowCooldown::Cleanup();
	CustomCamera::Cleanup();
	SmartDeny::Cleanup();
	DirectionMove::Cleanup();
	DamageDisplay::Cleanup();
	RuneNotify::Cleanup();
	MinimapPingEnemyHero::Cleanup();
	LastHit::Cleanup();
	MapHack::Cleanup();
#endif

#ifdef _DREAMWAR3
	CustomCamera::Cleanup();
#endif

#ifdef _FREEPLUGIN
	ShowCooldown::Cleanup();
	CustomCamera::Cleanup();
#endif

}