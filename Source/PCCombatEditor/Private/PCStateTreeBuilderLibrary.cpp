#include "PCStateTreeBuilderLibrary.h"
#include "AI/StateTree/PCMonsterStateTreeConditions.h"
#include "AI/StateTree/PCMonsterStateTreeTasks.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Misc/FeedbackContext.h"
#include "Misc/PackageName.h"
#include "PCGameplayTags.h"
#include "PCLog.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeFactory.h"
#include "StateTreeState.h"
#include "UObject/Package.h"

namespace
{
	/** 상태를 다시 고르기 전의 짧은 대기 시간들 */
	constexpr float DeadPollInterval = 0.5f;
	constexpr float StaggerPollInterval = 0.1f;
	constexpr float HoldInterval = 0.25f;
	constexpr float MinRecoverTime = 0.2f;

	void AddStateTagCondition(UStateTreeState& State, FGameplayTag Tag)
	{
		auto& Condition = State.AddEnterCondition<FPCStateTreeCondition_StateTag>();
		Condition.GetInstanceData().Tag = Tag;
	}

	void AddMonsterCheckCondition(UStateTreeState& State, EPCMonsterCheck Check, bool bInvert = false)
	{
		auto& Condition = State.AddEnterCondition<FPCStateTreeCondition_MonsterCheck>();
		Condition.GetInstanceData().Check = Check;
		Condition.GetInstanceData().bInvert = bInvert;
	}

	void AddRestTask(UStateTreeState& State, float Duration, bool bAddAttackRest = false)
	{
		auto& Rest = State.AddTask<FPCStateTreeTask_Rest>();
		Rest.GetInstanceData().Duration = Duration;
		Rest.GetInstanceData().bAddAttackRest = bAddAttackRest;
	}

	/** 상태가 끝나면(성공이든 실패든) Target 상태로 간다 */
	void GoToWhenCompleted(UStateTreeState& State, const UStateTreeState& Target)
	{
		State.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Target);
	}

	void PopulateMeleeMonsterTree(UStateTreeEditorData& EditorData)
	{
		UStateTreeState& Root = *EditorData.SubTrees[0];

		// 위에서부터 순서대로 들어갈 수 있는 첫 상태가 선택된다

		UStateTreeState& Dead = Root.AddChildState(TEXT("Dead"));
		AddStateTagCondition(Dead, PCTags::State_Dead);
		AddRestTask(Dead, DeadPollInterval);
		GoToWhenCompleted(Dead, Root);

		UStateTreeState& Staggered = Root.AddChildState(TEXT("Staggered"));
		AddStateTagCondition(Staggered, PCTags::State_Staggered);
		AddRestTask(Staggered, StaggerPollInterval);
		GoToWhenCompleted(Staggered, Root);

		UStateTreeState& Combat = Root.AddChildState(TEXT("Combat"));
		AddMonsterCheckCondition(Combat, EPCMonsterCheck::HasTarget);
		{
			UStateTreeState& Attack = Combat.AddChildState(TEXT("Attack"));
			UStateTreeState& Approach = Combat.AddChildState(TEXT("Approach"));
			UStateTreeState& Hold = Combat.AddChildState(TEXT("Hold"));
			UStateTreeState& Recover = Combat.AddChildState(TEXT("Recover"));

			// 지금 거리에서 쓸 수 있는 공격이 있으면 쓴다
			AddMonsterCheckCondition(Attack, EPCMonsterCheck::AttackAvailable);
			Attack.AddTask<FPCStateTreeTask_UseAttack>();
			GoToWhenCompleted(Attack, Recover);

			// 없고 멀면 다가간다
			AddMonsterCheckCondition(Approach, EPCMonsterCheck::WithinApproachRange, true);
			Approach.AddTask<FPCStateTreeTask_MoveToTarget>();
			GoToWhenCompleted(Approach, Root);

			// 거리는 맞는데 공격이 전부 재사용 대기 중이면 잠깐 기다린다
			AddRestTask(Hold, HoldInterval);
			GoToWhenCompleted(Hold, Root);

			// 공격 뒤 숨 고르기. 대상 주위를 돌며 거리를 잰다. Attack에서 전환으로만 들어온다.
			auto& Circle = Recover.AddTask<FPCStateTreeTask_CircleTarget>();
			Circle.GetInstanceData().Duration = MinRecoverTime;
			Circle.GetInstanceData().bAddAttackRest = true;
			GoToWhenCompleted(Recover, Root);
		}

		UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
		Idle.AddTask<FPCStateTreeTask_AcquireTarget>();
		GoToWhenCompleted(Idle, Root);
	}
}

UStateTree* UPCStateTreeBuilderLibrary::BuildMeleeMonsterStateTree(UObject* Outer, FName Name, EObjectFlags Flags)
{
	// 엔진의 팩토리로 만들어야 에디터 데이터, 스키마, 루트 상태가 에디터에서 만든 것과 같은 모양이 된다
	UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
	Factory->SetSchemaClass(UStateTreeAIComponentSchema::StaticClass());

	UStateTree* Tree = Cast<UStateTree>(Factory->FactoryCreateNew(UStateTree::StaticClass(), Outer, Name, Flags, nullptr, GWarn));
	UStateTreeEditorData* EditorData = Tree ? Cast<UStateTreeEditorData>(Tree->EditorData) : nullptr;
	if (!EditorData || EditorData->SubTrees.Num() == 0)
	{
		UE_LOG(LogPC, Error, TEXT("StateTree '%s'를 만들지 못했습니다"), *Name.ToString());
		return nullptr;
	}

	PopulateMeleeMonsterTree(*EditorData);

	FStateTreeCompilerLog Log;
	if (!UStateTreeEditingSubsystem::CompileStateTree(Tree, Log))
	{
		Log.DumpToLog(LogPC);
		UE_LOG(LogPC, Error, TEXT("StateTree '%s' 컴파일에 실패했습니다"), *Name.ToString());
		return nullptr;
	}

	return Tree;
}

UStateTree* UPCStateTreeBuilderLibrary::CreateMeleeMonsterStateTree(const FString& PackagePath, const FString& AssetName)
{
	const FString PackageName = PackagePath / AssetName;
	if (FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogPC, Warning, TEXT("'%s'가 이미 있어서 만들지 않습니다"), *PackageName);
		return nullptr;
	}

	UPackage* Package = CreatePackage(*PackageName);
	UStateTree* Tree = BuildMeleeMonsterStateTree(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
	if (Tree)
	{
		FAssetRegistryModule::AssetCreated(Tree);
		Package->MarkPackageDirty();
	}
	return Tree;
}
