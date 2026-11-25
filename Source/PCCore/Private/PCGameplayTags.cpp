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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaRegenPaused, "State.StaminaRegenPaused", "기력을 쓴 직후라 회복이 멈춰 있음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Swapping, "State.Swapping", "무기를 바꾸는 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming, "State.Aiming", "원거리 무기를 겨누는 중");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "공격 능력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Guard, "Ability.Guard", "방어 능력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "회피 능력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_SwapWeapon, "Ability.SwapWeapon", "무기 교체");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input, "Input", "입력 슬롯");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Primary, "Input.Primary", "주 공격 슬롯");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Secondary, "Input.Secondary", "보조 슬롯");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Special, "Input.Special", "특수 슬롯");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon, "Weapon", "무기 분류 (자식: 무기 이름)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Light, "Threat.Light", "일반 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Heavy, "Threat.Heavy", "강한 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Threat_Unblockable, "Threat.Unblockable", "방패로 막을 수 없는 공격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monster_Rank, "Monster.Rank", "몬스터 등급");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monster_Rank_Normal, "Monster.Rank.Normal", "일반");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monster_Rank_Elite, "Monster.Rank.Elite", "엘리트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monster_Rank_Boss, "Monster.Rank.Boss", "보스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monster_Habitat, "Monster.Habitat", "몬스터 서식지 (자식: 지역 이름)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller: 체력 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Stamina, "Data.Stamina", "SetByCaller: 기력 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "SetByCaller: 지속 시간(초)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_MoveSpeed, "Data.MoveSpeed", "SetByCaller: 이동 속도 배율");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Magnitude, "Data.Magnitude", "SetByCaller: 타격 반응으로 거는 효과의 세기");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Windup, "GameplayCue.Combat.Windup", "예비 동작 시작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Hit, "GameplayCue.Combat.Hit", "피격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_HitConfirm, "GameplayCue.Combat.HitConfirm", "내 공격이 닿음 (때린 쪽에 전달)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Block, "GameplayCue.Combat.Block", "방패로 막음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Interrupt, "GameplayCue.Combat.Interrupt", "경직됨");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_Swap, "GameplayCue.Combat.Swap", "무기 교체 시작");
}
