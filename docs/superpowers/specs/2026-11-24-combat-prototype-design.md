# 전투 프로토타입 설계 (1대1: 검과 방패 대 늑대인간)

- 작성일: 2026-11-24
- 엔진: UE 5.8.4 (`C:\Program Files\Epic Games\UE_5.8`)
- 상태: 사용자 승인됨 (구조). 구현 계획: `docs/superpowers/plans/2026-11-24-combat-prototype-stage1.md`
- 이 문서는 실험 결과에 따라 계속 바뀐다.

## 1. 목적

MMORPG 본 개발 전에 전투 판정 규칙이 재미있는지 확인한다. 네트워크는 다루지 않고 싱글 기준이다. 결과가 쓸 만하면 본 개발에 채택할 수 있도록, 엔진 표준 도구(GAS, 데이터 에셋, 데이터테이블, StateTree)와 별도 모듈로 본 개발 수준의 구조를 잡는다.

## 2. 전투 컨셉

- 세계는 판타지지만 인간은 마법을 쓰지 않는다. 인간은 무기로 실제로 가능한 기술만 쓴다. 몬스터만 초자연 능력을 쓴다.
- 논타겟 액션.
- **확정타 없음.** 공격은 실제로 휘둘러지는 물체이고, 경로에서 처음 닿은 대상 하나에 맞는다.
- 방패는 정면 각도 안의 공격만 막는다. 막으면 기력이 줄어든다.
- 회피 스텝에는 무적 시간이 없다. 범위 밖으로 몸이 빠져야 피한 것이다.
- **위험한 공격일수록 더 분명하게 예고한다.** 물리 공격은 예비 동작으로 읽게 하고, 바닥 범위 표시는 켜고 끌 수 있는 토글로 둔다.

## 3. 범위

### 1단계 (첫 테스트)

- 플레이어: 검과 방패. 기술 4개와 기력.
  - 베기: 기본 근접 공격.
  - 방패 올리기: 누르는 동안 정면 공격을 막음. 막을 때마다 기력 감소.
  - 방패 밀기: 짧은 거리 근접 공격. 피해는 낮고, 예비 동작 중인 상대의 공격을 끊고 경직시킴.
  - 회피 스텝: 입력 방향으로 짧게 이동.
- 늑대인간: AI 없이 제자리에서 정해진 간격으로 할퀴기를 반복.
- 확인할 판정: 맞음, 막힘, 빗나감(거리/회피), 옆·뒤에서 맞으면 방패 무효, 밀쳐서 끊기, 기력 부족 시 발동 불가.
- 표시: 체력과 기력, 예비 동작 예고, 바닥 범위 표시 토글.

### 2단계 (다음 세션)

- StateTree로 늑대인간이 접근, 공격 선택(할퀴기/돌진), 후퇴.
- 위험 등급별 예고 강도 다듬기.

### 범위 밖

- 네트워크, 아군 보호 장면, 쳐내기(타이밍 판정), 강공격, 다른 무기, 스탯 성장, 인벤토리.

## 4. 설계 원칙

1. 동작이 다르면 클래스, 숫자만 다르면 데이터.
2. 상속은 얕게, 기능은 컴포넌트로 조립.
3. 판정과 겉모습을 분리한다. 판정은 이벤트와 GameplayCue 태그만 내보낸다.
4. 시스템 사이는 인터페이스, 태그, 델리게이트로 연결한다.
5. 헤더는 전방 선언을 쓰고, 공용 정의는 Types 헤더에 모은다.
6. 에셋은 하드 참조를 피하고 `TSoftObjectPtr`를 쓴다.
7. 매직 넘버를 쓰지 않는다. 수치는 데이터 에셋과 데이터테이블에 둔다.
8. Tick을 최소화한다. 단계 진행은 타이머 또는 능력 태스크로 한다.

## 5. 모듈과 폴더

```
Source/
├─ ProjectC/                    기존 템플릿. 수정하지 않음
├─ PCCore/
│   ├─ PCGameplayTags.h/.cpp    네이티브 게임플레이 태그 선언
│   └─ PCLog.h/.cpp             로그 카테고리
└─ PCCombat/
    ├─ Types/PCCombatTypes.h
    ├─ AbilitySystem/
    │   ├─ PCAbilitySystemComponent
    │   ├─ Attributes/PCVitalAttributeSet
    │   ├─ Abilities/PCGameplayAbility, PCAbility_MeleeAttack, PCAbility_Guard, PCAbility_Dodge
    │   └─ Effects/             피해, 기력 소모, 기력 회복, 경직
    ├─ Data/PCAbilityData, PCCombatantRow
    ├─ Hit/PCHitResolver, PCHittable
    ├─ Characters/PCCharacterBase, PCPlayerCharacter, PCMonsterBase
    ├─ AI/                      2단계
    ├─ Presentation/            임시 겉모습, 예고 표시 토글
    └─ UI/
```

- 각 모듈은 `Public/`(헤더)과 `Private/`(구현) 폴더로 나누고, 위 폴더 구조는 양쪽에 같은 모양으로 둔다. 게임 모드는 `Game/`에 둔다.
- 의존 방향: `PCCombat → PCCore`. 두 모듈 모두 `ProjectC`(템플릿) 헤더를 포함하지 않는다.
- 이후 실험은 `PCStats`, `PCInventory` 같은 모듈로 추가하고, 공통 의존은 `PCCore`로 한정한다.
- 접두사는 `PC`. 파일 하나에 클래스 하나, 파일명은 클래스명과 같게 한다.
- `.uproject`에 `GameplayAbilities` 플러그인과 두 모듈을 추가한다. Target 파일의 `ExtraModuleNames`에도 추가한다.

## 6. 클래스 계층

```
UGameplayAbility
└─ UPCGameplayAbility (abstract)     데이터 에셋의 숫자로 예비 동작 → 판정 → 후딜 진행, 단계 태그 부여
    ├─ UPCAbility_MeleeAttack        판정 구간에 PCHitResolver 호출
    ├─ UPCAbility_Guard              입력을 누르는 동안 State.Guarding 유지
    └─ UPCAbility_Dodge              입력 방향으로 짧게 이동

ACharacter
└─ APCCharacterBase (IAbilitySystemInterface, IPCHittable)
    ├─ APCPlayerCharacter            카메라, 입력 → 능력 발동
    └─ APCMonsterBase                데이터테이블 행으로 수치 초기화, 능력 목록 보유
```

- 검 베기, 방패 밀기, 할퀴기는 모두 `UPCAbility_MeleeAttack`이고 데이터 에셋만 다르다.
- 늑대인간은 C++ 클래스가 없다. `APCMonsterBase`의 블루프린트 자식에 데이터와 능력 목록을 넣는다.
- `IPCHittable`은 능력 시스템이 없는 대상(상자 등)도 공격 경로에 놓일 수 있게 하는 얇은 인터페이스다.

## 7. 게임플레이 태그

```
State.Action.Windup / Active / Recovery
State.Guarding / Staggered / Dead / StaminaRegenPaused
Ability.Attack / Guard / Dodge
Threat.Light / Heavy / Unblockable
Data.Damage / Stamina / Duration            이펙트에 수치를 전달 (SetByCaller)
GameplayCue.Combat.Windup / Hit / Block / Interrupt
```

코드가 참조하는 태그는 `PCCore`에 네이티브로 선언한다. 개별 기술 이름(`Ability.Attack.Slash`, `Ability.Attack.ShieldBash`, `Ability.Attack.Claw`)은 콘텐츠 전용이라 `Config/DefaultGameplayTags.ini`에 둔다.

규칙:
- `State.Staggered` 또는 `State.Dead`이면 모든 능력 발동이 막힌다.
- 한 번에 하나의 행동만 한다. `State.Action.*`이나 `State.Guarding`이 있으면 다른 능력은 발동하지 않는다.
- 경직이 부여되면 진행 중인 능력이 모두 취소된다.
- 상태와 수치 변화는 모두 GameplayEffect가 맡는다. 이펙트는 `UPCGameplayEffect`의 자식으로 C++에 정의하고, 수치는 SetByCaller로 받는다.

| 이펙트 | 종류 | 하는 일 |
|---|---|---|
| `PCEffect_Damage` | 즉시 | 체력 변화 |
| `PCEffect_Stamina` | 즉시 | 기력 변화 (기술 비용, 막았을 때 소모) |
| `PCEffect_Stagger` | 기간 | `State.Staggered` 부여 |
| `PCEffect_Dead` | 무기한 | `State.Dead` 부여. 부활할 때 제거 |
| `PCEffect_StaminaRegen` | 무기한, 0.1초 주기 | 기력 회복. `State.Guarding`, `State.Dead`, `State.StaminaRegenPaused` 중 하나라도 있으면 멈춤 |
| `PCEffect_StaminaRegenDelay` | 기간 | `State.StaminaRegenPaused` 부여. 기력을 쓸 때마다 시간 갱신 |

- 캐릭터는 태그 변화에 반응만 한다. `State.Staggered`가 붙으면 능력을 취소하고 멈추며, `State.Dead`가 붙으면 능력을 취소하고 이동을 끈 뒤 부활 타이머를 건다.
- 처음에는 경직, 사망, 기력 회복을 캐릭터의 태그와 타이머로 처리했으나, 플레이 테스트 중 사용자 요청으로 이펙트로 옮겼다 (2026-11-24).

## 8. 데이터

### `UPCAbilityData` (UPrimaryDataAsset, 기술 하나당 하나)

```cpp
TSubclassOf<UGameplayAbility> AbilityClass; // 이 기술을 실행하는 능력 클래스
FGameplayTag        AbilityTag;          // 입력과 AI가 이 태그로 발동
FPCActionTiming     Timing;              // Windup, Active, Recovery (초)
float               StaminaCost;
float               Damage;
float               GuardStaminaDamage;  // 막혔을 때 상대 기력을 깎는 양
float               StaggerTime;         // 맞은 쪽 경직 시간. 0이면 경직 없음
bool                bStaggerOnlyOnWindup; // true면 상대가 예비 동작 중일 때만 경직 (방패 밀기)
FGameplayTag        ThreatTag;
FPCHitShape         HitShape;            // 정면 부채꼴: Range, HalfAngleDeg
float               GuardHalfAngleDeg;   // 방어 전용: 정면 기준 막을 수 있는 반각
float               DodgeDistance;       // 회피 전용: 이동 거리 (cm)
TSoftObjectPtr<UAnimMontage> Montage;    // 비어 있어도 동작
```

방어와 회피도 같은 에셋 형식을 쓰고, 해당 능력이 쓰지 않는 필드는 0으로 둔다. 방어는 `Timing`을 쓰지 않고 입력을 누르는 동안 유지되며, 회피는 `Timing.Active` 동안 `DodgeDistance`만큼 이동한다.

### `FPCCombatantRow` (데이터테이블 행)

`MaxHealth`, `MaxStamina`, `StaminaRegenPerSec`, `StaminaRegenDelay`, `MoveSpeed`. CSV로 작성해 에디터에서 가져온다.

## 9. 공격 한 번의 흐름

1. 입력 또는 AI가 능력 발동을 요청한다. 기력 부족, `State.Staggered`, 다른 행동 진행 중이면 발동하지 않는다.
2. 예비 동작: `State.Action.Windup` 부여, `GameplayCue.Combat.Windup` 실행.
3. 판정: 판정 구간 동안 `PCHitResolver`가 공격 범위(정면 부채꼴)를 짧은 간격으로 반복해서 훑고, 범위 안에서 가장 가까운 `IPCHittable` 하나를 고른다. 한 번 닿으면 더 훑지 않는다. 구간이 끝날 때까지 닿은 대상이 없으면 빗나감.
4. 대상의 `ReceiveHit`:
   - `State.Guarding`이고, 공격이 정면 허용 각도 안에서 왔고, 위험 등급이 `Unblockable`이 아니면 **막힘**. 기력을 `GuardStaminaDamage`만큼 깎는다. 기력이 바닥나면 방어가 풀리고 경직된다.
   - 그 외에는 **맞음**. 피해 이펙트를 적용하고 `StaggerTime`만큼 경직시킨다. 경직되면 진행 중이던 공격이 취소된다.
   - `bStaggerOnlyOnWindup`인 공격은 대상이 `State.Action.Windup`일 때만 경직시킨다. 방패 밀기는 타이밍을 맞춰야 상대의 공격을 끊는다.
5. 후딜 → 능력 종료. 기력은 일정 시간 뒤 회복을 시작한다.

## 10. 겉모습 (임시)

- 캐릭터는 마네킹. 늑대인간은 크기와 색을 바꾼 마네킹.
- 검과 방패는 손에 붙인 기본 도형. 방어 중에는 방패 도형이 앞으로 올라온다.
- 예비 동작은 머리 위 표시로 알린다. 색은 위험 등급에 따라 노랑(Light), 주황(Heavy), 빨강(Unblockable)이다.
- 맞음, 막힘, 경직은 머리 위 글자로 표시한다.
- 바닥 범위 표시는 콘솔 변수 `pc.Combat.ShowTelegraph`로 켜고 끈다.
- 체력과 기력은 임시 HUD가 막대로 그린다.
- 모두 GameplayCue와 이벤트를 듣는 쪽에 두어, 나중에 모델과 애니메이션으로 교체해도 판정 코드는 바뀌지 않게 한다.

## 11. 에셋

`Content/`는 저장소에 올리지 않는다. 대신 아래 에셋을 만드는 에디터 Python 스크립트(`Tools/pc_create_combat_assets.py`)를 저장소에 두고, 에디터에서 한 번 실행해 만든다. 수치를 바꿀 때도 스크립트의 값을 고치고 다시 실행한다. 스크립트로 안 되는 항목은 에디터에서 직접 만든다.

- 플레이어와 늑대인간의 블루프린트 자식 (메시, 능력 목록 지정)
- 기술 데이터 에셋 (베기, 방패 밀기, 방어, 회피, 할퀴기)
- 데이터테이블 (CSV 가져오기)
- 입력 액션과 매핑 컨텍스트
- 테스트 레벨과 게임 모드 지정
- 2단계: StateTree

## 12. 검증

- 빌드: `Build.bat ProjectCEditor Win64 Development`가 오류 없이 끝난다.
- 플레이 테스트 체크리스트(3절의 "확인할 판정")를 에디터에서 직접 확인한다.
- 자동화 테스트(`PC.Combat.*`, `Source/PCCombat/Private/Tests/`)로 판정 규칙을 확인한다.
  - `HitResolver.Sector`, `HitResolver.Guard`: 부채꼴 범위와 정면 방어 각도 계산.
  - `Rules.ReceiveHit`: 맞음, 정면 방어, 등 뒤 피격, 막을 수 없는 공격, 방어 중 기력 고갈, 예비 동작 중에만 걸리는 경직, 사망.
  - `Rules.FirstHit`: 처음 닿은 대상만 맞음, 사이에 끼어든 캐릭터가 대신 맞거나 방패로 막음, 범위 밖 빗나감.
  - `Rules.AbilityFlow`: 발동, 기력 소모, 단계 진행, 행동 중 재발동 불가, 경직으로 공격 끊김, 기력 부족.
  - `Rules.StateEffects`: 경직이 시간이 지나면 풀림, 기력 회복 지연과 주기 회복, 방어 중 회복 정지, 사망과 부활.
- 근접 공격의 결과는 Output Log에 `[전투]`로 시작하는 줄로 남는다.

## 13. 검토했지만 채택하지 않은 안

| 안 | 채택하지 않은 이유 |
|---|---|
| 탭 타겟, 하이브리드 타겟팅 | 방향, 거리, 타이밍이 판정에 들어가야 방패 기술이 의미를 가짐 |
| 바닥 범위 표시를 기본 예고 수단으로 | 바닥을 보게 되고 걸어 나가는 게 항상 정답이 됨. 토글로만 둠 |
| 템플릿 `Variant_Combat` 확장 | 판정이 몽타주 노티파이에 묶여 있고 플레이어/적 코드가 중복됨 |
| 자체 행동 시스템 (GAS 없이) | 채택 가능성을 고려하면 나중에 GAS로 옮기는 재작업이 큼 |
| 수치를 C++ 기본값에 | 데이터로 따로 관리하는 방침과 맞지 않음 |
| 처음부터 몽타주 기반 타이밍 | 애니메이션 에셋이 준비될 때까지 진행이 막힘 |
| "의료를 지켜라" 장면부터 | 기본 판정을 1대1에서 먼저 다듬기로 함 |

## 14. 2단계: 몬스터 구조와 AI (2026-11-24)

몬스터의 공격 패턴을 다양하게 실험하는 것이 목적이다. 몬스터를 종류, 등급, 서식지, 행동 방식이라는 서로 다른 축으로 구분하고, 축마다 맞는 수단을 쓴다.

### 구분 축

| 축 | 수단 |
|---|---|
| 종류 (늑대인간 등) | 몬스터 정의 에셋 `UPCMonsterData`. 몬스터 하나가 에셋 하나다 |
| 등급 (일반, 엘리트, 보스) | 데이터테이블 `DT_PC_MonsterRanks`의 행. 등급 태그와 배율(체력, 피해, 경직 시간)을 가진다 |
| 서식지 | 정의 에셋의 태그 (`Monster.Habitat.*`). 이번 단계는 태그만 있고 지역별 스포너는 없다 |
| 행동 방식 | StateTree. 행동의 뼈대가 다를 때만 새 트리를 만든다 |

몬스터 종류마다 C++ 클래스를 만들지 않는다. `APCMonsterBase` 하나가 정의 에셋을 읽어 자신을 구성한다.

### 공격 패턴은 데이터가 정한다

"언제 어떤 공격을 쓰는가"는 StateTree가 아니라 정의 에셋의 공격 목록(`FPCMonsterAttackOption`)이 정한다. 항목마다 기술 태그, 거리 범위, 뽑힐 비중, 재사용 시간, 체력 구간, 공격 뒤 숨 고르기 시간이 있다. 공격 추가와 선택 조건 변경은 에디터에서 데이터만 고치면 되고 빌드가 필요 없다.

기본 트리(`ST_PC_MeleeMonster`)의 뼈대:

```
Root
├─ Dead        State.Dead 동안 대기
├─ Staggered   State.Staggered 동안 대기
├─ Combat      대상이 있을 때
│   ├─ Attack     지금 거리에서 쓸 수 있는 공격이 있으면 사용 → Recover
│   ├─ Approach   공격 거리 밖이면 접근
│   ├─ Hold       거리는 맞는데 전부 재사용 대기 중이면 잠깐 대기
│   └─ Recover    공격 뒤 숨 고르기
└─ Idle        대상 찾기
```

공격이 시작되면 방향이 고정된다. 예비 동작 중에 옆으로 비켜서면 빗나간다.

### 판정 모양과 적용 규칙

기술 데이터의 `HitShape`에서 고른다.

- 모양: 부채꼴(`Sector`), 원(`Circle`)
- 적용 규칙: 처음 닿은 하나(`FirstOnly`, 무기 공격), 범위 안 전원(`AllInArea`, 휩쓸기와 내려찍기)

브레스(지속 판정, 엄폐물 고려)와 투사체는 새 능력 클래스가 필요해서 다음 실험으로 남긴다.

### 추가된 것

- 속성 `AttackPower`(주는 피해 배율), `StaggerResistance`(받는 경직 시간 감소). 등급이 초기값을 정한다.
- 기술 `UPCAbility_LungeAttack`: 판정 구간 동안 앞으로 이동하며 훑는다.
- `APCMonsterAIController`: 대상 찾기, 거리, 공격 선택. 선택 계산은 `FPCMonsterAttackSelector`에 있다.
- StateTree 부품: 태스크 4개(대상 찾기, 접근, 공격 사용, 숨 고르기), 조건 2개(상태 태그, 전투 상황).
- 에디터 모듈 `PCCombatEditor`: 기본 행동 트리를 코드로 생성한다.
- 이동은 내비게이션 메시 길찾기를 쓴다. 내비게이션 메시는 실행 중에 생성된다(`RuntimeGeneration=Dynamic`). 경로를 못 구하면 직진으로 대체한다.

### 에셋 방침 변경

`Content/PC/`는 저장소에 포함한다. 에디터에서 한 실험이 커밋에 남게 하기 위해서다. 에셋 생성 스크립트는 에셋이 없을 때만 만들고, 이미 있는 에셋은 `--overwrite`를 붙였을 때만 스크립트 값으로 되돌린다. 그 밖의 에셋을 저장소에 넣을 때는 먼저 상의한다.

### 확인

자동화 테스트 5개 추가: `Monster.AttackSelector`, `Monster.HitShapes`, `Monster.Rank`, `Monster.Lunge`, `Monster.Behavior`(코드로 만든 트리로 몬스터가 대상을 찾고, 접근하고, 공격하고, 경직 뒤 재개하고, 대상이 죽으면 놓고 되살아나면 다시 찾는지).

### 이번 단계에 넣지 않은 것

엘리트와 보스 몬스터의 실제 콘텐츠, 지역별 스포너, 브레스와 투사체, 보스 페이즈 연출, 아군 보호 장면.