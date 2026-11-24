# 전투 프로토타입 1단계 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 검과 방패를 든 플레이어가 GAS 기반 기술 4개(베기, 방패 올리기, 방패 밀기, 회피)와 기력으로, 제자리에서 할퀴기를 반복하는 늑대인간과 싸우며 "확정타 없음" 판정 규칙을 시험할 수 있게 한다.

**Architecture:** 새 모듈 `PCCore`(태그, 로그)와 `PCCombat`(능력, 속성, 판정, 캐릭터)을 추가한다. 기술은 `UGameplayAbility` 자식 3종이고 수치는 `UPCAbilityData` 데이터 에셋에서 읽는다. 판정은 `FPCHitResolver` 한 곳에서 하고, 겉모습은 GameplayCue를 듣는 컴포넌트가 임시 도형으로 그린다.

**Tech Stack:** UE 5.8.4 C++, Gameplay Ability System, Gameplay Tags, Enhanced Input, UE Automation Test, 에디터 Python 스크립트.

**설계 문서:** `docs/superpowers/specs/2026-11-24-combat-prototype-design.md`

---

## 공통 사항

모든 명령은 PowerShell에서 `C:\UnrealEngine\ProjectC`를 작업 폴더로 실행한다. **빌드와 명령줄 테스트는 언리얼 에디터가 꺼진 상태에서 실행한다.**

**빌드 명령** (이하 "빌드"):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ProjectCEditor Win64 Development -Project="C:\UnrealEngine\ProjectC\ProjectC.uproject" -WaitMutex
```

기대 결과: 마지막에 `Result: Succeeded`.

**자동화 테스트 명령** (이하 "테스트"):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\UnrealEngine\ProjectC\ProjectC.uproject" -ExecCmds="Automation RunTests PC.Combat; Quit" -unattended -nullrhi -nosplash -nosound -stdout -log
```

기대 결과: 로그에 각 테스트의 `Result={Success}`가 있고 `Result={Fail}`이 없다.

**코드 규칙**

- 모듈은 `Public/`(헤더)과 `Private/`(구현) 폴더를 쓴다. 포함 경로는 `Public/` 기준이다. 예: `#include "Hit/PCHitResolver.h"`.
- 두 모듈 모두 `ProjectC`(템플릿) 헤더를 포함하지 않는다.
- 코드가 참조하는 태그는 `PCGameplayTags`에 네이티브로 선언한다. 콘텐츠 전용 태그(개별 기술 이름)는 `Config/DefaultGameplayTags.ini`에 둔다.
- 엔진 API가 계획의 코드와 다르면 `C:\Program Files\Epic Games\UE_5.8\Engine` 소스를 확인해 고치고, 고친 내용을 커밋 메시지에 적는다.

## 파일 구성

```
ProjectC.uproject                               수정: 모듈 2개, GameplayAbilities 플러그인
Source/ProjectC.Target.cs                       수정: ExtraModuleNames
Source/ProjectCEditor.Target.cs                 수정: ExtraModuleNames
Config/DefaultGameplayTags.ini                  생성: 콘텐츠 전용 태그
Tools/pc_create_combat_assets.py                생성: 에셋 생성 스크립트
Tools/Data/DT_PC_Combatants.csv                 생성: 캐릭터 기본 수치

Source/PCCore/
  PCCore.Build.cs
  Public/PCLog.h                                로그 카테고리
  Public/PCGameplayTags.h                       네이티브 태그 선언
  Private/PCCoreModule.cpp, PCLog.cpp, PCGameplayTags.cpp

Source/PCCombat/
  PCCombat.Build.cs
  Private/PCCombatModule.cpp
  Public/Types/PCCombatTypes.h                  enum, struct
  Public/Hit/PCHitResolver.h                    판정 규칙
  Public/Hit/PCHittable.h                       맞을 수 있는 대상 인터페이스
  Public/Data/PCAbilityData.h                   기술 데이터 에셋
  Public/Data/PCCombatantRow.h                  데이터테이블 행
  Public/AbilitySystem/PCAbilitySystemComponent.h
  Public/AbilitySystem/Attributes/PCVitalAttributeSet.h
  Public/AbilitySystem/Effects/PCEffect_Damage.h, PCEffect_Stamina.h
  Public/AbilitySystem/Abilities/PCGameplayAbility.h, PCAbility_MeleeAttack.h, PCAbility_Guard.h, PCAbility_Dodge.h
  Public/Characters/PCCharacterBase.h, PCPlayerCharacter.h, PCMonsterBase.h
  Public/Presentation/PCCombatPresenterComponent.h
  Public/UI/PCCombatHUD.h
  Public/Game/PCCombatGameMode.h
  Private/...                                   위 헤더와 같은 경로의 .cpp
  Private/Tests/PCHitResolverTest.cpp           자동화 테스트
```

---

### Task 1: 모듈 뼈대와 GAS 플러그인

**Files:**
- Modify: `ProjectC.uproject`
- Modify: `Source/ProjectC.Target.cs:13`, `Source/ProjectCEditor.Target.cs:13`
- Create: `Source/PCCore/PCCore.Build.cs`, `Source/PCCore/Private/PCCoreModule.cpp`, `Source/PCCore/Public/PCLog.h`, `Source/PCCore/Private/PCLog.cpp`
- Create: `Source/PCCombat/PCCombat.Build.cs`, `Source/PCCombat/Private/PCCombatModule.cpp`

- [ ] **Step 1: `ProjectC.uproject`를 아래 내용으로 교체**

```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.8",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "ProjectC",
			"Type": "Runtime",
			"LoadingPhase": "Default",
			"AdditionalDependencies": [
				"Engine",
				"AIModule",
				"UMG"
			]
		},
		{
			"Name": "PCCore",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		},
		{
			"Name": "PCCombat",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "ModelingToolsEditorMode",
			"Enabled": true,
			"TargetAllowList": [
				"Editor"
			]
		},
		{
			"Name": "StateTree",
			"Enabled": true
		},
		{
			"Name": "GameplayStateTree",
			"Enabled": true
		},
		{
			"Name": "GameplayAbilities",
			"Enabled": true
		}
	]
}
```

- [ ] **Step 2: Target 파일 두 개 수정**

`Source/ProjectC.Target.cs`와 `Source/ProjectCEditor.Target.cs`에서 각각

```csharp
		ExtraModuleNames.Add("ProjectC");
```

를 아래로 바꾼다.

```csharp
		ExtraModuleNames.AddRange(new string[] { "ProjectC", "PCCore", "PCCombat" });
```

- [ ] **Step 3: PCCore 모듈 파일 생성**

`Source/PCCore/PCCore.Build.cs`

```csharp
using UnrealBuildTool;

public class PCCore : ModuleRules
{
	public PCCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags"
		});
	}
}
```

`Source/PCCore/Private/PCCoreModule.cpp`

```cpp
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, PCCore);
```

`Source/PCCore/Public/PCLog.h`

```cpp
#pragma once

#include "Logging/LogMacros.h"

/** ProjectC 실험 코드 공용 로그 카테고리 */
PCCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPC, Log, All);
```

`Source/PCCore/Private/PCLog.cpp`

```cpp
#include "PCLog.h"

DEFINE_LOG_CATEGORY(LogPC);
```

- [ ] **Step 4: PCCombat 모듈 파일 생성**

`Source/PCCombat/PCCombat.Build.cs`

```csharp
using UnrealBuildTool;

public class PCCombat : ModuleRules
{
	public PCCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"PCCore"
		});
	}
}
```

`Source/PCCombat/Private/PCCombatModule.cpp`

```cpp
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, PCCombat);
```

- [ ] **Step 5: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 6: 커밋**

```powershell
git add ProjectC.uproject Source/ProjectC.Target.cs Source/ProjectCEditor.Target.cs Source/PCCore Source/PCCombat
git commit -m "Add PCCore and PCCombat modules, enable GameplayAbilities"
```

---

### Task 2: 네이티브 게임플레이 태그

**Files:**
- Create: `Source/PCCore/Public/PCGameplayTags.h`, `Source/PCCore/Private/PCGameplayTags.cpp`
- Create: `Config/DefaultGameplayTags.ini`

- [ ] **Step 1: `Source/PCCore/Public/PCGameplayTags.h`**

```cpp
#pragma once

#include "NativeGameplayTags.h"

/** 코드가 참조하는 태그. 개별 기술 이름 같은 콘텐츠 전용 태그는 Config/DefaultGameplayTags.ini에 둔다. */
namespace PCTags
{
	// 행동 단계
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Windup);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Active);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Recovery);

	// 캐릭터 상태
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guarding);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Staggered);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	// 능력 분류
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);

	// 위험 등급
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Threat_Light);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Threat_Heavy);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Threat_Unblockable);

	// SetByCaller 수치 전달
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Stamina);

	// 겉모습 연결
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Windup);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Hit);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Block);
	PCCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Interrupt);
}
```

- [ ] **Step 2: `Source/PCCore/Private/PCGameplayTags.cpp`**

```cpp
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
```

- [ ] **Step 3: `Config/DefaultGameplayTags.ini`**

```ini
[/Script/GameplayTags.GameplayTagsSettings]
+GameplayTagList=(Tag="Ability.Attack.Slash",DevComment="검 베기")
+GameplayTagList=(Tag="Ability.Attack.ShieldBash",DevComment="방패 밀기")
+GameplayTagList=(Tag="Ability.Attack.Claw",DevComment="늑대인간 할퀴기")
```

- [ ] **Step 4: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 5: 커밋**

```powershell
git add Source/PCCore Config/DefaultGameplayTags.ini
git commit -m "Add native gameplay tags for combat"
```

---

### Task 3: 전투 타입과 판정 계산 (TDD)

**Files:**
- Create: `Source/PCCombat/Public/Types/PCCombatTypes.h`
- Create: `Source/PCCombat/Public/Hit/PCHitResolver.h`, `Source/PCCombat/Private/Hit/PCHitResolver.cpp`
- Test: `Source/PCCombat/Private/Tests/PCHitResolverTest.cpp`

- [ ] **Step 1: 실패하는 테스트 작성**

`Source/PCCombat/Private/Tests/PCHitResolverTest.cpp`

```cpp
#include "Misc/AutomationTest.h"
#include "Hit/PCHitResolver.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCHitResolverSectorTest, "PC.Combat.HitResolver.Sector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCHitResolverSectorTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward = FVector::ForwardVector;

	TestTrue(TEXT("정면, 범위 안이면 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 0, 0), 150.f, 45.f));
	TestFalse(TEXT("정면이어도 범위 밖이면 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(200, 0, 0), 150.f, 45.f));
	TestFalse(TEXT("옆은 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(0, 100, 0), 150.f, 45.f));
	TestFalse(TEXT("뒤는 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(-100, 0, 0), 150.f, 45.f));
	TestTrue(TEXT("각도 경계 안쪽(약 42도)은 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 90, 0), 150.f, 45.f));
	TestFalse(TEXT("각도 경계 바깥(약 48도)은 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 110, 0), 150.f, 45.f));
	TestTrue(TEXT("높이 차이는 무시한다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 0, 500), 150.f, 45.f));
	TestTrue(TEXT("같은 위치는 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, Origin, 150.f, 45.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCHitResolverGuardTest, "PC.Combat.HitResolver.Guard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCHitResolverGuardTest::RunTest(const FString& Parameters)
{
	const FVector Defender = FVector::ZeroVector;
	const FVector Facing = FVector::ForwardVector;

	TestTrue(TEXT("정면에서 온 공격은 막을 수 있다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(200, 0, 0), 60.f));
	TestFalse(TEXT("뒤에서 온 공격은 막을 수 없다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(-200, 0, 0), 60.f));
	TestFalse(TEXT("옆에서 온 공격은 방어 각도 60도로 막을 수 없다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(0, 200, 0), 60.f));
	TestTrue(TEXT("방어 각도가 100도면 옆도 막는다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(0, 200, 0), 100.f));
	TestTrue(TEXT("멀리서 온 공격도 방향만 맞으면 막는다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(5000, 0, 0), 60.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: 빌드해서 실패 확인**

빌드 명령 실행. 기대 결과: `Hit/PCHitResolver.h` 파일을 열 수 없다는 컴파일 오류.

- [ ] **Step 3: `Source/PCCombat/Public/Types/PCCombatTypes.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "PCCombatTypes.generated.h"

class AActor;
class UPCAbilityData;

/** 공격 판정 결과 */
UENUM(BlueprintType)
enum class EPCHitOutcome : uint8
{
	Miss,
	Hit,
	Blocked
};

/** 행동의 진행 단계 */
UENUM(BlueprintType)
enum class EPCActionPhase : uint8
{
	None,
	Windup,
	Active,
	Recovery
};

/** 행동 단계별 시간 (초) */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCActionTiming
{
	GENERATED_BODY()

	/** 예비 동작: 상대가 읽고 반응할 수 있는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Windup = 0.3f;

	/** 판정 구간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Active = 0.1f;

	/** 후딜 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Recovery = 0.4f;
};

/** 공격 범위: 공격자 정면의 부채꼴 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCHitShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "cm"))
	float Range = 180.f;

	/** 정면 기준 좌우 반각 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float HalfAngleDeg = 50.f;
};

/** 판정 한 번에 필요한 정보. 판정하는 동안에만 쓰고 보관하지 않는다. */
struct FPCHitInfo
{
	AActor* Attacker = nullptr;
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	const UPCAbilityData* Data = nullptr;
};
```

- [ ] **Step 4: `Source/PCCombat/Public/Hit/PCHitResolver.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Types/PCCombatTypes.h"

/**
 * 판정 규칙이 있는 유일한 곳.
 * "확정타 없음": 공격은 범위 안에서 처음 닿은 대상 하나에만 적용된다.
 */
struct PCCOMBAT_API FPCHitResolver
{
	/** Target이 Origin에서 Forward 방향 부채꼴(Range, HalfAngleDeg) 안에 있는가. 높이는 무시한다. */
	static bool IsWithinSector(const FVector& Origin, const FVector& Forward, const FVector& Target, float Range, float HalfAngleDeg);

	/** 공격이 방어자의 정면 방어 각도 안에서 왔는가 */
	static bool IsAttackFromFront(const FVector& DefenderLocation, const FVector& DefenderForward, const FVector& AttackOrigin, float GuardHalfAngleDeg);
};
```

- [ ] **Step 5: `Source/PCCombat/Private/Hit/PCHitResolver.cpp`**

```cpp
#include "Hit/PCHitResolver.h"

bool FPCHitResolver::IsWithinSector(const FVector& Origin, const FVector& Forward, const FVector& Target, float Range, float HalfAngleDeg)
{
	FVector ToTarget = Target - Origin;
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	if (Distance > Range)
	{
		return false;
	}
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return true;
	}

	FVector FlatForward = Forward;
	FlatForward.Z = 0.f;
	if (!FlatForward.Normalize())
	{
		return false;
	}

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));
	return FVector::DotProduct(FlatForward, ToTarget / Distance) >= CosHalfAngle;
}

bool FPCHitResolver::IsAttackFromFront(const FVector& DefenderLocation, const FVector& DefenderForward, const FVector& AttackOrigin, float GuardHalfAngleDeg)
{
	return IsWithinSector(DefenderLocation, DefenderForward, AttackOrigin, UE_BIG_NUMBER, GuardHalfAngleDeg);
}
```

- [ ] **Step 6: 빌드 후 테스트 통과 확인**

빌드 명령, 이어서 테스트 명령 실행. 기대 결과: `PC.Combat.HitResolver.Sector`와 `PC.Combat.HitResolver.Guard`가 모두 `Result={Success}`.

- [ ] **Step 7: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add combat types and hit sector math with tests"
```

---

### Task 4: 데이터 정의

**Files:**
- Create: `Source/PCCombat/Public/Data/PCAbilityData.h`, `Source/PCCombat/Private/Data/PCAbilityData.cpp`
- Create: `Source/PCCombat/Public/Data/PCCombatantRow.h`
- Create: `Tools/Data/DT_PC_Combatants.csv`

- [ ] **Step 1: `Source/PCCombat/Public/Data/PCAbilityData.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Types/PCCombatTypes.h"
#include "PCAbilityData.generated.h"

class UAnimMontage;
class UGameplayAbility;

/**
 * 기술 하나의 정의. 어떤 능력 클래스로 동작하는지와 모든 수치를 담는다.
 * 동작이 같고 수치만 다른 기술은 이 에셋만 새로 만든다.
 */
UCLASS(BlueprintType)
class PCCOMBAT_API UPCAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 이 기술을 실행하는 능력 클래스. UPCGameplayAbility의 자식을 지정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 이 기술을 가리키는 태그. 입력과 AI가 이 태그로 발동한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FPCActionTiming Timing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = 0))
	float StaminaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float Damage = 0.f;

	/** 막혔을 때 상대 기력을 깎는 양 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float GuardStaminaDamage = 0.f;

	/** 맞은 대상의 경직 시간. 0이면 경직 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "s"))
	float StaggerTime = 0.f;

	/** true면 대상이 예비 동작 중일 때만 경직시킨다 (방패 밀기: 타이밍을 맞춰야 끊는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bStaggerOnlyOnWindup = false;

	/** 위험 등급. 예고 강도와 방어 가능 여부를 정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (Categories = "Threat"))
	FGameplayTag ThreatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FPCHitShape HitShape;

	/** 방어 전용: 정면 기준 막을 수 있는 반각 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float GuardHalfAngleDeg = 70.f;

	/** 회피 전용: 이동 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = 0, Units = "cm"))
	float DodgeDistance = 350.f;

	/** 비어 있어도 동작한다. 애니메이션이 준비되면 채운다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UAnimMontage> Montage;
};
```

- [ ] **Step 2: `Source/PCCombat/Private/Data/PCAbilityData.cpp`**

```cpp
#include "Data/PCAbilityData.h"
```

- [ ] **Step 3: `Source/PCCombat/Public/Data/PCCombatantRow.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PCCombatantRow.generated.h"

/** 캐릭터와 몬스터의 기본 수치. 데이터테이블 한 행이 한 종류다. */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCCombatantRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 1))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0))
	float StaminaRegenPerSec = 25.f;

	/** 기력을 쓴 뒤 회복이 시작되기까지의 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0, Units = "s"))
	float StaminaRegenDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float MoveSpeed = 500.f;
};
```

- [ ] **Step 4: `Tools/Data/DT_PC_Combatants.csv`**

```csv
---,MaxHealth,MaxStamina,StaminaRegenPerSec,StaminaRegenDelay,MoveSpeed
Player,100,100,25,1.0,500
Werewolf,150,100,20,1.0,450
```

- [ ] **Step 5: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 6: 커밋**

```powershell
git add Source/PCCombat Tools
git commit -m "Add ability data asset and combatant row definitions"
```

---

### Task 5: 속성, 이펙트, 능력 시스템 컴포넌트

**Files:**
- Create: `Source/PCCombat/Public/AbilitySystem/Attributes/PCVitalAttributeSet.h`, `Private/AbilitySystem/Attributes/PCVitalAttributeSet.cpp`
- Create: `Source/PCCombat/Public/AbilitySystem/Effects/PCEffect_Damage.h`, `Private/AbilitySystem/Effects/PCEffect_Damage.cpp`
- Create: `Source/PCCombat/Public/AbilitySystem/Effects/PCEffect_Stamina.h`, `Private/AbilitySystem/Effects/PCEffect_Stamina.cpp`
- Create: `Source/PCCombat/Public/AbilitySystem/PCAbilitySystemComponent.h`, `Private/AbilitySystem/PCAbilitySystemComponent.cpp`

- [ ] **Step 1: `Public/AbilitySystem/Attributes/PCVitalAttributeSet.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PCVitalAttributeSet.generated.h"

/** 체력과 기력 */
UCLASS()
class PCCOMBAT_API UPCVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, MaxStamina)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
	/** 체력과 기력을 0과 최대값 사이로 제한한다 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
```

- [ ] **Step 2: `Private/AbilitySystem/Attributes/PCVitalAttributeSet.cpp`**

```cpp
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"

void UPCVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPCVitalAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPCVitalAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
}
```

- [ ] **Step 3: 이펙트 두 개**

`Public/AbilitySystem/Effects/PCEffect_Damage.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PCEffect_Damage.generated.h"

/** 즉시 체력을 바꾼다. 변화량은 SetByCaller(Data.Damage)로 받는다. 피해는 음수로 넣는다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Damage();
};
```

`Private/AbilitySystem/Effects/PCEffect_Damage.cpp`

```cpp
#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Damage::UPCEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = PCTags::Data_Damage;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UPCVitalAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
```

`Public/AbilitySystem/Effects/PCEffect_Stamina.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PCEffect_Stamina.generated.h"

/** 즉시 기력을 바꾼다. 변화량은 SetByCaller(Data.Stamina)로 받는다. 소모는 음수로 넣는다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Stamina : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Stamina();
};
```

`Private/AbilitySystem/Effects/PCEffect_Stamina.cpp`

```cpp
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Stamina::UPCEffect_Stamina()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = PCTags::Data_Stamina;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UPCVitalAttributeSet::GetStaminaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
```

- [ ] **Step 4: `Public/AbilitySystem/PCAbilitySystemComponent.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PCAbilitySystemComponent.generated.h"

class UPCAbilityData;

/** 데이터 에셋으로 기술을 부여하고, 기술 태그로 발동/취소한다 */
UCLASS(ClassGroup = "PC", meta = (BlueprintSpawnableComponent))
class PCCOMBAT_API UPCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 데이터 에셋이 지정한 능력 클래스를 부여한다. 데이터는 능력의 SourceObject로 전달된다. */
	FGameplayAbilitySpecHandle GrantAbilityFromData(const UPCAbilityData* Data);

	/** 기술 태그로 발동을 시도한다. 발동했으면 true. */
	bool TryActivateByAbilityTag(FGameplayTag AbilityTag);

	/** 기술 태그에 해당하는 능력이 실행 중이면 취소한다 */
	void CancelByAbilityTag(FGameplayTag AbilityTag);

	/** SetByCaller 수치 하나를 받는 즉시 이펙트를 자신에게 적용한다 */
	void ApplySetByCallerEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag DataTag, float Magnitude, AActor* Instigator);

private:
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> HandlesByAbilityTag;
};
```

- [ ] **Step 5: `Private/AbilitySystem/PCAbilitySystemComponent.cpp`**

```cpp
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Data/PCAbilityData.h"
#include "PCLog.h"

FGameplayAbilitySpecHandle UPCAbilitySystemComponent::GrantAbilityFromData(const UPCAbilityData* Data)
{
	if (!Data || !Data->AbilityClass || !Data->AbilityTag.IsValid())
	{
		UE_LOG(LogPC, Warning, TEXT("GrantAbilityFromData: 능력 클래스나 태그가 비어 있는 데이터 '%s'"), *GetNameSafe(Data));
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec Spec(Data->AbilityClass, 1, INDEX_NONE, const_cast<UPCAbilityData*>(Data));
	const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);
	HandlesByAbilityTag.Add(Data->AbilityTag, Handle);
	return Handle;
}

bool UPCAbilitySystemComponent::TryActivateByAbilityTag(FGameplayTag AbilityTag)
{
	const FGameplayAbilitySpecHandle* Handle = HandlesByAbilityTag.Find(AbilityTag);
	return Handle ? TryActivateAbility(*Handle) : false;
}

void UPCAbilitySystemComponent::CancelByAbilityTag(FGameplayTag AbilityTag)
{
	if (const FGameplayAbilitySpecHandle* Handle = HandlesByAbilityTag.Find(AbilityTag))
	{
		CancelAbilityHandle(*Handle);
	}
}

void UPCAbilitySystemComponent::ApplySetByCallerEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag DataTag, float Magnitude, AActor* Instigator)
{
	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddInstigator(Instigator, Instigator);

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
```

- [ ] **Step 6: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 7: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add vital attributes, damage/stamina effects, ability system component"
```

---

### Task 6: 맞을 수 있는 대상과 월드 판정

**Files:**
- Create: `Source/PCCombat/Public/Hit/PCHittable.h`, `Source/PCCombat/Private/Hit/PCHittable.cpp`
- Modify: `Source/PCCombat/Public/Hit/PCHitResolver.h`, `Source/PCCombat/Private/Hit/PCHitResolver.cpp`

- [ ] **Step 1: `Public/Hit/PCHittable.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/PCCombatTypes.h"
#include "PCHittable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPCHittable : public UInterface
{
	GENERATED_BODY()
};

/** 공격 경로에 놓일 수 있는 대상. 캐릭터가 아니어도 구현할 수 있다. */
class PCCOMBAT_API IPCHittable
{
	GENERATED_BODY()

public:
	/** 공격을 받는다. 막았는지 맞았는지는 받는 쪽이 판단한다. */
	virtual EPCHitOutcome ReceiveHit(const FPCHitInfo& Hit) = 0;
};
```

`Private/Hit/PCHittable.cpp`

```cpp
#include "Hit/PCHittable.h"
```

- [ ] **Step 2: `PCHitResolver.h`의 struct 안, `IsAttackFromFront` 선언 아래에 추가**

```cpp
	/** 공격 범위 안에서 공격자에게 가장 가까운 IPCHittable 액터를 찾는다. 없으면 nullptr. */
	static AActor* FindFirstHittable(const FPCHitInfo& Hit);

	/** 처음 닿은 대상 하나에 공격을 적용하고 결과를 돌려준다. 닿은 대상이 없으면 Miss. */
	static EPCHitOutcome ResolveAttack(const FPCHitInfo& Hit, AActor*& OutVictim);
```

- [ ] **Step 3: `PCHitResolver.cpp` 수정**

파일 맨 위 include를 아래로 바꾼다.

```cpp
#include "Hit/PCHitResolver.h"
#include "Hit/PCHittable.h"
#include "Data/PCAbilityData.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** 범위 검색에 더하는 여유. 대상의 몸 반지름만큼 멀리 있어도 후보에 들어오게 한다. */
	constexpr float MaxTargetRadius = 150.f;
}
```

파일 끝에 추가한다.

```cpp
AActor* FPCHitResolver::FindFirstHittable(const FPCHitInfo& Hit)
{
	if (!Hit.Attacker || !Hit.Data)
	{
		return nullptr;
	}

	UWorld* World = Hit.Attacker->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FPCHitShape& Shape = Hit.Data->HitShape;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCHitResolver), false, Hit.Attacker);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Hit.Origin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Shape.Range + MaxTargetRadius), QueryParams);

	AActor* Nearest = nullptr;
	float NearestDistance = UE_BIG_NUMBER;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Nearest || !Candidate->Implements<UPCHittable>())
		{
			continue;
		}

		const float TargetRadius = Candidate->GetSimpleCollisionRadius();
		const FVector TargetLocation = Candidate->GetActorLocation();
		if (!IsWithinSector(Hit.Origin, Hit.Direction, TargetLocation, Shape.Range + TargetRadius, Shape.HalfAngleDeg))
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Hit.Origin, TargetLocation) - TargetRadius;
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Candidate;
		}
	}

	return Nearest;
}

EPCHitOutcome FPCHitResolver::ResolveAttack(const FPCHitInfo& Hit, AActor*& OutVictim)
{
	OutVictim = FindFirstHittable(Hit);
	if (IPCHittable* Hittable = Cast<IPCHittable>(OutVictim))
	{
		return Hittable->ReceiveHit(Hit);
	}
	return EPCHitOutcome::Miss;
}
```

- [ ] **Step 4: 빌드 후 테스트**

빌드 명령, 이어서 테스트 명령 실행. 기대 결과: 빌드 성공, 기존 테스트 2개 `Result={Success}`.

- [ ] **Step 5: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add hittable interface and first-hit world query"
```

---

### Task 7: 캐릭터 베이스

**Files:**
- Create: `Source/PCCombat/Public/Characters/PCCharacterBase.h`, `Source/PCCombat/Private/Characters/PCCharacterBase.cpp`

- [ ] **Step 1: `Public/Characters/PCCharacterBase.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Hit/PCHittable.h"
#include "PCCharacterBase.generated.h"

class UPCAbilitySystemComponent;
class UPCVitalAttributeSet;
class UPCAbilityData;
struct FOnAttributeChangeData;

/** 판정 코드가 내보낸 GameplayCue를 겉모습 쪽에 전달한다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FPCOnCombatCue, FGameplayTag /*CueTag*/, const FGameplayCueParameters& /*Parameters*/);

/**
 * 전투에 참여하는 모든 캐릭터의 부모.
 * 능력 시스템, 체력/기력, 피격 처리, 경직, 사망과 부활을 가진다.
 */
UCLASS(Abstract)
class PCCOMBAT_API APCCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPCHittable, public IGameplayCueInterface
{
	GENERATED_BODY()

public:
	APCCharacterBase();

	// ~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ~IPCHittable
	virtual EPCHitOutcome ReceiveHit(const FPCHitInfo& Hit) override;

	// ~IGameplayCueInterface
	virtual void HandleGameplayCue(AActor* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

	UPCAbilitySystemComponent* GetPCAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** 방어 능력이 발동할 때 막을 수 있는 각도를 알려준다 */
	void SetGuardHalfAngle(float HalfAngleDeg) { GuardHalfAngleDeg = HalfAngleDeg; }

	/** 경직시킨다. 진행 중인 능력은 모두 취소된다. */
	void ApplyStagger(float Duration);

	bool IsDead() const;
	bool HasStateTag(FGameplayTag Tag) const;

	float GetHealth() const;
	float GetMaxHealth() const;
	float GetStamina() const;
	float GetMaxStamina() const;

	FPCOnCombatCue OnCombatCue;

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeath();
	virtual void Revive();

	/** 기본 수치를 읽어 올 데이터테이블 행. 비어 있으면 FPCCombatantRow의 기본값을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Data", meta = (RowType = "/Script/PCCombat.PCCombatantRow"))
	FDataTableRowHandle CombatantRow;

	/** 시작할 때 부여하는 기술 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Data")
	TArray<TObjectPtr<UPCAbilityData>> StartingAbilities;

	/** 방어 중 기력이 바닥났을 때의 경직 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Combat", meta = (ClampMin = 0, Units = "s"))
	float GuardBreakStaggerTime = 1.2f;

	/** 사망 후 부활까지의 시간. 실험용이라 죽어도 다시 일어난다. */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Combat", meta = (ClampMin = 0, Units = "s"))
	float ReviveDelay = 3.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPCVitalAttributeSet> VitalSet;

private:
	void InitializeFromRow();
	void GrantStartingAbilities();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void RegenStamina();
	void EndStagger();
	void ExecuteCue(FGameplayTag CueTag, const FPCHitInfo& Hit);

	float GuardHalfAngleDeg = 70.f;
	float StaminaRegenPerSec = 25.f;
	float StaminaRegenDelay = 1.f;
	double LastStaminaSpendTime = -1000.0;
	bool bStaggered = false;

	FTimerHandle StaminaRegenTimer;
	FTimerHandle StaggerTimer;
	FTimerHandle ReviveTimer;
};
```

- [ ] **Step 2: `Private/Characters/PCCharacterBase.cpp`**

```cpp
#include "Characters/PCCharacterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCCombatantRow.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "TimerManager.h"

namespace
{
	/** 기력 회복 타이머 간격 */
	constexpr float StaminaRegenInterval = 0.1f;
}

APCCharacterBase::APCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<UPCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	VitalSet = CreateDefaultSubobject<UPCVitalAttributeSet>(TEXT("VitalSet"));
}

UAbilitySystemComponent* APCCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APCCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitializeFromRow();
	GrantStartingAbilities();

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetHealthAttribute())
		.AddUObject(this, &APCCharacterBase::OnHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &APCCharacterBase::OnStaminaChanged);

	GetWorldTimerManager().SetTimer(StaminaRegenTimer, this, &APCCharacterBase::RegenStamina, StaminaRegenInterval, true);
}

void APCCharacterBase::InitializeFromRow()
{
	FPCCombatantRow Row;
	if (!CombatantRow.IsNull())
	{
		if (const FPCCombatantRow* Found = CombatantRow.GetRow<FPCCombatantRow>(TEXT("APCCharacterBase")))
		{
			Row = *Found;
		}
	}

	VitalSet->InitMaxHealth(Row.MaxHealth);
	VitalSet->InitHealth(Row.MaxHealth);
	VitalSet->InitMaxStamina(Row.MaxStamina);
	VitalSet->InitStamina(Row.MaxStamina);

	StaminaRegenPerSec = Row.StaminaRegenPerSec;
	StaminaRegenDelay = Row.StaminaRegenDelay;
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed;
}

void APCCharacterBase::GrantStartingAbilities()
{
	for (const UPCAbilityData* Data : StartingAbilities)
	{
		AbilitySystemComponent->GrantAbilityFromData(Data);
	}
}

bool APCCharacterBase::IsDead() const
{
	return HasStateTag(PCTags::State_Dead);
}

bool APCCharacterBase::HasStateTag(FGameplayTag Tag) const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(Tag);
}

float APCCharacterBase::GetHealth() const { return VitalSet->GetHealth(); }
float APCCharacterBase::GetMaxHealth() const { return VitalSet->GetMaxHealth(); }
float APCCharacterBase::GetStamina() const { return VitalSet->GetStamina(); }
float APCCharacterBase::GetMaxStamina() const { return VitalSet->GetMaxStamina(); }

EPCHitOutcome APCCharacterBase::ReceiveHit(const FPCHitInfo& Hit)
{
	if (!Hit.Data || IsDead())
	{
		return EPCHitOutcome::Miss;
	}

	const bool bGuarding = HasStateTag(PCTags::State_Guarding);
	const bool bBlockable = !Hit.Data->ThreatTag.MatchesTag(PCTags::Threat_Unblockable);
	const bool bFromFront = FPCHitResolver::IsAttackFromFront(GetActorLocation(), GetActorForwardVector(), Hit.Origin, GuardHalfAngleDeg);

	if (bGuarding && bBlockable && bFromFront)
	{
		if (Hit.Data->GuardStaminaDamage > 0.f)
		{
			AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Stamina::StaticClass(), PCTags::Data_Stamina, -Hit.Data->GuardStaminaDamage, Hit.Attacker);
		}
		ExecuteCue(PCTags::GameplayCue_Combat_Block, Hit);

		if (GetStamina() <= 0.f)
		{
			ApplyStagger(GuardBreakStaggerTime);
		}
		return EPCHitOutcome::Blocked;
	}

	// 피해로 죽거나 경직되면 상태가 바뀌므로, 예비 동작 중이었는지는 먼저 기록한다
	const bool bWasWindingUp = HasStateTag(PCTags::State_Action_Windup);

	if (Hit.Data->Damage > 0.f)
	{
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Damage::StaticClass(), PCTags::Data_Damage, -Hit.Data->Damage, Hit.Attacker);
	}
	ExecuteCue(PCTags::GameplayCue_Combat_Hit, Hit);

	if (!IsDead() && Hit.Data->StaggerTime > 0.f && (!Hit.Data->bStaggerOnlyOnWindup || bWasWindingUp))
	{
		ApplyStagger(Hit.Data->StaggerTime);
	}
	return EPCHitOutcome::Hit;
}

void APCCharacterBase::ExecuteCue(FGameplayTag CueTag, const FPCHitInfo& Hit)
{
	FGameplayCueParameters Parameters;
	Parameters.Instigator = Hit.Attacker;
	Parameters.SourceObject = Hit.Data;
	Parameters.Location = GetActorLocation();
	AbilitySystemComponent->ExecuteGameplayCue(CueTag, Parameters);
}

void APCCharacterBase::HandleGameplayCue(AActor* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	IGameplayCueInterface::HandleGameplayCue(Self, GameplayCueTag, EventType, Parameters);
	OnCombatCue.Broadcast(GameplayCueTag, Parameters);
}

void APCCharacterBase::ApplyStagger(float Duration)
{
	if (Duration <= 0.f || IsDead())
	{
		return;
	}

	if (!bStaggered)
	{
		bStaggered = true;
		AbilitySystemComponent->AddLooseGameplayTag(PCTags::State_Staggered);
	}

	AbilitySystemComponent->CancelAbilities();
	GetCharacterMovement()->StopMovementImmediately();

	FGameplayCueParameters Parameters;
	Parameters.Location = GetActorLocation();
	Parameters.RawMagnitude = Duration;
	AbilitySystemComponent->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Interrupt, Parameters);

	GetWorldTimerManager().SetTimer(StaggerTimer, this, &APCCharacterBase::EndStagger, Duration, false);
}

void APCCharacterBase::EndStagger()
{
	if (bStaggered)
	{
		bStaggered = false;
		AbilitySystemComponent->RemoveLooseGameplayTag(PCTags::State_Staggered);
	}
}

void APCCharacterBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && !IsDead())
	{
		HandleDeath();
	}
}

void APCCharacterBase::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		LastStaminaSpendTime = GetWorld()->GetTimeSeconds();
	}
}

void APCCharacterBase::RegenStamina()
{
	if (IsDead() || HasStateTag(PCTags::State_Guarding))
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - LastStaminaSpendTime < StaminaRegenDelay)
	{
		return;
	}
	if (GetStamina() < GetMaxStamina())
	{
		AbilitySystemComponent->ApplyModToAttribute(UPCVitalAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, StaminaRegenPerSec * StaminaRegenInterval);
	}
}

void APCCharacterBase::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(StaggerTimer);
	EndStagger();

	AbilitySystemComponent->AddLooseGameplayTag(PCTags::State_Dead);
	AbilitySystemComponent->CancelAbilities();
	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().SetTimer(ReviveTimer, this, &APCCharacterBase::Revive, ReviveDelay, false);
}

void APCCharacterBase::Revive()
{
	AbilitySystemComponent->RemoveLooseGameplayTag(PCTags::State_Dead);
	VitalSet->SetHealth(GetMaxHealth());
	VitalSet->SetStamina(GetMaxStamina());
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
```

- [ ] **Step 3: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 4: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add combat character base with hit handling, stagger, death"
```

---

### Task 8: 능력 (부모, 근접 공격, 방어, 회피)

**Files:**
- Create: `Public/AbilitySystem/Abilities/PCGameplayAbility.h`, `Private/AbilitySystem/Abilities/PCGameplayAbility.cpp`
- Create: `Public/AbilitySystem/Abilities/PCAbility_MeleeAttack.h`, `Private/.../PCAbility_MeleeAttack.cpp`
- Create: `Public/AbilitySystem/Abilities/PCAbility_Guard.h`, `Private/.../PCAbility_Guard.cpp`
- Create: `Public/AbilitySystem/Abilities/PCAbility_Dodge.h`, `Private/.../PCAbility_Dodge.cpp`

(경로는 모두 `Source/PCCombat/` 아래)

- [ ] **Step 1: `Public/AbilitySystem/Abilities/PCGameplayAbility.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Types/PCCombatTypes.h"
#include "PCGameplayAbility.generated.h"

class APCCharacterBase;
class UPCAbilityData;

/**
 * 모든 전투 능력의 부모.
 * 수치는 부여될 때 SourceObject로 받은 UPCAbilityData에서 읽는다.
 * 예비 동작 → 판정 → 후딜 단계를 데이터의 시간대로 진행하고, 단계마다 상태 태그를 붙인다.
 */
UCLASS(Abstract)
class PCCOMBAT_API UPCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPCGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 실행 중인 능력의 데이터 */
	const UPCAbilityData* GetAbilityData() const;

	APCCharacterBase* GetPCCharacter() const;

	EPCActionPhase GetCurrentPhase() const { return CurrentPhase; }

	/** 단계에 들어가고, Duration 뒤에 다음 단계로 넘어간다. Recovery가 끝나면 능력이 끝난다. */
	void StartPhase(EPCActionPhase Phase, float Duration);

	/** 단계가 바뀔 때 호출된다. 능력이 끝나면 NewPhase가 None이다. */
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) {}

	/** 능력을 취소로 끝낸다 */
	void EndAsCancelled();

private:
	UFUNCTION()
	void AdvancePhase();

	void SetPhase(EPCActionPhase NewPhase);

	static const UPCAbilityData* GetDataFromSpec(const UGameplayAbility& Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);
	static FGameplayTag PhaseToTag(EPCActionPhase Phase);

	EPCActionPhase CurrentPhase = EPCActionPhase::None;
};
```

- [ ] **Step 2: `Private/AbilitySystem/Abilities/PCGameplayAbility.cpp`**

```cpp
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

UPCGameplayAbility::UPCGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 한 번에 하나의 행동만 한다
	ActivationBlockedTags.AddTag(PCTags::State_Action);
	ActivationBlockedTags.AddTag(PCTags::State_Guarding);
	ActivationBlockedTags.AddTag(PCTags::State_Staggered);
	ActivationBlockedTags.AddTag(PCTags::State_Dead);
}

const UPCAbilityData* UPCGameplayAbility::GetDataFromSpec(const UGameplayAbility& Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	return Cast<UPCAbilityData>(Ability.GetSourceObject(Handle, ActorInfo));
}

const UPCAbilityData* UPCGameplayAbility::GetAbilityData() const
{
	return GetDataFromSpec(*this, CurrentSpecHandle, CurrentActorInfo);
}

APCCharacterBase* UPCGameplayAbility::GetPCCharacter() const
{
	return Cast<APCCharacterBase>(GetAvatarActorFromActorInfo());
}

bool UPCGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	const UPCAbilityData* Data = GetDataFromSpec(*this, Handle, ActorInfo);
	if (!Data || Data->StaminaCost <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return ASC && ASC->GetNumericAttribute(UPCVitalAttributeSet::GetStaminaAttribute()) >= Data->StaminaCost;
}

void UPCGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UPCAbilityData* Data = GetDataFromSpec(*this, Handle, ActorInfo);
	UPCAbilitySystemComponent* ASC = ActorInfo ? Cast<UPCAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (Data && ASC && Data->StaminaCost > 0.f)
	{
		ASC->ApplySetByCallerEffectToSelf(UPCEffect_Stamina::StaticClass(), PCTags::Data_Stamina, -Data->StaminaCost, ActorInfo->AvatarActor.Get());
	}
}

void UPCGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		SetPhase(EPCActionPhase::None);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPCGameplayAbility::EndAsCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPCGameplayAbility::StartPhase(EPCActionPhase Phase, float Duration)
{
	SetPhase(Phase);

	if (Duration <= 0.f)
	{
		AdvancePhase();
		return;
	}

	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	Task->OnFinish.AddDynamic(this, &UPCGameplayAbility::AdvancePhase);
	Task->ReadyForActivation();
}

void UPCGameplayAbility::AdvancePhase()
{
	const UPCAbilityData* Data = GetAbilityData();
	if (!Data)
	{
		EndAsCancelled();
		return;
	}

	switch (CurrentPhase)
	{
	case EPCActionPhase::Windup:
		StartPhase(EPCActionPhase::Active, Data->Timing.Active);
		break;
	case EPCActionPhase::Active:
		StartPhase(EPCActionPhase::Recovery, Data->Timing.Recovery);
		break;
	default:
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		break;
	}
}

void UPCGameplayAbility::SetPhase(EPCActionPhase NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}

	const EPCActionPhase OldPhase = CurrentPhase;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag OldTag = PhaseToTag(OldPhase);
		const FGameplayTag NewTag = PhaseToTag(NewPhase);
		if (OldTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(OldTag);
		}
		if (NewTag.IsValid())
		{
			ASC->AddLooseGameplayTag(NewTag);
		}
	}

	CurrentPhase = NewPhase;
	OnPhaseChanged(OldPhase, NewPhase);
}

FGameplayTag UPCGameplayAbility::PhaseToTag(EPCActionPhase Phase)
{
	switch (Phase)
	{
	case EPCActionPhase::Windup:   return PCTags::State_Action_Windup;
	case EPCActionPhase::Active:   return PCTags::State_Action_Active;
	case EPCActionPhase::Recovery: return PCTags::State_Action_Recovery;
	default:                       return FGameplayTag();
	}
}
```

- [ ] **Step 3: 근접 공격**

`Public/AbilitySystem/Abilities/PCAbility_MeleeAttack.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_MeleeAttack.generated.h"

/**
 * 근접 공격. 판정 구간 동안 정면 부채꼴을 반복해서 훑고, 처음 닿은 대상 하나에 적용한다.
 * 검 베기, 방패 밀기, 할퀴기가 모두 이 클래스이고 데이터만 다르다.
 */
UCLASS()
class PCCOMBAT_API UPCAbility_MeleeAttack : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	void ExecuteWindupCue();
	void TryHit();
	void StopHitChecks();

	bool bHasHit = false;
	FTimerHandle HitCheckTimer;
};
```

`Private/AbilitySystem/Abilities/PCAbility_MeleeAttack.cpp`

```cpp
#include "AbilitySystem/Abilities/PCAbility_MeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Data/PCAbilityData.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "TimerManager.h"

namespace
{
	/** 판정 구간 동안 범위를 다시 훑는 간격 */
	constexpr float HitCheckInterval = 0.02f;
}

UPCAbility_MeleeAttack::UPCAbility_MeleeAttack()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Attack));
}

void UPCAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasHit = false;
	StartPhase(EPCActionPhase::Windup, Data->Timing.Windup);
}

void UPCAbility_MeleeAttack::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	if (OldPhase == EPCActionPhase::Active)
	{
		StopHitChecks();
	}

	if (NewPhase == EPCActionPhase::Windup)
	{
		ExecuteWindupCue();
	}
	else if (NewPhase == EPCActionPhase::Active)
	{
		TryHit();
		if (!bHasHit && GetCurrentPhase() == EPCActionPhase::Active)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(HitCheckTimer, this, &UPCAbility_MeleeAttack::TryHit, HitCheckInterval, true);
			}
		}
	}
}

void UPCAbility_MeleeAttack::ExecuteWindupCue()
{
	const UPCAbilityData* Data = GetAbilityData();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Data || !Avatar || !ASC)
	{
		return;
	}

	FGameplayCueParameters Parameters;
	Parameters.Instigator = Avatar;
	Parameters.SourceObject = Data;
	Parameters.Location = Avatar->GetActorLocation();
	Parameters.RawMagnitude = Data->Timing.Windup;
	ASC->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Windup, Parameters);
}

void UPCAbility_MeleeAttack::TryHit()
{
	const UPCAbilityData* Data = GetAbilityData();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Data || !Avatar || bHasHit)
	{
		return;
	}

	FPCHitInfo Hit;
	Hit.Attacker = Avatar;
	Hit.Origin = Avatar->GetActorLocation();
	Hit.Direction = Avatar->GetActorForwardVector();
	Hit.Data = Data;

	AActor* Victim = nullptr;
	if (FPCHitResolver::ResolveAttack(Hit, Victim) != EPCHitOutcome::Miss)
	{
		bHasHit = true;
		StopHitChecks();
	}
}

void UPCAbility_MeleeAttack::StopHitChecks()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitCheckTimer);
	}
}
```

- [ ] **Step 4: 방어**

`Public/AbilitySystem/Abilities/PCAbility_Guard.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_Guard.generated.h"

/** 방패 올리기. 취소될 때까지 State.Guarding을 유지한다. 막는 판정은 맞는 쪽(APCCharacterBase)이 한다. */
UCLASS()
class PCCOMBAT_API UPCAbility_Guard : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
```

`Private/AbilitySystem/Abilities/PCAbility_Guard.cpp`

```cpp
#include "AbilitySystem/Abilities/PCAbility_Guard.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

UPCAbility_Guard::UPCAbility_Guard()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Guard));
	ActivationOwnedTags.AddTag(PCTags::State_Guarding);
}

void UPCAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	APCCharacterBase* Character = GetPCCharacter();
	if (!Data || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetGuardHalfAngle(Data->GuardHalfAngleDeg);
}
```

- [ ] **Step 5: 회피**

`Public/AbilitySystem/Abilities/PCAbility_Dodge.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_Dodge.generated.h"

/** 회피 스텝. 입력 방향으로 짧게 이동한다. 무적 시간은 없고, 범위 밖으로 나가야 피한 것이다. */
UCLASS()
class PCCOMBAT_API UPCAbility_Dodge : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	FVector DodgeDirection = FVector::ForwardVector;
};
```

`Private/AbilitySystem/Abilities/PCAbility_Dodge.cpp`

```cpp
#include "AbilitySystem/Abilities/PCAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

namespace
{
	/** 이동 속도 계산에서 0으로 나누지 않기 위한 최소 이동 시간 */
	constexpr float MinDodgeDuration = 0.05f;
}

UPCAbility_Dodge::UPCAbility_Dodge()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Dodge));
}

void UPCAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	APCCharacterBase* Character = GetPCCharacter();
	if (!Data || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 입력이 있으면 그 방향, 없으면 뒤로
	DodgeDirection = Character->GetLastMovementInputVector();
	DodgeDirection.Z = 0.f;
	if (!DodgeDirection.Normalize())
	{
		DodgeDirection = -Character->GetActorForwardVector();
	}

	StartPhase(EPCActionPhase::Windup, Data->Timing.Windup);
}

void UPCAbility_Dodge::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	if (NewPhase != EPCActionPhase::Active)
	{
		return;
	}

	const UPCAbilityData* Data = GetAbilityData();
	if (!Data)
	{
		return;
	}

	const float Duration = FMath::Max(Data->Timing.Active, MinDodgeDuration);
	const float Strength = Data->DodgeDistance / Duration;

	UAbilityTask_ApplyRootMotionConstantForce* Task = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, NAME_None, DodgeDirection, Strength, Duration,
		false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
	Task->ReadyForActivation();
}
```

- [ ] **Step 6: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 7: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add data-driven abilities: melee attack, guard, dodge"
```

---

### Task 9: 플레이어 캐릭터

**Files:**
- Create: `Source/PCCombat/Public/Characters/PCPlayerCharacter.h`, `Source/PCCombat/Private/Characters/PCPlayerCharacter.cpp`

- [ ] **Step 1: `Public/Characters/PCPlayerCharacter.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/** 입력 하나와 기술 태그 하나의 연결 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCAbilityInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** true면 누르는 동안 유지하고 떼면 취소한다 (방패 올리기) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool bHold = false;
};

/** 플레이어 캐릭터. 카메라와 입력만 맡고, 기술은 태그로 능력 시스템에 요청한다. */
UCLASS(Abstract)
class PCCOMBAT_API APCPlayerCharacter : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCPlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<TObjectPtr<UInputMappingContext>> MappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<FPCAbilityInputBinding> AbilityBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnAbilityInputPressed(FGameplayTag AbilityTag);
	void OnAbilityInputReleased(FGameplayTag AbilityTag);

	/** 방어 중에는 카메라 방향을 바라보게 해서 방패 방향을 조준할 수 있게 한다 */
	void OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount);
};
```

- [ ] **Step 2: `Private/Characters/PCPlayerCharacter.cpp`**

```cpp
#include "Characters/PCPlayerCharacter.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "PCGameplayTags.h"
#include "PCLog.h"

APCPlayerCharacter::APCPlayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCPlayerCharacter::OnGuardTagChanged);
}

void APCPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		for (const UInputMappingContext* Context : MappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
}

void APCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogPC, Error, TEXT("'%s': Enhanced Input 컴포넌트가 없습니다"), *GetNameSafe(this));
		return;
	}

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Move);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Look);
	}
	if (MouseLookAction)
	{
		Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Look);
	}

	for (const FPCAbilityInputBinding& Binding : AbilityBindings)
	{
		if (!Binding.InputAction || !Binding.AbilityTag.IsValid())
		{
			continue;
		}

		if (Binding.bHold)
		{
			// 누르는 동안 매 프레임 발동을 시도한다. 다른 행동이 끝나는 즉시 방어가 올라온다.
			Input->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::OnAbilityInputPressed, Binding.AbilityTag);
			Input->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &APCPlayerCharacter::OnAbilityInputReleased, Binding.AbilityTag);
		}
		else
		{
			Input->BindAction(Binding.InputAction, ETriggerEvent::Started, this, &APCPlayerCharacter::OnAbilityInputPressed, Binding.AbilityTag);
		}
	}
}

void APCPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!GetController() || HasStateTag(PCTags::State_Staggered) || IsDead())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, GetController()->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void APCPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void APCPlayerCharacter::OnAbilityInputPressed(FGameplayTag AbilityTag)
{
	AbilitySystemComponent->TryActivateByAbilityTag(AbilityTag);
}

void APCPlayerCharacter::OnAbilityInputReleased(FGameplayTag AbilityTag)
{
	AbilitySystemComponent->CancelByAbilityTag(AbilityTag);
}

void APCPlayerCharacter::OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bGuarding = NewCount > 0;
	bUseControllerRotationYaw = bGuarding;
	GetCharacterMovement()->bOrientRotationToMovement = !bGuarding;
}
```

- [ ] **Step 3: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 4: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add player character with tag-based ability input"
```

---

### Task 10: 몬스터 베이스

**Files:**
- Create: `Source/PCCombat/Public/Characters/PCMonsterBase.h`, `Source/PCCombat/Private/Characters/PCMonsterBase.cpp`

- [ ] **Step 1: `Public/Characters/PCMonsterBase.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCMonsterBase.generated.h"

/**
 * 몬스터의 부모. 몬스터 종류는 이 클래스의 블루프린트 자식에 데이터와 기술 목록을 넣어 만든다.
 * 1단계에서는 AI 없이 정해진 간격으로 기술 하나를 반복한다. 2단계에서 StateTree가 이 반복을 대체한다.
 */
UCLASS(Abstract)
class PCCOMBAT_API APCMonsterBase : public APCCharacterBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** true면 AutoAttackInterval마다 AutoAttackTag 기술을 쓴다 */
	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack")
	bool bAutoAttack = true;

	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack", meta = (Categories = "Ability"))
	FGameplayTag AutoAttackTag;

	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack", meta = (ClampMin = 0.1, Units = "s"))
	float AutoAttackInterval = 3.f;

private:
	/** 플레이어 쪽으로 몸을 돌린 뒤 공격한다. 공격이 시작된 뒤에는 방향이 고정된다. */
	void TryAutoAttack();

	FTimerHandle AutoAttackTimer;
};
```

- [ ] **Step 2: `Private/Characters/PCMonsterBase.cpp`**

```cpp
#include "Characters/PCMonsterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PCGameplayTags.h"
#include "TimerManager.h"

void APCMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoAttack && AutoAttackTag.IsValid())
	{
		GetWorldTimerManager().SetTimer(AutoAttackTimer, this, &APCMonsterBase::TryAutoAttack, AutoAttackInterval, true);
	}
}

void APCMonsterBase::TryAutoAttack()
{
	if (IsDead() || HasStateTag(PCTags::State_Staggered) || HasStateTag(PCTags::State_Action))
	{
		return;
	}

	if (const APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.Rotation());
		}
	}

	AbilitySystemComponent->TryActivateByAbilityTag(AutoAttackTag);
}
```

- [ ] **Step 3: 빌드**

빌드 명령 실행. 기대 결과: `Result: Succeeded`.

- [ ] **Step 4: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add monster base with interval auto attack"
```

---

### Task 11: 겉모습, HUD, 게임 모드

**Files:**
- Create: `Source/PCCombat/Public/Presentation/PCCombatPresenterComponent.h`, `Private/Presentation/PCCombatPresenterComponent.cpp`
- Create: `Source/PCCombat/Public/UI/PCCombatHUD.h`, `Private/UI/PCCombatHUD.cpp`
- Create: `Source/PCCombat/Public/Game/PCCombatGameMode.h`, `Private/Game/PCCombatGameMode.cpp`
- Modify: `Source/PCCombat/Public/Characters/PCCharacterBase.h`, `Private/Characters/PCCharacterBase.cpp`
- Modify: `Source/PCCombat/Public/Characters/PCMonsterBase.h`, `Private/Characters/PCMonsterBase.cpp`

- [ ] **Step 1: `Public/Presentation/PCCombatPresenterComponent.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PCCombatPresenterComponent.generated.h"

class APCCharacterBase;
class UStaticMesh;
class UStaticMeshComponent;
struct FGameplayCueParameters;
struct FPCHitShape;

/**
 * 임시 겉모습. 판정 코드가 내보낸 GameplayCue와 상태 태그를 듣고 도형과 디버그 표시로 보여준다.
 * 모델과 애니메이션이 준비되면 이 컴포넌트만 교체한다. 판정 코드는 이 컴포넌트를 모른다.
 */
UCLASS(ClassGroup = "PC", meta = (BlueprintSpawnableComponent))
class PCCOMBAT_API UPCCombatPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPCCombatPresenterComponent();

	/** 손에 검과 방패 도형을 붙일지 */
	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	bool bShowWeapons = true;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	TSoftObjectPtr<UStaticMesh> SwordMesh;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	TSoftObjectPtr<UStaticMesh> ShieldMesh;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	FName SwordSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	FName ShieldSocket = TEXT("hand_l");

protected:
	virtual void BeginPlay() override;

private:
	void HandleCombatCue(FGameplayTag CueTag, const FGameplayCueParameters& Parameters);
	void OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount);

	UStaticMeshComponent* CreateWeaponMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket, const FVector& Scale);
	void DrawSector(const FVector& Origin, const FVector& Forward, const FPCHitShape& Shape, const FColor& Color, float Duration) const;

	UPROPERTY()
	TObjectPtr<APCCharacterBase> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShieldComponent;
};
```

- [ ] **Step 2: `Private/Presentation/PCCombatPresenterComponent.cpp`**

```cpp
#include "Presentation/PCCombatPresenterComponent.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Characters/PCCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameplayEffectTypes.h"
#include "HAL/IConsoleManager.h"
#include "PCGameplayTags.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShowTelegraph(
		TEXT("pc.Combat.ShowTelegraph"), 0,
		TEXT("1이면 예비 동작 동안 공격 범위를 바닥에 표시한다"));

	const FVector SwordScale(0.05f, 0.05f, 0.9f);
	const FVector ShieldScale(0.6f, 0.6f, 0.05f);
	const FVector ShieldGuardLocation(45.f, 0.f, 20.f);
	const FRotator ShieldGuardRotation(90.f, 0.f, 0.f);
	const FVector OverheadOffset(0.f, 0.f, 130.f);
	constexpr float OverheadMarkerRadius = 18.f;
	constexpr float PopupTextDuration = 0.6f;
	constexpr int32 SectorSegments = 16;

	FColor ThreatColor(const UPCAbilityData* Data)
	{
		if (Data && Data->ThreatTag.MatchesTag(PCTags::Threat_Unblockable))
		{
			return FColor::Red;
		}
		if (Data && Data->ThreatTag.MatchesTag(PCTags::Threat_Heavy))
		{
			return FColor::Orange;
		}
		return FColor::Yellow;
	}
}

UPCCombatPresenterComponent::UPCCombatPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SwordMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
	ShieldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
}

void UPCCombatPresenterComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<APCCharacterBase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->OnCombatCue.AddUObject(this, &UPCCombatPresenterComponent::HandleCombatCue);

	if (bShowWeapons)
	{
		CreateWeaponMesh(SwordMesh, SwordSocket, SwordScale);
		ShieldComponent = CreateWeaponMesh(ShieldMesh, ShieldSocket, ShieldScale);

		OwnerCharacter->GetPCAbilitySystemComponent()
			->RegisterGameplayTagEvent(PCTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPCCombatPresenterComponent::OnGuardTagChanged);
	}
}

UStaticMeshComponent* UPCCombatPresenterComponent::CreateWeaponMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket, const FVector& Scale)
{
	UStaticMesh* Loaded = Mesh.LoadSynchronous();
	if (!Loaded)
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(OwnerCharacter);
	Component->SetStaticMesh(Loaded);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetupAttachment(OwnerCharacter->GetMesh(), Socket);
	Component->RegisterComponent();
	Component->SetRelativeScale3D(Scale);
	return Component;
}

void UPCCombatPresenterComponent::OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!ShieldComponent || !OwnerCharacter)
	{
		return;
	}

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false);
	if (NewCount > 0)
	{
		// 방어 중: 방패를 몸 앞으로 올린다
		ShieldComponent->AttachToComponent(OwnerCharacter->GetRootComponent(), Rules);
		ShieldComponent->SetRelativeLocationAndRotation(ShieldGuardLocation, ShieldGuardRotation);
	}
	else
	{
		ShieldComponent->AttachToComponent(OwnerCharacter->GetMesh(), Rules, ShieldSocket);
	}
}

void UPCCombatPresenterComponent::HandleCombatCue(FGameplayTag CueTag, const FGameplayCueParameters& Parameters)
{
	UWorld* World = GetWorld();
	if (!World || !OwnerCharacter)
	{
		return;
	}

	const FVector Location = OwnerCharacter->GetActorLocation();
	const UPCAbilityData* Data = Cast<UPCAbilityData>(Parameters.SourceObject.Get());

	if (CueTag == PCTags::GameplayCue_Combat_Windup)
	{
		const FColor Color = ThreatColor(Data);
		const float Windup = Parameters.RawMagnitude;

		// 머리 위 표시: 예비 동작 동안 유지된다
		DrawDebugSphere(World, Location + OverheadOffset, OverheadMarkerRadius, 12, Color, false, Windup, 0, 2.f);

		if (Data && CVarShowTelegraph.GetValueOnGameThread() != 0)
		{
			DrawSector(Location, OwnerCharacter->GetActorForwardVector(), Data->HitShape, Color, Windup + Data->Timing.Active);
		}
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Hit)
	{
		DrawDebugString(World, Location + OverheadOffset, TEXT("HIT"), nullptr, FColor::Red, PopupTextDuration);
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Block)
	{
		DrawDebugString(World, Location + OverheadOffset, TEXT("BLOCK"), nullptr, FColor::Cyan, PopupTextDuration);
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Interrupt)
	{
		DrawDebugString(World, Location + OverheadOffset * 1.2f, TEXT("STAGGER"), nullptr, FColor::Yellow, Parameters.RawMagnitude);
	}
}

void UPCCombatPresenterComponent::DrawSector(const FVector& Origin, const FVector& Forward, const FPCHitShape& Shape, const FColor& Color, float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Ground = Origin;
	Ground.Z -= OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.f;

	FVector FlatForward = Forward;
	FlatForward.Z = 0.f;
	FlatForward.Normalize();

	FVector Previous = Ground;
	for (int32 Index = 0; Index <= SectorSegments; ++Index)
	{
		const float Angle = -Shape.HalfAngleDeg + (2.f * Shape.HalfAngleDeg) * Index / SectorSegments;
		const FVector Point = Ground + FlatForward.RotateAngleAxis(Angle, FVector::UpVector) * Shape.Range;
		DrawDebugLine(World, Previous, Point, Color, false, Duration, 0, 3.f);
		Previous = Point;
	}
	DrawDebugLine(World, Previous, Ground, Color, false, Duration, 0, 3.f);
}
```

- [ ] **Step 3: 캐릭터 베이스에 겉모습 컴포넌트 추가**

`Public/Characters/PCCharacterBase.h`: 전방 선언 목록에 `class UPCCombatPresenterComponent;`를 추가하고, `VitalSet` 선언 아래에 추가한다.

```cpp
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCCombatPresenterComponent> Presenter;
```

`Private/Characters/PCCharacterBase.cpp`: include 목록에 `#include "Presentation/PCCombatPresenterComponent.h"`를 추가하고, 생성자의 `VitalSet = ...` 줄 아래에 추가한다.

```cpp
	Presenter = CreateDefaultSubobject<UPCCombatPresenterComponent>(TEXT("Presenter"));
```

- [ ] **Step 4: 몬스터는 검과 방패를 들지 않게 한다**

`Public/Characters/PCMonsterBase.h`: `GENERATED_BODY()` 아래에 추가한다.

```cpp
public:
	APCMonsterBase();
```

`Private/Characters/PCMonsterBase.cpp`: include 목록에 `#include "Presentation/PCCombatPresenterComponent.h"`를 추가하고, `BeginPlay` 위에 추가한다.

```cpp
APCMonsterBase::APCMonsterBase()
{
	Presenter->bShowWeapons = false;
}
```

- [ ] **Step 5: HUD**

`Public/UI/PCCombatHUD.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PCCombatHUD.generated.h"

/** 임시 HUD. 플레이어의 체력과 기력, 다른 캐릭터의 머리 위 체력을 막대로 그린다. */
UCLASS()
class PCCOMBAT_API APCCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Ratio, const FLinearColor& Color);
};
```

`Private/UI/PCCombatHUD.cpp`

```cpp
#include "UI/PCCombatHUD.h"
#include "Characters/PCCharacterBase.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

namespace
{
	constexpr float PlayerBarWidth = 320.f;
	constexpr float PlayerBarHeight = 18.f;
	constexpr float PlayerBarMargin = 40.f;
	constexpr float PlayerBarGap = 8.f;
	constexpr float WorldBarWidth = 110.f;
	constexpr float WorldBarHeight = 8.f;
	constexpr float WorldBarHeightOffset = 115.f;
	constexpr float BarBorder = 2.f;

	const FLinearColor BarBackColor(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor HealthColor(0.8f, 0.1f, 0.1f, 1.f);
	const FLinearColor StaminaColor(0.9f, 0.75f, 0.1f, 1.f);

	float SafeRatio(float Value, float Max)
	{
		return Max > 0.f ? FMath::Clamp(Value / Max, 0.f, 1.f) : 0.f;
	}
}

void APCCombatHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const APCCharacterBase* Player = Cast<APCCharacterBase>(GetOwningPawn());
	if (Player)
	{
		const float StaminaY = Canvas->SizeY - PlayerBarMargin - PlayerBarHeight;
		const float HealthY = StaminaY - PlayerBarGap - PlayerBarHeight;
		DrawBar(PlayerBarMargin, HealthY, PlayerBarWidth, PlayerBarHeight, SafeRatio(Player->GetHealth(), Player->GetMaxHealth()), HealthColor);
		DrawBar(PlayerBarMargin, StaminaY, PlayerBarWidth, PlayerBarHeight, SafeRatio(Player->GetStamina(), Player->GetMaxStamina()), StaminaColor);
	}

	for (TActorIterator<APCCharacterBase> It(GetWorld()); It; ++It)
	{
		const APCCharacterBase* Other = *It;
		if (Other == Player)
		{
			continue;
		}

		const FVector Screen = Project(Other->GetActorLocation() + FVector(0.f, 0.f, WorldBarHeightOffset));
		if (Screen.Z > 0.f)
		{
			DrawBar(Screen.X - WorldBarWidth * 0.5f, Screen.Y, WorldBarWidth, WorldBarHeight, SafeRatio(Other->GetHealth(), Other->GetMaxHealth()), HealthColor);
		}
	}
}

void APCCombatHUD::DrawBar(float X, float Y, float Width, float Height, float Ratio, const FLinearColor& Color)
{
	DrawRect(BarBackColor, X - BarBorder, Y - BarBorder, Width + BarBorder * 2.f, Height + BarBorder * 2.f);
	DrawRect(Color, X, Y, Width * Ratio, Height);
}
```

- [ ] **Step 6: 게임 모드**

`Public/Game/PCCombatGameMode.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PCCombatGameMode.generated.h"

/** 전투 실험용 게임 모드. 기본 폰은 블루프린트 자식에서 지정한다. */
UCLASS()
class PCCOMBAT_API APCCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APCCombatGameMode();
};
```

`Private/Game/PCCombatGameMode.cpp`

```cpp
#include "Game/PCCombatGameMode.h"
#include "UI/PCCombatHUD.h"

APCCombatGameMode::APCCombatGameMode()
{
	HUDClass = APCCombatHUD::StaticClass();
}
```

- [ ] **Step 7: 빌드 후 테스트**

빌드 명령, 이어서 테스트 명령 실행. 기대 결과: 빌드 성공, 테스트 2개 `Result={Success}`.

- [ ] **Step 8: 커밋**

```powershell
git add Source/PCCombat
git commit -m "Add placeholder presenter, HUD, and combat game mode"
```

---

### Task 12: 에셋 생성 스크립트

`Content/`는 저장소에 올리지 않으므로, 필요한 에셋을 스크립트로 다시 만들 수 있게 한다. 스크립트는 이미 있는 에셋은 불러와서 값만 다시 설정한다.

**Files:**
- Create: `Tools/pc_create_combat_assets.py`

만들어지는 에셋 (`/Game/PC/` 아래):

| 경로 | 내용 |
|---|---|
| `Input/IA_PC_Attack`, `IA_PC_Guard`, `IA_PC_Bash`, `IA_PC_Dodge` | 입력 액션 |
| `Input/IMC_PC_Combat` | 좌클릭=베기, 우클릭=방어, F=방패 밀기, 왼쪽 Shift=회피 |
| `Combat/Abilities/DA_PC_Slash`, `DA_PC_ShieldBash`, `DA_PC_Guard`, `DA_PC_Dodge`, `DA_PC_Claw` | 기술 데이터 |
| `Combat/DT_PC_Combatants` | 기본 수치 (CSV에서 가져옴) |
| `Combat/BP_PC_Player`, `BP_PC_Werewolf`, `BP_PC_CombatGameMode` | 블루프린트 자식 |
| `Maps/Lvl_PC_CombatTest` | 바닥, 조명, 시작 지점, 늑대인간 1마리 |

- [ ] **Step 1: `Tools/pc_create_combat_assets.py`**

```python
"""전투 프로토타입 1단계 에셋을 만든다. 여러 번 실행해도 된다.

실행: 에디터의 Output Log에서
    py "C:/UnrealEngine/ProjectC/Tools/pc_create_combat_assets.py"
"""
import os

import unreal

ROOT = "/Game/PC"
INPUT_DIR = f"{ROOT}/Input"
COMBAT_DIR = f"{ROOT}/Combat"
ABILITY_DIR = f"{COMBAT_DIR}/Abilities"
MAP_PATH = f"{ROOT}/Maps/Lvl_PC_CombatTest"
CSV_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Data", "DT_PC_Combatants.csv")

MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
UNARMED_ANIM_BP = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def load_or_create(name, folder, asset_class, factory):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    asset = asset_tools.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError(f"에셋을 만들지 못했습니다: {path}")
    return asset


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text(f'(TagName="{name}")')
    return tag


def make_key(name):
    key = unreal.Key()
    key.import_text(name)
    return key


def save(asset):
    eal.save_loaded_asset(asset)


# ---------- 입력 ----------
def create_input():
    actions = {}
    for name in ("Attack", "Guard", "Bash", "Dodge"):
        actions[name] = load_or_create(f"IA_PC_{name}", INPUT_DIR, unreal.InputAction, unreal.InputAction_Factory())
        save(actions[name])

    context = load_or_create("IMC_PC_Combat", INPUT_DIR, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    context.unmap_all()
    context.map_key(actions["Attack"], make_key("LeftMouseButton"))
    context.map_key(actions["Guard"], make_key("RightMouseButton"))
    context.map_key(actions["Bash"], make_key("F"))
    context.map_key(actions["Dodge"], make_key("LeftShift"))
    save(context)
    return actions, context


# ---------- 기술 데이터 ----------
def create_ability(name, ability_class, tag, **props):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PCAbilityData)
    data = load_or_create(name, ABILITY_DIR, unreal.PCAbilityData, factory)
    data.set_editor_property("ability_class", ability_class)
    data.set_editor_property("ability_tag", make_tag(tag))
    for key, value in props.items():
        data.set_editor_property(key, value)
    save(data)
    return data


def timing(windup, active, recovery):
    value = unreal.PCActionTiming()
    value.set_editor_property("windup", windup)
    value.set_editor_property("active", active)
    value.set_editor_property("recovery", recovery)
    return value


def shape(hit_range, half_angle):
    value = unreal.PCHitShape()
    value.set_editor_property("range", hit_range)
    value.set_editor_property("half_angle_deg", half_angle)
    return value


def create_abilities():
    light = make_tag("Threat.Light")
    return {
        "Slash": create_ability(
            "DA_PC_Slash", unreal.PCAbility_MeleeAttack, "Ability.Attack.Slash",
            timing=timing(0.25, 0.10, 0.35), stamina_cost=12.0,
            damage=15.0, guard_stamina_damage=15.0, stagger_time=0.0,
            stagger_only_on_windup=False, threat_tag=light, hit_shape=shape(190.0, 55.0)),
        "ShieldBash": create_ability(
            "DA_PC_ShieldBash", unreal.PCAbility_MeleeAttack, "Ability.Attack.ShieldBash",
            timing=timing(0.15, 0.08, 0.45), stamina_cost=20.0,
            damage=3.0, guard_stamina_damage=25.0, stagger_time=1.2,
            stagger_only_on_windup=True, threat_tag=light, hit_shape=shape(140.0, 45.0)),
        "Guard": create_ability(
            "DA_PC_Guard", unreal.PCAbility_Guard, "Ability.Guard",
            stamina_cost=0.0, guard_half_angle_deg=70.0),
        "Dodge": create_ability(
            "DA_PC_Dodge", unreal.PCAbility_Dodge, "Ability.Dodge",
            timing=timing(0.0, 0.20, 0.25), stamina_cost=20.0, dodge_distance=350.0),
        "Claw": create_ability(
            "DA_PC_Claw", unreal.PCAbility_MeleeAttack, "Ability.Attack.Claw",
            timing=timing(0.80, 0.12, 0.90), stamina_cost=0.0,
            damage=20.0, guard_stamina_damage=30.0, stagger_time=0.35,
            stagger_only_on_windup=False, threat_tag=light, hit_shape=shape(220.0, 50.0)),
    }


# ---------- 데이터테이블 ----------
def create_data_table():
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.PCCombatantRow.static_struct())
    table = load_or_create("DT_PC_Combatants", COMBAT_DIR, unreal.DataTable, factory)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, CSV_PATH):
        raise RuntimeError(f"CSV를 가져오지 못했습니다: {CSV_PATH}")
    save(table)
    return table


def row_handle(table, row_name):
    handle = unreal.DataTableRowHandle()
    handle.set_editor_property("data_table", table)
    handle.set_editor_property("row_name", row_name)
    return handle


# ---------- 블루프린트 ----------
def create_blueprint(name, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return load_or_create(name, COMBAT_DIR, unreal.Blueprint, factory)


def finish_blueprint(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)


def defaults_of(blueprint):
    return unreal.get_default_object(blueprint.generated_class())


def set_mannequin(defaults):
    mesh = defaults.get_editor_property("mesh")
    mesh.set_editor_property("skeletal_mesh_asset", eal.load_asset(MANNY_MESH))
    mesh.set_editor_property("anim_class", eal.load_asset(UNARMED_ANIM_BP).generated_class())


def input_binding(action, tag, hold):
    binding = unreal.PCAbilityInputBinding()
    binding.set_editor_property("input_action", action)
    binding.set_editor_property("ability_tag", make_tag(tag))
    binding.set_editor_property("hold", hold)
    return binding


def create_blueprints(actions, context, abilities, table):
    player = create_blueprint("BP_PC_Player", unreal.PCPlayerCharacter)
    defaults = defaults_of(player)
    set_mannequin(defaults)
    defaults.set_editor_property("combatant_row", row_handle(table, "Player"))
    defaults.set_editor_property("starting_abilities", [abilities["Slash"], abilities["ShieldBash"], abilities["Guard"], abilities["Dodge"]])
    defaults.set_editor_property("mapping_contexts", [
        eal.load_asset("/Game/Input/IMC_Default"),
        eal.load_asset("/Game/Input/IMC_MouseLook"),
        context,
    ])
    defaults.set_editor_property("move_action", eal.load_asset("/Game/Input/Actions/IA_Move"))
    defaults.set_editor_property("look_action", eal.load_asset("/Game/Input/Actions/IA_Look"))
    defaults.set_editor_property("mouse_look_action", eal.load_asset("/Game/Input/Actions/IA_MouseLook"))
    defaults.set_editor_property("ability_bindings", [
        input_binding(actions["Attack"], "Ability.Attack.Slash", False),
        input_binding(actions["Bash"], "Ability.Attack.ShieldBash", False),
        input_binding(actions["Guard"], "Ability.Guard", True),
        input_binding(actions["Dodge"], "Ability.Dodge", False),
    ])
    finish_blueprint(player)

    werewolf = create_blueprint("BP_PC_Werewolf", unreal.PCMonsterBase)
    defaults = defaults_of(werewolf)
    set_mannequin(defaults)
    defaults.set_editor_property("combatant_row", row_handle(table, "Werewolf"))
    defaults.set_editor_property("starting_abilities", [abilities["Claw"]])
    defaults.set_editor_property("auto_attack", True)
    defaults.set_editor_property("auto_attack_tag", make_tag("Ability.Attack.Claw"))
    defaults.set_editor_property("auto_attack_interval", 3.0)
    finish_blueprint(werewolf)

    game_mode = create_blueprint("BP_PC_CombatGameMode", unreal.PCCombatGameMode)
    defaults_of(game_mode).set_editor_property("default_pawn_class", player.generated_class())
    finish_blueprint(game_mode)

    return player, werewolf, game_mode


# ---------- 테스트 레벨 ----------
def create_level(werewolf, game_mode):
    if eal.does_asset_exist(MAP_PATH):
        unreal.log(f"레벨이 이미 있어서 건너뜁니다: {MAP_PATH}")
        return

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError(f"레벨을 만들지 못했습니다: {MAP_PATH}")

    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, -50.0))
    floor.static_mesh_component.set_static_mesh(eal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(40.0, 40.0, 1.0))
    floor.set_actor_label("Floor")

    actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 600.0), unreal.Rotator(pitch=-50.0, yaw=35.0, roll=0.0))
    sky_light = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0.0, 0.0, 600.0))
    sky_light.light_component.set_editor_property("real_time_capture", True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))

    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-400.0, 0.0, 100.0))

    wolf = actors.spawn_actor_from_class(werewolf.generated_class(), unreal.Vector(300.0, 0.0, 130.0), unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0))
    wolf.set_actor_scale3d(unreal.Vector(1.3, 1.3, 1.3))
    wolf.set_actor_label("Werewolf")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", game_mode.generated_class())
    level_editor.save_current_level()


def main():
    actions, context = create_input()
    abilities = create_abilities()
    table = create_data_table()
    _, werewolf, game_mode = create_blueprints(actions, context, abilities, table)
    create_level(werewolf, game_mode)
    unreal.log("전투 프로토타입 에셋 생성 완료. /Game/PC/Maps/Lvl_PC_CombatTest 를 열고 Play 하세요.")


main()
```

- [ ] **Step 2: 스크립트 실행**

에디터로 `ProjectC.uproject`를 연다. 새 모듈을 다시 빌드할지 물으면 "예"를 누른다. 에디터 아래쪽 Output Log의 입력란에서 `Cmd`를 선택하고 실행한다.

```
py "C:/UnrealEngine/ProjectC/Tools/pc_create_combat_assets.py"
```

기대 결과: 로그에 `전투 프로토타입 에셋 생성 완료`가 나오고, 콘텐츠 브라우저의 `/Game/PC/` 아래에 위 표의 에셋이 있다.

Python API 이름이 달라 오류가 나면 오류 메시지의 줄을 고친다. `unreal` 모듈의 이름은 에디터 Output Log에서 `py print([n for n in dir(unreal) if "PC" in n])`로 확인할 수 있다. 스크립트로 해결되지 않는 항목은 에디터에서 직접 만든다. 그 경우 필요한 값은 스크립트의 `create_abilities`와 `create_blueprints`에 있는 값 그대로다.

- [ ] **Step 3: 커밋**

```powershell
git add Tools
git commit -m "Add editor script that generates combat prototype assets"
```

---

### Task 13: 플레이 테스트

에디터에서 `/Game/PC/Maps/Lvl_PC_CombatTest`를 열고 Play를 누른다.

조작: WASD 이동, 마우스 시점, 좌클릭 베기, 우클릭(누르고 있기) 방패 올리기, F 방패 밀기, 왼쪽 Shift 회피. 콘솔(`~`)에서 `pc.Combat.ShowTelegraph 1`로 바닥 범위 표시를 켠다.

- [ ] **Step 1: 판정 체크리스트 확인**

| # | 행동 | 기대 결과 |
|---|---|---|
| 1 | 늑대인간에게 다가가 베기 | 늑대인간 머리 위 `HIT`, 체력 막대 감소, 내 기력 감소 |
| 2 | 닿지 않는 거리에서 베기 | 표시 없음, 기력만 감소 |
| 3 | 늑대인간을 등지고 베기 | 표시 없음 |
| 4 | 늑대인간 머리 위에 노란 표시가 뜬 뒤 가만히 서 있기 | 내 머리 위 `HIT`, 체력 감소, 짧게 멈춤 |
| 5 | 노란 표시가 뜬 동안 늑대인간을 향해 방패 올리기 | `BLOCK`, 체력 그대로, 기력 감소 |
| 6 | 방패를 올린 채 늑대인간에게 등을 보이기 | `HIT`, 체력 감소 |
| 7 | 노란 표시가 뜬 동안 뒤로 걸어서 범위 밖으로 나가기 | 표시 없음, 피해 없음 |
| 8 | 노란 표시가 뜬 동안 옆으로 회피 | 표시 없음, 피해 없음 |
| 9 | 노란 표시가 뜬 동안 방패 밀기를 맞히기 | 늑대인간 `STAGGER`, 할퀴기가 나오지 않음 |
| 10 | 노란 표시가 없을 때 방패 밀기를 맞히기 | `HIT`만 뜨고 `STAGGER` 없음 |
| 11 | 기력이 바닥날 때까지 베기 연타 | 기력이 부족하면 베기가 나가지 않음, 잠시 뒤 기력 회복 |
| 12 | 방패를 올린 채 기력이 바닥날 때까지 막기 | 방어가 풀리고 `STAGGER` |
| 13 | 늑대인간 체력을 0으로 만들기 | 공격을 멈추고 3초 뒤 체력이 가득 찬 채 다시 공격 |
| 14 | `pc.Combat.ShowTelegraph 1` 후 할퀴기 지켜보기 | 예비 동작 동안 바닥에 부채꼴 표시 |

- [ ] **Step 2: 결과 기록**

기대와 다른 항목은 번호와 실제 결과를 적는다. 수치 문제(시간이 너무 짧다, 범위가 너무 넓다)는 `Tools/pc_create_combat_assets.py`의 값을 고치고 스크립트를 다시 실행한다. 판정 버그는 `superpowers:systematic-debugging`으로 원인을 찾는다.

- [ ] **Step 3: 조율한 수치가 있으면 커밋**

```powershell
git add Tools
git commit -m "Tune combat prototype numbers after playtest"
```
