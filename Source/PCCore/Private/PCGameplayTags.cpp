#include "PCGameplayTags.h"

namespace PCTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action, "State.Action", "행동 진행 중 (자식: Windup, Active, Recovery)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Windup, "State.Action.Windup", "예비 동작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Active, "State.Action.Active", "판정 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Recovery, "State.Action.Recovery", "후딜");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Guarding, "State.Guarding", "방패를 올리고 있음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Staggered, "State.Staggered", "경직");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "공격 능력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Guard, "Ability.Guard", "방어 능력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "회피 능력");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Light, "Threat.Light", "일반 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Heavy, "Threat.Heavy", "강한 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Unblockable, "Threat.Unblockable", "방패로 막을 수 없는 공격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller: 체력 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Stamina, "Data.Stamina", "SetByCaller: 기력 변화량");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Windup, "GameplayCue.Combat.Windup", "예비 동작 시작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Hit, "GameplayCue.Combat.Hit", "피격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Block, "GameplayCue.Combat.Block", "방패로 막음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Interrupt, "GameplayCue.Combat.Interrupt", "경직됨");
}
