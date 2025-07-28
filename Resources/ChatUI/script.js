let aguardandoResposta = false;
let efeitoDigitando = null;

// Enviar pergunta
function enviarPergunta() {
    if (aguardandoResposta) return;

    const input = document.getElementById("inputPergunta");
    const pergunta = input.value.trim();
    if (pergunta.length === 0) return;

    adicionarMensagem("🧑 Você", pergunta);
    input.value = "";

    mostrarDigitando();
    aguardandoResposta = true;
    desativarInput();

    if (window.uebridge) {
        uebridge.EnviarParaGPT(pergunta);
    }
}

// Mostrar resposta da IA (com efeito de digitação)
function exibirRespostaGPT(resposta) {
    esconderDigitando();
    aguardandoResposta = false;
    ativarInput();
    digitarTexto("🤖 GPT", resposta);
}

// Adicionar mensagem direto
function adicionarMensagem(remetente, texto) {
    const div = document.getElementById("chat-history");
    const msg = document.createElement("div");
    msg.innerHTML = `<strong>${remetente}:</strong> ${texto}`;
    msg.style.marginBottom = "10px";
    div.appendChild(msg);
    div.scrollTop = div.scrollHeight;
}

// Efeito de digitação da IA
function digitarTexto(remetente, texto) {
    const div = document.getElementById("chat-history");
    const msg = document.createElement("div");
    msg.innerHTML = `<strong>${remetente}:</strong> `;
    div.appendChild(msg);

    let i = 0;
    efeitoDigitando = setInterval(() => {
        if (i < texto.length) {
            msg.innerHTML += texto.charAt(i);
            div.scrollTop = div.scrollHeight;
            i++;
        } else {
            clearInterval(efeitoDigitando);
        }
    }, 20);
}

// Indicador de "GPT está digitando"
function mostrarDigitando() {
    const div = document.getElementById("chat-history");
    const loader = document.createElement("div");
    loader.id = "gpt-typing";
    loader.innerHTML = `<strong>🤖 GPT:</strong> <span class="loader">...</span>`;
    div.appendChild(loader);
    div.scrollTop = div.scrollHeight;
}

function esconderDigitando() {
    const loader = document.getElementById("gpt-typing");
    if (loader) loader.remove();
}

function desativarInput() {
    document.getElementById("inputPergunta").disabled = true;
    document.querySelector("button").disabled = true;
}

function ativarInput() {
    document.getElementById("inputPergunta").disabled = false;
    document.querySelector("button").disabled = false;
}
