#include "configtransfer.h"
#include "dados.h"
#include "ajustes.h"
#include "perfis.h"
#include "fileiras.h"
#include "player.h"
#include "js.h"
#include "jsw.h"
#ifdef NV_ANDROID
#include "android.h"
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CT_MAX_JSON (512u * 1024u)
#define CT_MAX_ARQ  (128u * 1024u)
#define CT_MAX_ITENS 6

typedef struct { char nome[64]; char *conteudo; } CtItem;
static int apagarSeExiste(const char *nome);

static void aplicarPerfilAtual(int perfil) {
  ajustes_perfil_restaurar(perfil);
  fil_definir_perfil(perfil);
  fil_recarregar();
}

static int perfilAtual(void) {
  int p = perfis_ativo();
  return p > 0 ? p : 1;
}
static void nomePerfil(char *dst, size_t tam, const char *prefixo, int perfil) {
  snprintf(dst, tam, "%s-p%d.txt", prefixo, perfil);
}
static void escreverArquivo(Jsw *w, const char *nome) {
  char *s = dados_ler(nome);
  jsw_obj_ini(w);
  jsw_cs(w, "name", nome);
  jsw_cs(w, "content", s ? s : "");
  jsw_obj_fim(w);
  free(s);
}

int configtransfer_exportar(char *dst, size_t tam) {
  Jsw w;
  char nAjustes[48], nRegras[48], nFileiras[48];
  const char *json;
  int perfil = perfilAtual(), ok;
  if (!dst || !tam || !dados_dir()[0]) return 0;
  // Consolida o snapshot de perfil antes de empacotar. Credenciais ficam em
  // arquivos separados e nunca entram nesta lista permitida.
  ajustes_perfil_guardar(perfil);
  nomePerfil(nAjustes, sizeof nAjustes, "ajustes", perfil);
  nomePerfil(nRegras, sizeof nRegras, "fonteregra", perfil);
  nomePerfil(nFileiras, sizeof nFileiras, "fileirasui", perfil);
  jsw_iniciar(&w);
  jsw_obj_ini(&w);
  jsw_cs(&w, "format", "nuvio-settings");
  jsw_ci(&w, "version", 1);
  jsw_ci(&w, "profile", perfil);
  jsw_chave(&w, "files"); jsw_arr_ini(&w);
  escreverArquivo(&w, "ajustes.txt");
  escreverArquivo(&w, "player.txt");
  escreverArquivo(&w, "player-externo.txt");
  escreverArquivo(&w, nAjustes);
  escreverArquivo(&w, nRegras);
  escreverArquivo(&w, nFileiras);
  jsw_arr_fim(&w);
  jsw_obj_fim(&w);
  json = jsw_texto_final(&w);
  if (!json || strlen(json) > CT_MAX_JSON) { jsw_livre(&w); return 0; }
  ok = dados_gravar("nuvio-configuracoes.json", json);
  jsw_livre(&w);
  if (!ok || !dados_caminho(dst, (unsigned)tam, "nuvio-configuracoes.json")) return 0;
  return 1;
}

// Aceita somente nomes literais da lista branca. Nunca usa um caminho vindo do
// JSON; nomes por perfil sao convertidos para o ID ativo neste aparelho.
static int nomeDestino(const char *entrada, int perfil, char *dst, size_t tam) {
  static const char *const base[] = { "ajustes.txt", "player.txt", "player-externo.txt" };
  static const char *const pref[] = { "ajustes-p", "fonteregra-p", "fileirasui-p" };
  for (size_t i = 0; i < sizeof base / sizeof *base; i++)
    if (!strcmp(entrada, base[i])) { snprintf(dst, tam, "%s", base[i]); return 1; }
  // Pacotes criados antes da separacao por perfil tambem podem ser importados.
  if (!strcmp(entrada, "fonteregra.txt")) {
    if (perfil > 0) nomePerfil(dst, tam, "fonteregra", perfil);
    else snprintf(dst, tam, "fonteregra.txt");
    return 1;
  }
  if (!strcmp(entrada, "fileirasui.txt")) {
    if (perfil > 0) nomePerfil(dst, tam, "fileirasui", perfil);
    else snprintf(dst, tam, "fileirasui.txt");
    return 1;
  }
  for (size_t i = 0; i < sizeof pref / sizeof *pref; i++) {
    size_t n = strlen(pref[i]);
    const char *p = entrada + n;
    if (strncmp(entrada, pref[i], n)) continue;
    if (!*p) return 0;
    for (; *p && *p >= '0' && *p <= '9'; p++) {}
    if (strcmp(p, ".txt")) return 0;
    nomePerfil(dst, tam, pref[i], perfil);
    return 1;
  }
  return 0;
}

int configtransfer_importar(const char *caminho) {
  FILE *f;
  char *json = NULL;
  long tamanho;
  const char *fim, *it, *arr;
  CtItem itens[CT_MAX_ITENS];
  int n = 0, perfil = perfilAtual(), ok = 0;
  memset(itens, 0, sizeof itens);
  if (!caminho || !*caminho || !(f = fopen(caminho, "rb"))) return 0;
  if (fseek(f, 0, SEEK_END) || (tamanho = ftell(f)) < 2 || tamanho > (long)CT_MAX_JSON || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
  json = (char *)malloc((size_t)tamanho + 1);
  if (!json) { fclose(f); return 0; }
  if (fread(json, 1, (size_t)tamanho, f) != (size_t)tamanho) goto fim;
  json[tamanho] = 0; fclose(f); f = NULL;
  fim = js_fim(json);
  { char formato[32];
    const char *v = strstr(json, "\"version\"");
    const char *fch = strstr(json, "\"files\"");
    if (!fim || !js_texto_raiz(json, "format", formato, sizeof formato) ||
        strcmp(formato, "nuvio-settings") || !v || !fch || v > fch) goto fim;
    v = strchr(v, ':');
    if (!v) goto fim;
    v++;
    while (*v == ' ' || *v == '\t') v++;
    if (*v != '1' || (v[1] != ',' && v[1] != ' ' && v[1] != '\n' && v[1] != '\r' && v[1] != '}')) goto fim;
  }
  arr = js_array(json, fim, "files");
  if (!arr) goto fim;
  for (it = arr; it && *it; ) {
    const char *fe = js_fim(it);
    char nome[64], destino[64];
    size_t len;
    if (!fe || !js_texto(it, fe, "name", nome, sizeof nome) ||
        !nomeDestino(nome, perfil, destino, sizeof destino) || n >= CT_MAX_ITENS) goto fim;
    for (int k = 0; k < n; k++) if (!strcmp(itens[k].nome, destino)) goto fim;
    itens[n].conteudo = (char *)malloc(CT_MAX_ARQ + 1);
    if (!itens[n].conteudo || !js_texto_linhas(it, fe, "content", itens[n].conteudo, CT_MAX_ARQ + 1)) goto fim;
    len = strlen(itens[n].conteudo);
    if (len > CT_MAX_ARQ) goto fim;
    snprintf(itens[n].nome, sizeof itens[n].nome, "%s", destino);
    n++;
    it = js_prox(fe);
  }
  // Um pacote valido precisa conter as tres preferencias gerais e as tres do
  // perfil; nao se aplica silenciosamente um JSON incompleto.
  if (n != CT_MAX_ITENS) goto fim;
  // Todas as entradas foram validadas e estao na lista branca antes da primeira
  // escrita. String vazia significa que o arquivo nao existia na origem.
  for (int k = 0; k < n; k++) {
    int gravou = itens[k].conteudo[0]
      ? dados_gravar(itens[k].nome, itens[k].conteudo)
      : apagarSeExiste(itens[k].nome);
    if (!gravou) goto fim;
  }
  // Recarrega valores e caches em memoria para que uma gravacao posterior
  // nao sobrescreva o pacote importado com o estado antigo da sessao.
  ajustes_dir(dados_dir());
  aplicarPerfilAtual(perfil);
  player_dir(dados_dir());
  player_leg_estilo_carregar();
  printf("[config] preferencias importadas para o perfil %d; reinicie o app para aplicar\n", perfil);
  ok = 1;
fim:
  if (f) fclose(f);
  for (int k = 0; k < CT_MAX_ITENS; k++) free(itens[k].conteudo);
  free(json);
  return ok;
}

static int apagarSeExiste(const char *nome) {
  char *s = dados_ler(nome);
  if (!s) return 1;
  free(s);
  return dados_apagar(nome);
}

static int copiar(const char *origem, const char *destino) {
  char *s = dados_ler(origem);
  int ok;
  if (!s) return apagarSeExiste(destino);
  ok = dados_gravar(destino, s);
  free(s);
  return ok;
}
int configtransfer_copiar_perfil(int origem, int destino) {
  char a[48], b[48];
  if (origem <= 0 || destino <= 0 || origem == destino) return 0;
  nomePerfil(a, sizeof a, "ajustes", origem); nomePerfil(b, sizeof b, "ajustes", destino);
  if (!copiar(a, b)) return 0;
  nomePerfil(a, sizeof a, "fonteregra", origem); nomePerfil(b, sizeof b, "fonteregra", destino);
  if (!copiar(a, b)) return 0;
  nomePerfil(a, sizeof a, "fileirasui", origem); nomePerfil(b, sizeof b, "fileirasui", destino);
  if (!copiar(a, b)) return 0;
  if (destino == perfilAtual()) aplicarPerfilAtual(destino);
  printf("[config] preferencias do perfil %d copiadas para %d; reinicie o app para aplicar\n", origem, destino);
  return 1;
}

#ifdef __EMSCRIPTEN__
EM_JS(void, ct_browser_exportar, (const char *caminho), {
  try {
    var bytes = FS.readFile(UTF8ToString(caminho));
    var blob = new Blob([bytes], {type: "application/json"});
    var url = URL.createObjectURL(blob);
    var a = document.createElement("a");
    a.href = url; a.download = "Nuvio-Configuracoes.json";
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
  } catch (e) { alert("Não foi possível exportar as configurações."); }
});
EM_JS(void, ct_browser_importar, (), {
  var input = document.createElement("input");
  input.type = "file"; input.accept = ".json,application/json";
  document.body.appendChild(input);
  input.onchange = function () {
    if (!input.files || !input.files.length) return;
    var reader = new FileReader();
    reader.onload = function () {
      try {
        FS.writeFile("/nuvio-config-import.json", new Uint8Array(reader.result));
        Module._configtransfer_importar_callback();
      } catch (e) { alert("Não foi possível ler o arquivo de configurações."); }
    };
    reader.onerror = function () { alert("Não foi possível ler o arquivo de configurações."); };
    reader.readAsArrayBuffer(input.files[0]);
  };
  input.click();
});
EM_JS(void, ct_browser_notificar, (int ok, int tipo), {
  var msg = !ok ? (tipo === 3 ? "Não foi possível exportar as configurações." : "Não foi possível aplicar as configurações. Verifique o arquivo e tente novamente.")
    : (tipo === 1 ? "Configurações importadas. Reinicie o Nuvio para aplicar."
       : "Configurações copiadas. Reinicie o Nuvio para aplicar.");
  alert(msg);
});
EMSCRIPTEN_KEEPALIVE void configtransfer_importar_callback(void) {
  int ok = configtransfer_importar("/nuvio-config-import.json");
  remove("/nuvio-config-import.json");
  configtransfer_notificar(ok, 1);
}
#endif

void configtransfer_notificar(int sucesso, int tipo) {
#ifdef NV_ANDROID
  android_config_resultado(sucesso, tipo);
#elif defined(__EMSCRIPTEN__)
  ct_browser_notificar(sucesso, tipo);
#else
  printf("[config] %s (tipo %d)\n", sucesso ? "operacao concluida" : "operacao falhou", tipo);
#endif
}

void configtransfer_pedir_exportacao(void) {
  char caminho[600];
  if (!configtransfer_exportar(caminho, sizeof caminho)) {
    configtransfer_notificar(0, 3);
    return;
  }
#ifdef NV_ANDROID
  android_config_exportar(caminho);
#elif defined(__EMSCRIPTEN__)
  ct_browser_exportar(caminho);
#else
  configtransfer_notificar(0, 3);
#endif
}

void configtransfer_pedir_importacao(void) {
#ifdef NV_ANDROID
  android_config_importar();
#elif defined(__EMSCRIPTEN__)
  ct_browser_importar();
#else
  configtransfer_notificar(0, 1);
#endif
}

void configtransfer_processar_eventos(void) {
#ifdef NV_ANDROID
  char caminho[600];
  if (android_config_evento(caminho, sizeof caminho)) {
    int ok = configtransfer_importar(caminho);
    remove(caminho);
    android_config_resultado(ok, 1);
  }
#endif
}
