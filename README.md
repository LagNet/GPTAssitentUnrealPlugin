
# GPTAssistentPlugin 🧠🔧

Plugin para Unreal Engine 5 que integra o poder da OpenAI diretamente ao editor, permitindo análises, geração e manipulação de Blueprints, arquivos e lógica C++ com um assistente técnico especializado. Ideal para desenvolvedores que desejam acelerar seu fluxo de trabalho com IA.

> 🚀 Desenvolvido por Morisakki — compatível com Unreal Engine 5.3+ (Editor Only)

---

## 📦 Instalação

### 1. Clonar o repositório
```bash
git clone https://github.com/SeuUsuario/GPTAssistentPlugin.git
```

### 2. Mover o plugin para seu projeto
Coloque a pasta `GPTAssistentPlugin` dentro da pasta `Plugins/` do seu projeto Unreal:

```
SeuProjeto/Plugins/GPTAssistentPlugin
```

### 3. Regenerar os arquivos do projeto
Se estiver usando C++, clique com o botão direito no `.uproject` e selecione **Generate Visual Studio project files**.

### 4. Abrir o projeto
Abra seu projeto normalmente na Unreal Engine. O plugin será carregado automaticamente no modo Editor.

---

## 🧠 Requisitos

- Unreal Engine 5.3 ou superior  
- API Key da OpenAI com acesso ao Assistants API (GPT-4o recomendado)  
- Conexão com a internet (para comunicação com a OpenAI)  

---

## ⚙️ Configuração

Crie um arquivo `.ini` em:

```
Plugins/GPTAssistentPlugin/Config/DefaultGPTAssistentPlugin.ini
```

Com o seguinte conteúdo:

```ini
[GPTSettings]
ApiKey=
```

---

## 🧪 Comandos Disponíveis

### Envio e Interação

| Comando                         | Descrição |
|----------------------------------|-----------|
| `gpt.send [mensagem]`           | Envia mensagem diretamente para o assistente |


---

### Leitura e Análise de Arquivos

| Comando                                     | Descrição |
|--------------------------------------------|-----------|
| `gpt.readfile { "path": "..." }`           | Lê qualquer arquivo `.cpp`, `.h`, `.json` ou `.txt` |
| `gpt.listfiles { "ext": ".cpp", "recursive": true }` | Lista arquivos por extensão, nome ou diretório |
| `gpt.extractbp { "path": "..." }`          | Extrai uma Blueprint `.uasset` em formato JSON técnico |

---

### Criação e Geração

| Comando                                         | Descrição |
|------------------------------------------------|-----------|
| `gpt.createblueprint { "blueprint": { ... } }` | Cria uma Blueprint completa com lógica interna baseada em JSON |


---

## 🖥️ Interface Visual

O plugin inclui uma janela visual baseada em Slate:
- Envio e recebimento de mensagens  
- Notificações de status (pensando, processando, erro)  
- Scroll automático  
- Suporte a UTF-8  

Abra pelo menu: **Window > GPT Assistant**

---

## 📁 Biblioteca de Documentos (File Search)

A IA utiliza documentos internos para:
- Verificar instruções de uso  
- Validar nós existentes em Blueprints  
- Aprender com logs e arquivos de crash  
- Evitar loops infinitos ou execuções perigosas  



---

## 💡 Exemplos de Uso

### 🔍 Analisar Blueprint
```bash
gpt.extractbp { "path": "C:/Projeto/Content/BP_Monstro.uasset" }
```

### 🧠 Criar Blueprint por texto
```text
"Crie um ator que gira lentamente em Z e muda de cor quando o jogador se aproxima"
```

A IA responderá com a lógica e solicitará permissão antes de criar a Blueprint real.

---

## 🔐 Segurança

- Todas as ferramentas que alteram o projeto (como mover arquivos ou criar BPs) **requerem confirmação**.  
- O plugin verifica automaticamente estouro de dados e quebra arquivos em blocos se necessário.  
- Sistema de aprovação futura para comandos críticos será adicionado.  

---

## 🛠️ Em desenvolvimento

- 🔧 Sistema de preview antes de aplicar alterações  
- 📊 Geração de levels e posicionamento procedural  
- 🧩 Compatibilidade com assistentes locais (LLM Embedded)  
- 🖼️ Visor de Blueprint visual com nodes gerados  

---

## ❓ Contribuição

Contribuições são bem-vindas!  
Abra um Pull Request ou Issue com sugestões, bugs ou melhorias. ✨

---

## 📜 Licença

MIT — use livremente, com créditos!
