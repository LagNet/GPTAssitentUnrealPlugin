// SGPTChatWindow.cpp
#include "SGPTChatWindow.h"
#include "UGPTChatController.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "GPTAssistentPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UChatBridge.h"  
#include "GPTAssistentPlugin.h" // se for o único include


extern class UChatBridge* GlobalChatBridge;

void SGPTChatWindow::Construct(const FArguments& InArgs)
{
    
    Controller = InArgs._ChatController;

    if (!Controller)
    {
        UE_LOG(LogTemp, Error, TEXT("[SGPTChatWindow] Nenhum controller fornecido."));
        return;
    }
    
    
    MessageReceivedHandle = Controller->OnMessageReceived.AddLambda([this](const FString& Msg)
    {
        AppendMessage(Msg, false, FLinearColor(0.00f, 0.00f, 0.00f)); 
        ResponseReceived();
    });

    ChildSlot
    [
        SNew(SVerticalBox)

       
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SImage)
                .Image(FCoreStyle::Get().GetBrush("Icons.Help"))
            ]

            + SHorizontalBox::Slot().Padding(8,0)
            [
                SAssignNew(HeaderTitle, STextBlock)
                .Text(FText::FromString("BOB GPT Assistente"))
                .Justification(ETextJustify::Center)
                .ColorAndOpacity(FLinearColor::White)
            ]
              
            + SHorizontalBox::Slot()
                    .AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Right)
                    [
                    SNew(SButton)
                    .Text(FText::FromString("+ New Chat"))
                    .OnClicked(this, &SGPTChatWindow::ResetWindow)
                    ]
        ]
        
           
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(WaitingText, STextBlock)
            .Text(FText::FromString("Aguardando resposta do GPT..."))
            .Visibility(EVisibility::Collapsed)
            .Justification(ETextJustify::Center)
            .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
        ]

     
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        .Padding(5)
        [
            SAssignNew(ChatScrollBox, SScrollBox)
        ]

      
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            [
                SAssignNew(InputTextBox, SEditableTextBox)
                .HintText(FText::FromString("Digite sua mensagem..."))
                .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
                {
                    if (CommitType == ETextCommit::OnEnter)
                        OnSendMessage();
                })
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(ActionButton, SButton)
                .Text(FText::FromString("Enviar"))
                .OnClicked(this, &SGPTChatWindow::OnSendMessage)
            ]
        ]
    ];
    
    RegisterActiveTimer(1.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGPTChatWindow::HandleTickCheckReferences));

    if (GlobalChatBridge)
    {
        GlobalChatBridge->ResetThread();
    }
}

FReply SGPTChatWindow::OnSendMessage()
{
    if (!InputTextBox.IsValid()) return FReply::Handled();

    if (!Controller)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SGPTChatWindow] Controller nulo! Tentando criar novo..."));
        Controller = NewObject<UGPTChatController>();
        Controller->AddToRoot();
        MessageReceivedHandle = Controller->OnMessageReceived.AddLambda([this](const FString& Msg)
        {
            AppendMessage(Msg, false, FLinearColor(0.01f, 0.01f, 0.015f));
            ResponseReceived();
        });

        if (GlobalChatBridge == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SGPTChatWindow] ChatBridge global nulo! Criando novo..."));
            GlobalChatBridge = NewObject<UChatBridge>();
            GlobalChatBridge->AddToRoot();
          
                FString PluginConfigPath = FPaths::Combine(
                    IPluginManager::Get().FindPlugin(TEXT("GPTAssistentPlugin"))->GetBaseDir(),
                    TEXT("GPTAssistentPluginConfig.ini")
                );
            
                FString ApiKey;
                GConfig->LoadFile(PluginConfigPath);
                if (!GConfig->GetString(TEXT("GPTSettings"), TEXT("ApiKey"), ApiKey, PluginConfigPath))
                {
                    UE_LOG(LogTemp, Error, TEXT("[GPT Plugin] Não foi possível encontrar a API Key no arquivo .ini (%s)"), *PluginConfigPath);
                }
                else
                {
                    GlobalChatBridge->Initialize(ApiKey);
                    UE_LOG(LogTemp, Log, TEXT("[GPT Plugin] API Key carregada com sucesso."));
                }

        }

        Controller->Initialize(GlobalChatBridge);
    }

    const FString Message = InputTextBox->GetText().ToString();
    if (Message.IsEmpty()) return FReply::Handled();

    AppendMessage(Message, true, FLinearColor(0.02f, 0.02f, 0.02f));
    if (Controller)
    {
        Controller->SendMessage(Message);
        WaitResponse();
        InputTextBox->SetText(FText::GetEmpty());
    }
  

    return FReply::Handled();
}

void SGPTChatWindow::AppendMessage(const FString& Message, bool bIsUser, FLinearColor OptionalColor)
{
    if (!ChatScrollBox.IsValid()) return;

    FLinearColor BackgroundColor;

    if (OptionalColor != FLinearColor::Transparent)
    {
        BackgroundColor = OptionalColor;
    }
    else
    {
        BackgroundColor = bIsUser 
            ? FLinearColor(0.15f, 0.2f, 0.6f)
            : FLinearColor(0.25f, 0.25f, 0.25f);
    }

    ChatScrollBox->AddSlot()
    .Padding(5)
    [
        SNew(SBorder)
        .Padding(10)
        .BorderBackgroundColor(BackgroundColor)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text(FText::FromString(Message))
            .ColorAndOpacity(FLinearColor::White)
        ]
    ];

    ChatScrollBox->ScrollToEnd();
}

void SGPTChatWindow::WaitResponse()
{
    if (ActionButton.IsValid()) ActionButton->SetEnabled(false);
    if (InputTextBox.IsValid()) InputTextBox->SetIsReadOnly(true);
    if (WaitingText.IsValid()) WaitingText->SetVisibility(EVisibility::Visible);
}

void SGPTChatWindow::ResponseReceived()
{
    if (ActionButton.IsValid()) ActionButton->SetEnabled(true);
    if (InputTextBox.IsValid()) InputTextBox->SetIsReadOnly(false);
    if (WaitingText.IsValid()) WaitingText->SetVisibility(EVisibility::Collapsed);
}

SGPTChatWindow::~SGPTChatWindow()
{
    if (Controller && MessageReceivedHandle.IsValid())
    {
        Controller->OnMessageReceived.Remove(MessageReceivedHandle);
    }
}

EActiveTimerReturnType SGPTChatWindow::HandleTickCheckReferences(double InCurrentTime, float InDeltaTime)
{
    bool bControllerRecreated = false;
    bool bBridgeRecreated = false;
    bool bDelegateRestored = false;

    if (!Controller)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SGPTChatWindow Tick] Controller estava nulo. Recriando..."));
        Controller = NewObject<UGPTChatController>();
        Controller->AddToRoot();
        bControllerRecreated = true;
    }

    if (!GlobalChatBridge)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SGPTChatWindow Tick] ChatBridge estava nulo. Recriando..."));
        GlobalChatBridge = NewObject<UChatBridge>();
        GlobalChatBridge->AddToRoot();

        FString PluginConfigPath = FPaths::Combine(
            IPluginManager::Get().FindPlugin(TEXT("GPTAssistentPlugin"))->GetBaseDir(),
            TEXT("GPTAssistentPluginConfig.ini")
        );

        FString ApiKey;
        GConfig->LoadFile(PluginConfigPath);
        if (!GConfig->GetString(TEXT("GPTSettings"), TEXT("ApiKey"), ApiKey, PluginConfigPath))
        {
            UE_LOG(LogTemp, Error, TEXT("[GPT Plugin Tick] Não foi possível encontrar a API Key."));
        }
        else
        {
            GlobalChatBridge->Initialize(ApiKey);
            UE_LOG(LogTemp, Log, TEXT("[GPT Plugin Tick] API Key carregada com sucesso."));
        }

        bBridgeRecreated = true;
    }

    if (bControllerRecreated || bBridgeRecreated)
    {
        Controller->Initialize(GlobalChatBridge);
    }

    if (!MessageReceivedHandle.IsValid() && Controller)
    {
        MessageReceivedHandle = Controller->OnMessageReceived.AddLambda([this](const FString& Msg)
        {
            AppendMessage(Msg, false, FLinearColor(0.03f, 0.03f, 0.03f));
            ResponseReceived();
        });

        bDelegateRestored = true;
    }

    if (bControllerRecreated || bBridgeRecreated || bDelegateRestored)
    {
        if (ActionButton.IsValid()) ActionButton->SetEnabled(true);
        if (InputTextBox.IsValid()) InputTextBox->SetIsReadOnly(false);
        if (WaitingText.IsValid()) WaitingText->SetVisibility(EVisibility::Collapsed);

        UE_LOG(LogTemp, Warning, TEXT("[SGPTChatWindow Tick] Estado restaurado (Controller, Bridge ou Delegate)."));
    }

    return EActiveTimerReturnType::Continue;
}

FReply SGPTChatWindow::ResetWindow()
{
    if (ChatScrollBox.IsValid()) ChatScrollBox->ClearChildren();
    if (InputTextBox.IsValid()) InputTextBox->SetText(FText::GetEmpty());
    if (InputTextBox.IsValid()) InputTextBox->SetIsReadOnly(false);
    if (ActionButton.IsValid()) ActionButton->SetEnabled(true);
    if (WaitingText.IsValid()) WaitingText->SetVisibility(EVisibility::Collapsed);

    if (GlobalChatBridge) GlobalChatBridge->ResetThread();
    
    if (Controller)
    {
        Controller->OnMessageReceived.Remove(MessageReceivedHandle);
        MessageReceivedHandle = Controller->OnMessageReceived.AddLambda([this](const FString& Msg)
        {
            AppendMessage(Msg, false, FLinearColor(0.03f, 0.03f, 0.03f));
            ResponseReceived();
        });

        UE_LOG(LogTemp, Log, TEXT("[SGPTChatWindow] Delegate re-registrado após ResetWindow."));
    }

    UE_LOG(LogTemp, Log, TEXT("[SGPTChatWindow] ResetWindow acionado manualmente."));
    return FReply::Handled();
}

