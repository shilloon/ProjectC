#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PCStateTreeBuilderLibrary.generated.h"

class UStateTree;

/**
 * 행동 트리(StateTree) 에셋을 코드로 만든다.
 * 에셋 생성 스크립트가 처음 한 번 호출해서 기본 트리를 만들고, 그 뒤로는 에디터에서 자유롭게 고친다.
 */
UCLASS()
class PCCOMBATEDITOR_API UPCStateTreeBuilderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 근접 몬스터용 기본 행동 트리를 에셋으로 만든다.
	 * 뼈대: 죽음 / 경직 / 전투(공격 → 접근 → 대기 → 숨 고르기) / 대상 찾기.
	 * 어떤 공격을 언제 쓰는지는 트리가 아니라 몬스터 정의의 공격 목록이 정한다.
	 * @return 만든 에셋. 같은 이름의 에셋이 이미 있거나 실패하면 nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "PC|Editor")
	static UStateTree* CreateMeleeMonsterStateTree(const FString& PackagePath, const FString& AssetName);

	/** 같은 트리를 에셋 등록 없이 Outer 아래에 만든다 (자동화 테스트용) */
	static UStateTree* BuildMeleeMonsterStateTree(UObject* Outer, FName Name, EObjectFlags Flags);
};
