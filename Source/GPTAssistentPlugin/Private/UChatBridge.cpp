#include "UChatBridge.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GPTToolExecutor.h"
#include "HttpManager.h"
#include "Containers/Ticker.h"

UChatBridge::UChatBridge()
{
    AssistantId = TEXT("asst_xJRVYOUEid1kLq3WE0Xl7CbM");
    ModelVersion = TEXT("gpt-3.5-turbo");
    DefaultLanguage = TEXT("pt-BR");
    bIsPolling = false;
}

void UChatBridge::Initialize(const FString& InApiKey)
{
    ApiKey = InApiKey;
    RegisterConsoleCommands();
}

void UChatBridge::CreateThread(const FString& OptionalPrompt)
{
    const FString Url = "https://api.openai.com/v1/threads";
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));

    Request->OnProcessRequestComplete().BindLambda([this, OptionalPrompt](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
    {
        if (!bSuccess || !Resp.IsValid() || Resp->GetResponseCode() != 200)
        {
            UE_LOG(LogTemp, Error, TEXT("Falha ao criar thread. Código HTTP: %d"), Resp.IsValid() ? Resp->GetResponseCode() : -1);
            return;
        }

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            ThreadId = JsonObject->GetStringField("id");
            UE_LOG(LogTemp, Log, TEXT("Thread criada: %s"), *ThreadId);

            if (!OptionalPrompt.IsEmpty())
            {
                SendPromptWithAssistant(OptionalPrompt);
            }
        }
    });

    Request->ProcessRequest();
}

void UChatBridge::SendPromptWithAssistant(const FString& Prompt)
{
    if (ApiKey.IsEmpty() || AssistantId.IsEmpty() || ThreadId.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("API Key, Assistant ID ou Thread ID ausentes."));
        return;
    }

    const FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/messages"), *ThreadId);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField("role", "user");
    Body->SetStringField("content", Prompt);

    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(Body, Writer);
    Request->SetContentAsString(Content);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
    {
        if (bSuccess && Resp->GetResponseCode() == 200)
        {
            UE_LOG(LogTemp, Log, TEXT("[GPT] Enviando mensagem!"));
            RunAssistant();
        }
        else
        {
            FString ResponseBody = Resp.IsValid() ? Resp->GetContentAsString() : TEXT("Resposta inválida");
            UE_LOG(LogTemp, Error, TEXT("Erro ao enviar mensagem ao Assistant. Código HTTP: %d"), Resp.IsValid() ? Resp->GetResponseCode() : -1);
            UE_LOG(LogTemp, Error, TEXT("Resposta: %s"), *ResponseBody);
        }
    });

    Request->ProcessRequest();
}

void UChatBridge::RunAssistant()
{
    const FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/runs"), *ThreadId);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField("assistant_id", AssistantId);

    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(Body, Writer);
    Request->SetContentAsString(Content);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
    {
        if (bSuccess && Resp->GetResponseCode() == 200)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                RunId = JsonObject->GetStringField("id");
                UE_LOG(LogTemp, Log, TEXT("[GPT] Run iniciado com ID: %s"), *RunId);
                StartPolling();
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Erro ao iniciar execução do Assistant."));
        }
    });

    Request->ProcessRequest();
}

void UChatBridge::PollForRunCompletion()
{
    const FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/runs/%s"), *ThreadId, *RunId);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
    {
        if (!bSuccess || !Resp.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("Erro ao verificar status do Run."));
            return;
        }

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            const FString Status = JsonObject->GetStringField("status");
            UE_LOG(LogTemp, Log, TEXT("[GPT] Status do run: %s"), *Status);

            if (Status == "completed")
            {
                StopPolling();
                GetFinalResponse();
            }
            else if (Status == "failed")
            {
                StopPolling();
                FString ErrorMessage;
                if (const TSharedPtr<FJsonObject>* ErrObj; JsonObject->TryGetObjectField("last_error", ErrObj))
                {
                    (*ErrObj)->TryGetStringField("message", ErrorMessage);
                }
                UE_LOG(LogTemp, Error, TEXT("[GPT] Run falhou. %s"), *ErrorMessage);
            }
            else if (Status == "requires_action")
            {
                UE_LOG(LogTemp, Warning, TEXT("[GPT] Run requer ação. Processando tool_calls..."));

                const TSharedPtr<FJsonObject>* RequiredActionObject;
                if (JsonObject->TryGetObjectField("required_action", RequiredActionObject))
                {
                    const TSharedPtr<FJsonObject>* SubmitToolOutputs;
                    if ((*RequiredActionObject)->TryGetObjectField("submit_tool_outputs", SubmitToolOutputs))
                    {
                        const TArray<TSharedPtr<FJsonValue>>* ToolCallsArray;
                        if ((*SubmitToolOutputs)->TryGetArrayField("tool_calls", ToolCallsArray))
                        {
                            TArray<FToolCall> ParsedCalls;
                            for (const TSharedPtr<FJsonValue>& ToolCallValue : *ToolCallsArray)
                            {
                                const TSharedPtr<FJsonObject> ToolCallObj = ToolCallValue->AsObject();
                                if (!ToolCallObj.IsValid()) continue;

                                FToolCall ParsedCall;
                                ToolCallObj->TryGetStringField("id", ParsedCall.ToolCallId);
                                ParsedCall.RunId = RunId;

                                const TSharedPtr<FJsonObject>* FunctionObject;
                                if (ToolCallObj->TryGetObjectField("function", FunctionObject))
                                {
                                    (*FunctionObject)->TryGetStringField("name", ParsedCall.ToolName);
                                    FString ArgumentsStr;                                  
                                   if ((*FunctionObject)->TryGetStringField("arguments", ArgumentsStr))
                                   {
                                       TSharedRef<TJsonReader<>> ArgReader = TJsonReaderFactory<>::Create(ArgumentsStr);
                                       TSharedPtr<FJsonObject> ParsedArgs;
                                       if (FJsonSerializer::Deserialize(ArgReader, ParsedArgs) && ParsedArgs.IsValid())
                                       {
                                           ParsedCall.Arguments = ParsedArgs;
                                       }
                                       else
                                       {
                                           UE_LOG(LogTemp, Error, TEXT("[GPT] Falha ao desserializar argumentos da função '%s': %s"), *ParsedCall.ToolName, *ArgumentsStr);
                                           ParsedCall.Arguments = MakeShared<FJsonObject>();
                                       }
                                   }
                                   else
                                   {
                                       UE_LOG(LogTemp, Warning, TEXT("[GPT] Função '%s' não possui campo 'arguments'. Usando objeto vazio."), *ParsedCall.ToolName);
                                       ParsedCall.Arguments = MakeShared<FJsonObject>();
                                   }


                                    ParsedCalls.Add(ParsedCall);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("[GPT] Objeto 'function' inválido ou ausente."));
                                }
                            }

                            HandleToolCallsFromRun(ParsedCalls);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("[GPT] 'submit_tool_outputs' não contém 'tool_calls'."));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("[GPT] 'required_action' não contém 'submit_tool_outputs'."));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[GPT] 'required_action' ausente no JSON."));
                }
            }
        }
    });

    Request->ProcessRequest();
}

void UChatBridge::GetFinalResponse()
{
    const FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/messages"), *ThreadId);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
    {
        if (bSuccess && Resp->GetResponseCode() == 200)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* Messages;
                if (JsonObject->TryGetArrayField("data", Messages))
                {
                    for (const auto& Msg : *Messages)
                    {
                        TSharedPtr<FJsonObject> MsgObj = Msg->AsObject();
                        if (MsgObj->GetStringField("role") == "assistant")
                        {
                            const TArray<TSharedPtr<FJsonValue>>* ContentArray;
                            if (MsgObj->TryGetArrayField("content", ContentArray) && ContentArray->Num() > 0)
                            {
                                TSharedPtr<FJsonObject> ContentObj = (*ContentArray)[0]->AsObject();
                                FString Value = ContentObj->GetObjectField("text")->GetStringField("value");
                                UE_LOG(LogTemp, Log, TEXT("🧠 Resposta do Assistant: %s"), *Value);
                                OnGPTResponse.Broadcast(Value);
                                return;
                            }
                        }
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Nenhuma resposta do assistant encontrada."));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Erro ao obter resposta final. Código HTTP: %d"), Resp.IsValid() ? Resp->GetResponseCode() : -1);
        }
    });

    Request->ProcessRequest();
}

void UChatBridge::RegisterConsoleCommands()
{
    IConsoleManager& ConsoleManager = IConsoleManager::Get();

    SendPromptCommand = ConsoleManager.RegisterConsoleCommand(
        TEXT("gpt.send"),
        TEXT("Envia um prompt para o Assistant."),
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UChatBridge::HandleSendPrompt),
        ECVF_Default
    );
}

void UChatBridge::ClearCommands()
{
    IConsoleManager& ConsoleManager = IConsoleManager::Get();

    if (SendPromptCommand)
    {
        ConsoleManager.UnregisterConsoleObject(SendPromptCommand);
        SendPromptCommand = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Comando 'gpt.send' removido com sucesso."));
    }
}

void UChatBridge::HandleSendPrompt(const TArray<FString>& Args)
{
    FString Prompt = FString::Join(Args, TEXT(" "));
    if (ThreadId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Thread ainda não criada. Criando agora..."));
        CreateThread(Prompt);
        return;
    }

    SendPromptWithAssistant(Prompt);
}

void UChatBridge::StartPolling()
{
    if (bIsPolling) return;
    bIsPolling = true;
    PollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UChatBridge::TickPollRun), 1.5f
    );
}

bool UChatBridge::TickPollRun(float DeltaTime)
{
    PollForRunCompletion();
    return bIsPolling;
}

void UChatBridge::StopPolling()
{
    if (!bIsPolling) return;
    bIsPolling = false;
    FTSTicker::GetCoreTicker().RemoveTicker(PollTickerHandle);
}

void UChatBridge::HandleToolCallsFromRun(const TArray<FToolCall>& ToolCalls)
{
    TArray<TSharedPtr<FJsonValue>> ToolOutputsArray;
    FString CurrentRunId;

    for (const FToolCall& ToolCall : ToolCalls)
    {
        const FString& ToolName = ToolCall.ToolName;
        const TSharedPtr<FJsonObject>& Arguments = ToolCall.Arguments;
        const FString& ToolCallId = ToolCall.ToolCallId;
        const FString& RunIdLocal = ToolCall.RunId;
        CurrentRunId = RunIdLocal; // Salva o run id

        FString ToolResult;
        bool bSuccess = false;

        if (!Arguments.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[GPT] ❌ Argumentos inválidos para a ferramenta '%s'."), *ToolName);
            continue;
        }

        UGPTToolExecutor* Executor = NewObject<UGPTToolExecutor>();
        bSuccess = Executor->ExecuteTool(ToolName, Arguments, ToolResult);


        if (!bSuccess)
        {
            UE_LOG(LogTemp, Error, TEXT("[GPT] ❌ Falha ao executar ferramenta: %s"), *ToolName);
        }

        // Cria JSON do resultado
        TSharedPtr<FJsonObject> OutputObject = MakeShared<FJsonObject>();
        OutputObject->SetStringField(TEXT("tool_call_id"), ToolCallId);
        OutputObject->SetStringField(TEXT("output"), ToolResult);
        ToolOutputsArray.Add(MakeShared<FJsonValueObject>(OutputObject));
    }

    // Envia tudo junto
    if (ToolOutputsArray.Num() > 0 && !CurrentRunId.IsEmpty())
    {
        SubmitToolOutputsArray(CurrentRunId, ToolOutputsArray);
    }
}

void UChatBridge::SendPromptWithCallback(const FString& Message, const FString& InThreadId)
{
    // ⚠️ Atualiza o ThreadId localmente, caso necessário
    ThreadId = InThreadId;

    // Monta o JSON corretamente com role e content
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject);
    Payload->SetStringField(TEXT("role"), TEXT("user"));       // Campo obrigatório
    Payload->SetStringField(TEXT("content"), Message);         // Texto da mensagem

    // Serializa o JSON para string
    FString PayloadString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    // Cria requisição HTTP
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/messages"), *ThreadId));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));  // Obrigatório para API v2

    Request->SetContentAsString(PayloadString);

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Resp.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GPT] ❌ Falha ao enviar mensagem para a thread."));
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("[GPT] ✅ Mensagem enviada com sucesso!"));
        }
    );

    Request->ProcessRequest();
}

void UChatBridge::SubmitToolOutputsToRun(const FString& InRunId, const FString& ToolCallId, const FString& Output)
{
    FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/runs/%s/submit_tool_outputs"), *ThreadId, *InRunId);

    // Monta JSON com tool_outputs
    TSharedPtr<FJsonObject> OutputObject = MakeShareable(new FJsonObject);
    OutputObject->SetStringField(TEXT("tool_call_id"), ToolCallId);
    OutputObject->SetStringField(TEXT("output"), Output);

    TArray<TSharedPtr<FJsonValue>> ToolOutputsArray;
    ToolOutputsArray.Add(MakeShareable(new FJsonValueObject(OutputObject)));

    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetArrayField(TEXT("tool_outputs"), ToolOutputsArray);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    // Cria a requisição HTTP
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetContentAsString(RequestBody);

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (!bSuccess || !Resp.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GPT] ❌ Falha ao enviar output da ferramenta."));
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("[GPT] ✅ Output da ferramenta enviado com sucesso: "));
        }
    );

    Request->ProcessRequest();
}

void UChatBridge::SubmitToolOutputsArray(const FString& InRunId, const TArray<TSharedPtr<FJsonValue>>& ToolOutputsArray)
{
    FString Url = FString::Printf(TEXT("https://api.openai.com/v1/threads/%s/runs/%s/submit_tool_outputs"), *ThreadId, *InRunId);

    const int32 MaxSafeLength = 1000000;
    TArray<TSharedPtr<FJsonValue>> SafeToolOutputs;

    for (const TSharedPtr<FJsonValue>& ToolOutputValue : ToolOutputsArray)
    {
        TSharedPtr<FJsonObject> OutputObject = ToolOutputValue->AsObject();
        if (!OutputObject.IsValid())
        {
            continue;
        }

        FString Content;
        if (OutputObject->TryGetStringField("output", Content) && Content.Len() > MaxSafeLength)
        {
            FString ToolCallId;
            OutputObject->TryGetStringField("tool_call_id", ToolCallId);

            TSharedPtr<FJsonObject> ErrorOutput = MakeShared<FJsonObject>();
            ErrorOutput->SetStringField("tool_call_id", ToolCallId);
            ErrorOutput->SetStringField("output", TEXT("❌ O conteúdo retornado por esta ferramenta excede o tamanho máximo permitido. "
                                                      "Tente dividir o conteúdo em seções menores ou utilizar filtros."));

            SafeToolOutputs.Add(MakeShared<FJsonValueObject>(ErrorOutput));
        }
        else
        {
            SafeToolOutputs.Add(ToolOutputValue);
        }
    }

    TSharedPtr<FJsonObject> RequestJson = MakeShared<FJsonObject>();
    RequestJson->SetArrayField(TEXT("tool_outputs"), SafeToolOutputs);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("OpenAI-Beta"), TEXT("assistants=v2"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetContentAsString(RequestBody);

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (!bSuccess || !Resp.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GPT] ❌ Falha ao enviar outputs das ferramentas."));
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("[GPT] ✅ Outputs das ferramentas enviados com sucesso: "));
        }
    );

    Request->ProcessRequest();
}

void UChatBridge::SendToGPT(const FString& Message)
{
    if (Message.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Mensagem vazia, cancelando envio."));
        return;
    }

    HandleSendPrompt({ Message }); // ainda pode ser public, mas o uso será controlado
}

void UChatBridge::ResetThread()
{
    UE_LOG(LogTemp, Log, TEXT("[GPT] Resetando thread antiga: %s"), *ThreadId);
    ThreadId.Empty();
    RunId.Empty();
}
