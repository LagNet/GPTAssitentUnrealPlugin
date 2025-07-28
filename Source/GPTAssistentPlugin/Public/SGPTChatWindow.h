#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UGPTChatController;
class SEditableTextBox;
class SScrollBox;
class STextBlock;
class SButton;
class SThrobber;
class SImage;

/**
 * Widget principal do chat GPT (Slate).
 * Exibe a janela de diálogo com cabeçalho, área de mensagens, campo de entrada e botão de envio.
 */
class SGPTChatWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGPTChatWindow) {}
    SLATE_ARGUMENT(UGPTChatController*, ChatController)
SLATE_END_ARGS()

void Construct(const FArguments& InArgs);
    virtual ~SGPTChatWindow() override;

    // Envia a mensagem digitada
    FReply OnSendMessage();

    // Adiciona uma nova mensagem à interface
    void AppendMessage(const FString& Message, bool bIsUser, FLinearColor OptionalColor = FLinearColor::Transparent);

    // Desativa inputs e exibe loading
    void WaitResponse();

    // Reativa inputs após resposta
    void ResponseReceived();

private:
    TSharedPtr<SEditableTextBox> InputTextBox;
    TSharedPtr<SScrollBox> ChatScrollBox;
    TSharedPtr<STextBlock> WaitingText;
    TSharedPtr<SButton> ActionButton;
    TSharedPtr<SThrobber> LoadingIndicator;
    TSharedPtr<STextBlock> HeaderTitle;
    UPROPERTY()
    UGPTChatController* Controller;
    FDelegateHandle MessageReceivedHandle;
    EActiveTimerReturnType HandleTickCheckReferences(double InCurrentTime, float InDeltaTime);
    UFUNCTION()
    FReply ResetWindow();


};
