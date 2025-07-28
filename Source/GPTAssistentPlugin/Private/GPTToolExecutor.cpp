#include "GPTToolExecutor.h"

#include "BlueprintEditorModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "UObject/ObjectMacros.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Exporters/Exporter.h"
#include "Exporters/ExportTextContainer.h"
#include "UnrealExporter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Containers/StringConv.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableSet.h"
#include "K2Node_VariableGet.h"
#include "K2Node.h"
#include "K2Node_InputKey.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "K2Node_MakeArray.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompilerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditor.h" 
#include "EdGraphUtilities.h"



bool UGPTToolExecutor::ExecuteTool(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments, FString& OutResult)
{
    if (ToolName == "gpt_readfile")
    {
        return HandleReadFile(Arguments, OutResult);
    }
    else if (ToolName == "gpt_listfiles")
    {
        return HandleListFiles(Arguments, OutResult);
    }
    else if (ToolName == "gpt_extractbp")
    {
        return ExportBlueprintToT3D(Arguments, OutResult);
    }
    else if (ToolName == "gpt_createblueprint")
    {
        return HandleCreateBlueprint(Arguments, OutResult);
    }


    OutResult = FString::Printf(TEXT("Ferramenta desconhecida: %s"), *ToolName);
    return false;
}

bool UGPTToolExecutor::HandleReadFile(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult)
{
    // Checa o chunk requisitado (padrão = 0)
    int32 Chunk = 0;
    Arguments->TryGetNumberField(TEXT("chunk"), Chunk);
    
    FString VirtualPath;
    if (!Arguments->TryGetStringField("path", VirtualPath))
    {
        OutResult = TEXT("❌ Argumento 'path' ausente.");
        return false;
    }
    
    FString FilePath;
    if (VirtualPath.StartsWith(TEXT("/Plugins")))
    {
        FilePath = VirtualPath.Replace(TEXT("/Plugins"), *FPaths::ProjectPluginsDir());
    }
    else if (VirtualPath.StartsWith(TEXT("/Source")))
    {
        FilePath = VirtualPath.Replace(TEXT("/Source"), *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")));
    }
    else if (VirtualPath.StartsWith(TEXT("/Game")))
    {
        FilePath = VirtualPath.Replace(TEXT("/Game"), *FPaths::ProjectContentDir());
    }
    else if (VirtualPath.StartsWith(TEXT("/Saved")))
    {
        FilePath = VirtualPath.Replace(TEXT("/Saved"), *FPaths::ProjectSavedDir());
    }
    else
    {
        OutResult = TEXT("❌ Caminho virtual inválido.");
        return false;
    }

    FilePath = FPaths::ConvertRelativePathToFull(FilePath);

    if (!FPaths::FileExists(FilePath))
    {
        OutResult = TEXT("❌ Arquivo não encontrado: ") + FilePath;
        return false;
    }

    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        OutResult = TEXT("❌ Falha ao ler o arquivo: ") + FilePath;
        return false;
    }
    FString Key = FilePath; // Você pode melhorar com um hash ou adicionar modelo/language

    OutResult = ServeChunkedOutput(Key, FileContent, Chunk);
    return true;
}

bool UGPTToolExecutor::HandleListFiles(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult)
{
    // Checa o chunk requisitado (padrão = 0)
    int32 Chunk = 0;
    Arguments->TryGetNumberField(TEXT("chunk"), Chunk);

    FString Directory;
    if (!Arguments->TryGetStringField("directory", Directory))
    {
        Directory = TEXT("");
    }
    else
    {
        Directory = Arguments->GetStringField("directory");
    }

    FString Extension = Arguments->GetStringField("extension");

    FString NameFilter;
    Arguments->TryGetStringField("name_filter", NameFilter);

    bool bRecursive = false;
    Arguments->TryGetBoolField("recursive", bRecursive);

    bool bIncludeFolders = false;
    Arguments->TryGetBoolField("include_folders", bIncludeFolders);

    bool bAutoFallback = false;
    Arguments->TryGetBoolField("auto_fallback", bAutoFallback);

    const FString CacheKey = Directory + Extension + NameFilter + (bRecursive ? TEXT("_r") : TEXT("_n"));
    const double CurrentTime = FPlatformTime::Seconds();

    // Se resultados já estão cacheados, usa direto
    if (!CachedFileResults.Contains(CacheKey) || (CurrentTime - CachedFileResults[CacheKey].LastUpdateTime) >= 10.0)
    {
        FString BasePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FString CleanedDirectory = Directory.Replace(TEXT("\\"), TEXT("/"));
        FString TargetFolder = BasePath / CleanedDirectory;
        FPaths::CollapseRelativeDirectories(TargetFolder);

        if (!FPaths::DirectoryExists(TargetFolder))
        {
            if (!bAutoFallback)
            {
                OutResult = TEXT("Diretório não encontrado: ") + TargetFolder;
                return false;
            }

            TArray<FString> AlternativeDirs = {
                TEXT("/Game"), TEXT("/Plugins"), TEXT("/Source"), TEXT("/Saved")
            };

            for (const FString& AltDir : AlternativeDirs)
            {
                TSharedPtr<FJsonObject> AltArgs = MakeShareable(new FJsonObject(*Arguments));
                AltArgs->SetStringField("directory", AltDir);
                AltArgs->SetBoolField("auto_fallback", false);

                FString AltResult;
                if (HandleListFiles(AltArgs, AltResult))
                {
                    OutResult = AltResult;
                    return true;
                }
            }

            OutResult = TEXT("Diretório não encontrado nem via fallback: ") + Directory;
            return false;
        }

        TArray<FString> FilePaths;
        IFileManager& FileManager = IFileManager::Get();

        FString SearchPattern = bRecursive ? TEXT("**/*") : TEXT("*");
        SearchPattern += Extension;

        FileManager.FindFilesRecursive(FilePaths, *TargetFolder, *SearchPattern, true, !bIncludeFolders);

        if (!NameFilter.IsEmpty())
        {
            FilePaths = FilePaths.FilterByPredicate([&](const FString& Path)
            {
                return Path.Contains(NameFilter, ESearchCase::IgnoreCase);
            });
        }

        for (FString& Path : FilePaths)
        {
            FPaths::MakePathRelativeTo(Path, *BasePath);
            Path = TEXT("/") + Path.Replace(TEXT("\\"), TEXT("/"));
        }

        CachedFileResults.FindOrAdd(CacheKey) = { FilePaths, CurrentTime };
    }

    // Junta o resultado em uma string longa para aplicar chunk
    const TArray<FString>& FinalPaths = CachedFileResults[CacheKey].ResultPaths;
    FString FullText = FString::Join(FinalPaths, TEXT("\n"));

    // Gera resultado fracionado
    OutResult = ServeChunkedOutput(CacheKey, FullText, Chunk);

    UE_LOG(LogTemp, Warning, TEXT("📁 [ListFiles] Chunk %d | Diretório: %s | Ext: %s | Filtro: %s | Recursivo: %s | Resultados: %d"),
        Chunk, *Directory, *Extension, *NameFilter, bRecursive ? TEXT("Sim") : TEXT("Não"), FinalPaths.Num());

    return true;
}

bool UGPTToolExecutor::HandleExtractUAsset(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult)
{
    FString RawPath;
    if (!Arguments->TryGetStringField(TEXT("path"), RawPath))
    {
        OutResult = TEXT("Erro: Caminho do asset não fornecido.");
        return false;
    }

    // Checa o chunk requisitado (padrão = 0)
    int32 Chunk = 0;
    Arguments->TryGetNumberField(TEXT("chunk"), Chunk);

    FString VirtualPath = ConvertPhysicalPathToVirtual(RawPath);
    if (VirtualPath.IsEmpty())
    {
        OutResult = TEXT("Erro ao converter caminho.");
        return false;
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *VirtualPath);
    if (!Blueprint)
    {
        OutResult = TEXT("Erro ao carregar Blueprint.");
        return false;
    }

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> GraphsArray;

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        TSharedRef<FJsonObject> GraphJson = MakeShared<FJsonObject>();
        GraphJson->SetStringField(TEXT("GraphName"), Graph->GetName());

        TArray<TSharedPtr<FJsonValue>> NodesArray;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            TSharedRef<FJsonObject> NodeJson = MakeShared<FJsonObject>();
            NodeJson->SetStringField(TEXT("NodeClass"), Node->GetClass()->GetName());
            NodeJson->SetStringField(TEXT("NodeTitle"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            NodeJson->SetStringField(TEXT("Tooltip"), Node->GetTooltipText().ToString());
            NodeJson->SetStringField(TEXT("NodeName"), Node->GetName());

            TArray<TSharedPtr<FJsonValue>> PinsArray;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                TSharedRef<FJsonObject> PinJson = MakeShared<FJsonObject>();
                PinJson->SetStringField(TEXT("PinName"), Pin->PinName.ToString());
                PinJson->SetStringField(TEXT("Direction"), (Pin->Direction == EGPD_Input ? "Input" : "Output"));
                PinJson->SetStringField(TEXT("DefaultValue"), Pin->DefaultValue);

                TArray<TSharedPtr<FJsonValue>> LinkedArray;
                for (UEdGraphPin* Linked : Pin->LinkedTo)
                {
                    if (Linked)
                        LinkedArray.Add(MakeShared<FJsonValueString>(Linked->GetName()));
                }
                PinJson->SetArrayField(TEXT("LinkedTo"), LinkedArray);
                PinsArray.Add(MakeShared<FJsonValueObject>(PinJson));
            }

            NodeJson->SetArrayField(TEXT("Pins"), PinsArray);
            NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
        }

        GraphJson->SetArrayField(TEXT("Nodes"), NodesArray);
        GraphsArray.Add(MakeShared<FJsonValueObject>(GraphJson));
    }

    Root->SetArrayField(TEXT("Graphs"), GraphsArray);

    TArray<TSharedPtr<FJsonValue>> VarsArray;
    for (FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        TSharedRef<FJsonObject> VarJson = MakeShared<FJsonObject>();
        VarJson->SetStringField(TEXT("VarName"), Var.VarName.ToString());
        VarJson->SetStringField(TEXT("VarType"), Var.VarType.PinCategory.ToString());
        VarJson->SetBoolField(TEXT("bEditable"), Var.PropertyFlags & CPF_Edit);
        VarJson->SetStringField(TEXT("Category"), Var.Category.ToString());
        VarsArray.Add(MakeShared<FJsonValueObject>(VarJson));
    }

    Root->SetArrayField(TEXT("Variables"), VarsArray);

    FString FullJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&FullJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutResult = TEXT("Erro ao serializar Blueprint para JSON.");
        return false;
    }

    // Gera cache de chunk e retorna a parte solicitada
    FString UniqueKey = RawPath; // Pode adicionar extra (ex: assistantId) se quiser segmentar por sessão
    OutResult = ServeChunkedOutput(UniqueKey, FullJson, Chunk);

    return true;
}

FString UGPTToolExecutor::ConvertPhysicalPathToVirtual(const FString& PhysicalPath)
{
    FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    FString Normalized = FPaths::ConvertRelativePathToFull(PhysicalPath).Replace(TEXT("\\"), TEXT("/"));

  
    FString VirtualSubPath;
    if (Normalized.StartsWith(ProjectContentDir))
    {
        VirtualSubPath = Normalized.RightChop(ProjectContentDir.Len());
    }
    else
    {
       
        FString Dummy;
        if (Normalized.Split(TEXT("/Content/"), &Dummy, &VirtualSubPath))
        {
          
        }
        else
        {
           
            return TEXT("");
        }
    }

   
    if (VirtualSubPath.EndsWith(TEXT(".uasset")))
    {
        VirtualSubPath = VirtualSubPath.LeftChop(7);
    }

  
    VirtualSubPath = VirtualSubPath.Replace(TEXT("//"), TEXT("/")).TrimStartAndEnd();

    
    VirtualSubPath = VirtualSubPath.Replace(TEXT(" "), TEXT("_"));

   
    FString BaseName = FPaths::GetBaseFilename(VirtualSubPath);
    FString VirtualPath = FString::Printf(TEXT("/Game/%s.%s"), *VirtualSubPath, *BaseName);

    return VirtualPath;
}

bool UGPTToolExecutor::ExportBlueprintToT3D(const TSharedPtr<FJsonObject>& AssetPath, FString& OutResult)
{
    FString RawPath;
    if (!AssetPath->TryGetStringField(TEXT("path"), RawPath))
    {
        OutResult = TEXT("Erro: Caminho do asset não fornecido.");
        return false;
    }

    // Checa o chunk requisitado (padrão = 0)
    int32 Chunk = 0;
    AssetPath->TryGetNumberField(TEXT("chunk"), Chunk);

    FString VirtualPath = ConvertPhysicalPathToVirtual(RawPath);
    if (VirtualPath.IsEmpty())
    {
        OutResult = TEXT("Erro ao converter caminho para formato virtual.");
        return false;
    }

    UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *VirtualPath);
    if (!Asset)
    {
        OutResult = TEXT("Erro ao carregar o asset.");
        return false;
    }

    UExporter* Exporter = UExporter::FindExporter(Asset, TEXT("t3d"));
    if (!Exporter)
    {
        OutResult = TEXT("Exporter T3D não encontrado para este tipo de asset.");
        return false;
    }

    Exporter->SetFlags(RF_Transient);
    FStringOutputDevice OutputDevice;
    FExportObjectInnerContext Context;
    Exporter->ExportToOutputDevice(&Context, Asset, nullptr, OutputDevice, TEXT("t3d"), 0, 0, false, nullptr);

    FString ExportedText = OutputDevice;

    // Fragmenta a saída e retorna o chunk correspondente
    OutResult = ServeChunkedOutput(RawPath, ExportedText, Chunk);

    return true;
}

bool UGPTToolExecutor::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult)
{
    UE_LOG(LogTemp, Warning, TEXT("[GPT] 📦 Criando Blueprint dinamicamente..."));

    
    const TSharedPtr<FJsonObject>* BlueprintDef;
    if (!Arguments->TryGetObjectField(TEXT("blueprint"), BlueprintDef) || !BlueprintDef->IsValid())
    {
        OutResult = TEXT("❌ Campo 'blueprint' ausente ou malformado.");
        return false;
    }

    FString BlueprintName = (*BlueprintDef)->GetStringField(TEXT("BlueprintName"));
    FString BlueprintType = (*BlueprintDef)->GetStringField(TEXT("BlueprintType")); 

   
    FString PackagePath = FString::Printf(TEXT("/Game/Generated/%s"), *BlueprintName);
    UObject* Outer = CreatePackage(*PackagePath);
    UClass* ParentClass = AActor::StaticClass(); 

    UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(ParentClass, Outer, FName(*BlueprintName), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName("GPTFactory"));
    if (!NewBP)
    {
        OutResult = TEXT("❌ Falha ao criar Blueprint.");
        return false;
    }

    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(NewBP);
    if (!EventGraph)
    {
        OutResult = TEXT("❌ Blueprint criada mas sem EventGraph.");
        return false;
    }

    
    TMap<FString, TFunction<void(const TSharedPtr<FJsonObject>&, UEdGraph*)>> NodeFactoryMap;

    NodeFactoryMap.Add("K2Node_InputKey", [](const TSharedPtr<FJsonObject>& NodeJson, UEdGraph* Graph)
    {
        FString KeyName = TEXT("E"); 
        if (NodeJson->HasField("Key"))
        {
            KeyName = NodeJson->GetStringField("Key");
        }

        UK2Node_InputKey* InputNode = NewObject<UK2Node_InputKey>(Graph);
        InputNode->InputKey = FKey(*KeyName);
        InputNode->bConsumeInput = true;
        InputNode->bExecuteWhenPaused = false;
        InputNode->bOverrideParentBinding = false;

        InputNode->NodePosX = 100;
        InputNode->NodePosY = 100;

        Graph->AddNode(InputNode);
        InputNode->AllocateDefaultPins();
    });
    
    const TArray<TSharedPtr<FJsonValue>>* EventGraphArray;
    if ((*BlueprintDef)->TryGetArrayField("EventGraph", EventGraphArray))
    {
        for (const TSharedPtr<FJsonValue>& Entry : *EventGraphArray)
        {
            const TSharedPtr<FJsonObject>* EventEntry;
            if (Entry->TryGetObject(EventEntry))
            {
                const TArray<TSharedPtr<FJsonValue>>* Nodes;
                if ((*EventEntry)->TryGetArrayField("Nodes", Nodes))
                {
                    for (const TSharedPtr<FJsonValue>& NodeVal : *Nodes)
                    {
                        const TSharedPtr<FJsonObject>* NodeObj;
                        if (NodeVal->TryGetObject(NodeObj))
                        {
                            FString NodeType = (*NodeObj)->GetStringField("Type");

                            if (NodeFactoryMap.Contains(NodeType))
                            {
                                NodeFactoryMap[NodeType](*NodeObj, EventGraph);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("[GPT] ⚠️ Tipo de nó não suportado: %s"), *NodeType);
                            }
                        }
                    }
                }
            }
        }
    }

 
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(NewBP);
    OutResult = FString::Printf(TEXT("✅ Blueprint '%s' criada com sucesso."), *BlueprintName);

   
    
    NewBP->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewBP);
    
  
    UPackage* Package = NewBP->GetOutermost();
    FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, NewBP, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);
    
    return true;
}

bool UGPTToolExecutor::AnalyzeSelectedBlueprintNodes(FString& OutResult)
{
    TSharedPtr<FBlueprintEditor> BlueprintEditor;
    FBlueprintEditorModule& KismetModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
    const TArray<TSharedRef<IBlueprintEditor>>& EditorRefs = KismetModule.GetBlueprintEditors();

    for (const TSharedRef<IBlueprintEditor>& EditorRef : EditorRefs)
    {
        TSharedPtr<FBlueprintEditor> ConcreteEditor = StaticCastSharedRef<FBlueprintEditor>(EditorRef);
        if (ConcreteEditor.IsValid() && ConcreteEditor->GetFocusedGraph())
        {
            BlueprintEditor = ConcreteEditor;
            break;
        }
    }

    if (!BlueprintEditor.IsValid())
    {
        OutResult = TEXT("❌ Nenhum editor de Blueprint ativo.");
        return false;
    }

    const FGraphPanelSelectionSet SelectedNodes = BlueprintEditor->GetSelectedNodes();
    TArray<UEdGraphNode*> NodeArray;
    for (UObject* Obj : SelectedNodes)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            NodeArray.Add(Node);
        }
    }

    if (NodeArray.Num() == 0)
    {
        OutResult = TEXT("❌ Nenhum nó selecionado.");
        return false;
    }

    TSet<UObject*> NodeSet;
    for (UEdGraphNode* Node : NodeArray)
    {
        NodeSet.Add(Node);
    }

    FEdGraphUtilities::ExportNodesToText(NodeSet, OutResult);
    return true;
}

FString UGPTToolExecutor::ServeChunkedOutput(const FString& UniqueKey, const FString& FullText, int32 RequestedChunk)
{
	const int32 MaxChunkSize = 3000;

	// Se já estiver cacheado, usa
	if (!ChunkCache.Contains(UniqueKey))
	{
		TArray<FString> Chunks;
		FString Remaining = FullText;

		while (!Remaining.IsEmpty())
		{
			int32 Size = FMath::Min(MaxChunkSize, Remaining.Len());
			Chunks.Add(Remaining.Left(Size));
			Remaining.RightChopInline(Size);
		}

		if (Chunks.Num() == 0)
			Chunks.Add(TEXT("[Mensagem vazia]"));

		FChunkedMessage Data;
		Data.Chunks = Chunks;
		Data.LastAccessTime = FPlatformTime::Seconds();
		ChunkCache.Add(UniqueKey, Data);
	}

	const FChunkedMessage& Stored = ChunkCache[UniqueKey];
	if (!Stored.Chunks.IsValidIndex(RequestedChunk))
	{
		return FString::Printf(TEXT("[Erro] Chunk %d/%d inexistente."), RequestedChunk + 1, Stored.Chunks.Num());
	}

	return FString::Printf(TEXT("[Parte %d/%d]\n%s"), RequestedChunk + 1, Stored.Chunks.Num(), *Stored.Chunks[RequestedChunk]);
}



