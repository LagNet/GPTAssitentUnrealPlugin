#include "UGPTChatController.h"
#include "UChatBridge.h"

void UGPTChatController::Initialize(UChatBridge* InBridge)
{
	ChatBridge = InBridge;

	if (ChatBridge)
	{
		ResponseHandle = ChatBridge->OnGPTResponse.AddLambda([this](const FString& Response)
		{
			OnMessageReceived.Broadcast(Response);
		});
	}
}

void UGPTChatController::SendMessage(const FString& Message)
{
	if (ChatBridge)
	{
		ChatBridge->SendToGPT(Message);
	}
}

void UGPTChatController::BeginDestroy()
{
	if (ChatBridge && ResponseHandle.IsValid())
	{
		ChatBridge->OnGPTResponse.Remove(ResponseHandle);
	}

	Super::BeginDestroy();
}
