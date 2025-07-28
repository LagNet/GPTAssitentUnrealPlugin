#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UGPTChatController.generated.h"

class UChatBridge;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChatMessage, const FString&);

UCLASS()
class GPTASSISTENTPLUGIN_API UGPTChatController : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UChatBridge* InBridge);

    void SendMessage(const FString& Message);

    FOnChatMessage OnMessageReceived;

    UPROPERTY()
    UChatBridge* ChatBridge;
    
private:
    FDelegateHandle ResponseHandle;

    virtual void BeginDestroy() override;
};
