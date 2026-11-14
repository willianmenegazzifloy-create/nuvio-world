// Colecoes da conta no shape do web -> ColFolder, e a chave de fileira por id.
#include "../src/colecoes.h"
#include "nvtmp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
static const char *baseAddonAtual = "https://resolvido";
const char *addons_base_por_id(const char *id) {
  if (id && !strcmp(id, "x")) return "https://addon/abc";
  return id && !strcmp(id, "org.x") ? baseAddonAtual : "";
}
int addons_base_por_id_copiar(const char *id, char *dst, unsigned tam) {
  const char *base = addons_base_por_id(id);
  if (!base || !base[0] || !dst || !tam) return 0;
  snprintf(dst, tam, "%s", base);
  return 1;
}
int addons_catalogo_base_ativo(const char *base) {
  return base && ((!strcmp(base, "https://addon/abc")) ||
                  !strcmp(base, "https://x") ||
                  (baseAddonAtual[0] && !strcmp(base, baseAddonAtual)));
}
int main(void) {
  const char *web =
    "[{\"collections_json\":{\"collections\":[{\"id\":\"c1\",\"title\":\"Streaming\",\"backdropImageUrl\":\"https://img/bg.jpg\",\"viewMode\":\"FOLLOW_LAYOUT\",\"showAllTab\":true,"
    "\"folders\":[{\"id\":\"f1\",\"title\":\"Netflix\",\"coverImageUrl\":\"https://img/nf.jpg\",\"titleLogoUrl\":\"https://img/nf.png\",\"hideTitle\":true,"
    "\"sources\":[{\"provider\":\"addon\",\"addonId\":\"x\",\"addonBaseUrl\":\"https://addon/abc/manifest.json\",\"type\":\"movie\",\"catalogId\":\"nf_movies\",\"title\":\"Movies\",\"genre\":\"None\"},"
    "{\"addonBaseUrl\":\"https://addon/abc\",\"type\":\"series\",\"catalogId\":\"nf_series\",\"catalogName\":\"Series\"}]},"
    "{\"id\":\"f2\",\"title\":\"Vazia\",\"sources\":[]}]}]}}]";
  assert(col_definir_json(web) == 1);
  const ColFolder *f = col_folder(0);
  assert(f && !strcmp(f->group, "Streaming") && !strcmp(f->groupId, "c1") && !strcmp(f->id, "f1"));
  assert(!strcmp(f->cover, "https://img/nf.jpg") && f->hideTitle == 1);
  assert(f->viewMode == COL_VIEW_FOLLOW_LAYOUT && f->showAllTab == 1);
  // BANNER na ordem do web: heroBackdropUrl > coverImageUrl > backdropImageUrl
  // da colecao. A pasta nao trouxe heroBackdropUrl, entao o hero fica vazio e o
  // backdrop da colecao e so o ultimo recurso.
  assert(!f->hero[0] && !strcmp(f->groupBackdrop, "https://img/bg.jpg"));
  assert(!strcmp(col_banner(f), "https://img/nf.jpg") && !strcmp(col_capa(f), "https://img/nf.jpg"));
  assert(f->nSources == 2);
  assert(!strcmp(f->sources[0].base, "https://addon/abc") && !strcmp(f->sources[0].catId, "nf_movies") && !f->sources[0].genre[0]);
  assert(!strcmp(f->sources[1].title, "Series") && !strcmp(f->sources[1].type, "series"));
  {
    int pastas = 0, fontes = 0, ativas = 0;
    col_estatisticas(&pastas, &fontes, &ativas);
    assert(pastas == 1 && fontes == 2 && ativas == 2);
  }

  // Mesmo quando o JSON original inclui fontes diretas TMDB/Trakt, o World
  // conserva somente os catalogos de addons instalados e ativos.
  const char *fontesComProjecao =
    "{\"collections\":[{\"id\":\"mix\",\"title\":\"Mix\",\"viewMode\":\"TABBED_GRID\",\"showAllTab\":false,\"folders\":[{\"id\":\"f\",\"title\":\"Todas\","
    "\"sources\":["
      "{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"title\":\"Filmes Populares\"},"
      "{\"provider\":\"trakt\",\"traktListId\":123,\"title\":\"Lista Trakt\"},"
      "{\"provider\":\"addon\",\"addonId\":\"org.x\",\"type\":\"movie\",\"catalogId\":\"popular\"}],"
    "\"catalogSources\":[{\"addonId\":\"org.x\",\"type\":\"movie\",\"catalogId\":\"popular\"}]}]}]}";
  assert(col_definir_json(fontesComProjecao) == 1);
  f = col_folder(0);
  assert(f->viewMode == COL_VIEW_TABBED_GRID && f->showAllTab == 0);
  assert(f->nSources == 1);
  assert(!strcmp(f->sources[0].addonId, "org.x"));
  puts("ok  sources ignora TMDB/Trakt e conserva catalogo de addon");

  // Quando a lista completa tem apenas provedores, a projecao de catalogos
  // ainda pode fornecer as fontes validas para o World.
  assert(col_definir_json("{\"collections\":[{\"id\":\"projection\",\"title\":\"Projection\",\"folders\":[{\"id\":\"f\",\"title\":\"F\",\"sources\":[{\"provider\":\"tmdb\",\"title\":\"Popular\"}],\"catalogSources\":[{\"addonId\":\"org.x\",\"type\":\"movie\",\"catalogId\":\"popular\"}]}]}]}") == 1);
  assert(col_folder(0)->nSources == 1 &&
         !strcmp(col_folder(0)->sources[0].addonId, "org.x"));
  puts("ok  catalogSources prevalece quando sources so tem provedores externos");

  // Importacao antiga: quando sources esta ausente/vazio, usa catalogSources.
  assert(col_definir_json("{\"collections\":[{\"id\":\"old\",\"title\":\"Old\",\"folders\":[{\"id\":\"f\",\"title\":\"F\",\"sources\":[],\"catalogSources\":[{\"addonId\":\"org.x\",\"type\":\"movie\",\"catalogId\":\"old\"}]}]}]}") == 1);
  assert(col_folder(0)->nSources == 1 && !strcmp(col_folder(0)->sources[0].catId, "old"));
  puts("ok  catalogSources usado como compatibilidade quando sources esta vazio");

  // Addon reconfigurado: o ID antigo sozinho nao identifica a instancia atual,
  // mas um addonBaseUrl salvo ainda pode resolver com seguranca pela URL exata.
  assert(col_definir_json("{\"collections\":[{\"id\":\"changed\",\"title\":\"Changed\",\"folders\":[{\"id\":\"f\",\"title\":\"F\",\"sources\":[{\"provider\":\"addon\",\"addonId\":\"old.manifest.id\",\"addonBaseUrl\":\"https://resolvido/manifest.json\",\"type\":\"movie\",\"catalogId\":\"popular\"}]}]}]}") == 1);
  {
    char base[600];
    assert(col_fonte_base_ativa(&col_folder(0)->sources[0], base, sizeof base));
    assert(!strcmp(base, "https://resolvido"));
  }
  // Formato atual do site: so addonId/type/catalogId. ID desconhecido deve
  // continuar recebido, mas nunca ser tratado como fonte ativa nem virar fileira.
  assert(col_definir_json("{\"collections\":[{\"id\":\"unresolved\",\"title\":\"Unresolved\",\"folders\":[{\"id\":\"f\",\"title\":\"F\",\"catalogSources\":[{\"addonId\":\"old.manifest.id\",\"type\":\"movie\",\"catalogId\":\"popular\"}]}]}]}") == 1);
  {
    int pastas = 0, fontes = 0, ativas = 0;
    char base[600];
    col_estatisticas(&pastas, &fontes, &ativas);
    assert(pastas == 1 && fontes == 1 && ativas == 0);
    assert(!col_fonte_base_ativa(&col_folder(0)->sources[0], base, sizeof base));
  }
  puts("ok  IDs de manifesto antigos: URL exata recupera; ID opaco permanece recebido e inativo");

  // Cadeia de arte da pasta, na ordem do web, ate o ultimo campo.
  assert(col_definir_json("{\"collections\":[{\"id\":\"cc\",\"title\":\"C\",\"backdropImageUrl\":\"https://img/grupo.jpg\",\"folders\":["
    "{\"id\":\"a\",\"title\":\"So grupo\",\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
    "{\"id\":\"b\",\"title\":\"Com hero\",\"heroBackdropUrl\":\"https://img/h.jpg\",\"coverImageUrl\":\"https://img/c.jpg\","
      "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
    "{\"id\":\"n\",\"title\":\"Nada\",\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]},"
    "{\"id\":\"cd\",\"title\":\"D\",\"folders\":[{\"id\":\"z\",\"title\":\"Zero\","
      "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}") == 4);
  assert(!strcmp(col_capa(col_folder(0)), "https://img/grupo.jpg") && !strcmp(col_banner(col_folder(0)), "https://img/grupo.jpg"));
  assert(!strcmp(col_banner(col_folder(1)), "https://img/h.jpg") && !strcmp(col_capa(col_folder(1)), "https://img/c.jpg"));
  assert(!col_banner(col_folder(3))[0] && !col_capa(col_folder(3))[0]);   // sem campo nenhum: nada inventado
  puts("ok  banner: heroBackdropUrl > coverImageUrl > backdropImageUrl da colecao; capa: cover > backdrop; vazio fica vazio");
  assert(col_definir_json(web) == 1);
  puts("ok  shape do web: linha da RPC, manifest.json cortado, fontes somente de addons, genre None vazio");
  {
    ColDiagnostico d;
    col_snapshot_diagnostico(&d);
    assert(d.colecoes == 1 && d.pastas == 2 && d.fontes == 2);
    assert(d.fontesValidas == 2 && d.pastasValidas == 1 && d.pastasSemFonte == 1);
  }

  // Algumas exportacoes antigas usam snake_case e name em vez de title.
  // O parser aceita os aliases sem mudar a regra: catalogo precisa de
  // identidade, tipo e catalogId; provedores externos continuam de fora.
  assert(col_definir_json("{\\\"collections\\\":[{\\\"id\\\":\\\"alias\\\",\\\"name\\\":\\\"Alias group\\\",\\\"folders\\\":[{\\\"id\\\":\\\"fa\\\",\\\"name\\\":\\\"Alias folder\\\",\\\"catalog_sources\\\":[{\\\"addon_id\\\":\\\"org.x\\\",\\\"catalog_type\\\":\\\"movie\\\",\\\"catalog_id\\\":\\\"alias-popular\\\"}]}]}]}") == 1);
  {
    const ColFolder *a = col_folder(0);
    ColDiagnostico d;
    assert(a && !strcmp(a->group, "Alias group") && !strcmp(a->title, "Alias folder"));
    assert(a->nSources == 1 && !strcmp(a->sources[0].addonId, "org.x"));
    assert(!strcmp(a->sources[0].type, "movie") && !strcmp(a->sources[0].catId, "alias-popular"));
    col_snapshot_diagnostico(&d);
    assert(d.colecoes == 1 && d.pastas == 1 && d.fontes == 1 && d.fontesValidas == 1);
  }
  assert(col_definir_json("{\\\"collections\\\":[{\\\"id\\\":\\\"incomplete\\\",\\\"title\\\":\\\"Incomplete\\\",\\\"folders\\\":[{\\\"id\\\":\\\"f\\\",\\\"title\\\":\\\"Missing catalog ID\\\",\\\"catalogSources\\\":[{\\\"addonId\\\":\\\"org.x\\\",\\\"type\\\":\\\"movie\\\"}]}]}]}") == 0);
  {
    ColDiagnostico d;
    col_snapshot_diagnostico(&d);
    assert(d.colecoes == 1 && d.pastas == 1 && d.fontes == 1);
    assert(d.fontesIncompletas == 1 && d.fontesSemCatalogId == 1);
    assert(d.pastasValidas == 0 && d.pastasSemFonte == 1);
  }

  char chave[192];
  col_chave_grupo("Streaming", chave, sizeof chave); assert(!strcmp(chave, "collection_c1"));
  col_chave_grupo("Outro", chave, sizeof chave);     assert(!strcmp(chave, "collection_Outro"));
  puts("ok  chave por id da colecao, nome quando nao ha id");

  // Collections compostas apenas por fontes TMDB/Trakt nao existem no World.
  const char *descobrir =
    "{\"collections\":[{\"id\":\"d\",\"title\":\"Descobrir\",\"folders\":["
    "{\"id\":\"nf\",\"title\":\"Novos Filmes\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"mediaType\":\"MOVIE\"}]},"
    "{\"id\":\"ns\",\"title\":\"Novas Séries\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"mediaType\":\"TV\"}]},"
    "{\"id\":\"alta\",\"title\":\"Filmes em Alta\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"mediaType\":\"MOVIE\"}]},"
    "{\"id\":\"rede\",\"title\":\"Rede de TV\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"NETWORK\",\"mediaType\":\"MOVIE\",\"tmdbId\":123}]}]}]}";
  assert(col_definir_json(descobrir) == 0 && col_n() == 0);
  puts("ok  pastas sem addon ativo sao descartadas, inclusive TMDB/Trakt");

  // A capacidade real usada pela collection do usuario: 249 pastas numa
  // fileira e ate 52 fontes dentro de uma pasta (o codigo antigo parava em 32).
  {
    char grande[100000]; size_t usado = 0; int idx[256];
    usado += (size_t)snprintf(grande + usado, sizeof grande - usado,
      "{\"collections\":[{\"id\":\"longa\",\"title\":\"Colecoes de Filmes\",\"folders\":[");
    for (int p = 0; p < 249; p++) {
      int fontes = p == 0 ? 52 : 1;
      usado += (size_t)snprintf(grande + usado, sizeof grande - usado,
        "%s{\"id\":\"p%d\",\"title\":\"Pasta %d\",\"sources\":[",
        p ? "," : "", p, p);
      for (int k = 0; k < fontes; k++)
        usado += (size_t)snprintf(grande + usado, sizeof grande - usado,
          "%s{\"provider\":\"addon\",\"addonBaseUrl\":\"https://addon/abc\",\"type\":\"movie\",\"catalogId\":\"p%d_%d\"}",
          k ? "," : "", p, k);
      usado += (size_t)snprintf(grande + usado, sizeof grande - usado, "]}");
      assert(usado < sizeof grande);
    }
    usado += (size_t)snprintf(grande + usado, sizeof grande - usado, "]}]}");
    assert(usado < sizeof grande);
    assert(col_definir_json(grande) == 249 && col_n() == 249);
    assert(col_grupo_chave("collection_longa", idx, 256) == 249);
    assert(col_folder(idx[0])->nSources == 52);
    assert(!strcmp(col_folder(idx[0])->sources[51].catId, "p0_51"));
    puts("ok  collection com 249 pastas e uma pasta com 52 fontes preservada por inteiro");
  }

  // Fontes diretas TMDB/Trakt nao sao fontes validas de Collection no World.
  assert(col_definir_json("{\"collections\":[{\"id\":\"providers\",\"title\":\"Providers\",\"folders\":[{\"id\":\"p\",\"title\":\"Providers\",\"sources\":[{\"provider\":\"tmdb\",\"title\":\"Popular\",\"tmdbSourceType\":\"DISCOVER\"},{\"provider\":\"trakt\",\"title\":\"Trending\",\"traktListId\":123}]}]}]}") == 0);
  assert(col_n() == 0);
  puts("ok  fontes TMDB/Trakt rejeitadas nas Collections");

  // string escapada, como parseRemoteCollectionsPayload aceita
  const char *esc = "{\"collections_json\":\"{\\\"collections\\\":[{\\\"id\\\":\\\"c9\\\",\\\"title\\\":\\\"T\\\",\\\"folders\\\":[{\\\"id\\\":\\\"g\\\",\\\"title\\\":\\\"G\\\",\\\"sources\\\":[{\\\"addonBaseUrl\\\":\\\"https://a\\\",\\\"type\\\":\\\"movie\\\",\\\"catalogId\\\":\\\"k\\\"}]}]}]}\"}";
  assert(col_definir_json(esc) == 1 && !strcmp(col_folder(0)->groupId, "c9"));
  puts("ok  collections_json como string escapada");

  // Only an explicit complete empty snapshot deletes old account collections.
  unsigned rev = col_revisao();
  const char *invalidas[] = {
    "{\"collections\":[] garbage}",
    "{\"collections\":[],\"broken\":}",
    "{\"collections\":[],\"broken\":[}",
    "{\"collections\":[],}",
    "{\"collections\":[],\"broken\":01}",
    "{\"collections\":[],\"broken\":1e+}",
    "{\"collections\":[],\"broken\":\"\\q\"}",
    "{\"collections\":[],\"broken\":\"\\u123\"}",
    "{\"collections\":[],\"broken\":truex}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"broken\\\":}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[]}\\u0000garbage\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\n\\\"}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\u000A\\\"}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\u000D\\\"}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\u0009\\\"}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\u0001\\\"}\"}",
    "{\"collections_json\":\"{\\\"collections\\\":[],\\\"x\\\":\\\"\\uD83D\\uDE\"}"
  };
  for (size_t i = 0; i < sizeof invalidas / sizeof invalidas[0]; i++) {
    assert(!col_resposta_valida(invalidas[i]));
    assert(col_definir_json(invalidas[i]) == 0 && col_n() == 1 && col_revisao() == rev);
  }
  assert(col_resposta_valida(" \n{\"collections\":[],\"other\":[-1.25e+3,true,false,null,\"\\u00e1\\n\",{}]}\t"));
  assert(col_resposta_valida("{\"collections_json\":\"\\n{\\\"collections\\\":[],\\\"x\\\":\\\"\\u00e1\\uD83D\\uDE00\\\"}\\t\"}"));
  assert(col_resposta_valida("{\"collections_json\":\"\\u000A\\u000D{\\\"collections\\\":[]}\\u0009\"}"));
  puts("ok  malformed envelope and encoded snapshot retain last-good collections; complete JSON values accepted");
  assert(col_definir_json("[]") == 0 && col_n() == 1 && col_revisao() == rev);
  assert(col_definir_json("{\"collections_json\":null}") == 0 && col_n() == 1);
  assert(col_definir_json("{\"collections\":[{\"id\":\"cut\"") == 0 && col_n() == 1);
  assert(col_definir_json("{\"collections\":[]}") == 1 && col_n() == 1 && col_revisao() == rev);
  puts("ok  empty snapshot preserves the last local collection; malformed snapshots retain it");
  col_esquecer_perfil();
  rev = col_revisao();
  assert(col_n() == 0);
  assert(col_definir_json("{\"collections\":[]}") == 0 && col_n() == 0 && col_revisao() == rev);
  puts("ok  empty snapshot with no local collections remains empty");
  // fonte so com addonId (como a conta manda): entra, e a base resolve no acesso
  assert(col_definir_json("{\"collections\":[{\"id\":\"c\",\"title\":\"T\",\"folders\":[{\"id\":\"g\",\"title\":\"G\",\"sources\":[{\"provider\":\"addon\",\"addonId\":\"org.x\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}") == 1);
  assert(!strcmp(col_folder(0)->sources[0].base, "https://resolvido"));
  puts("ok  addonId sem URL resolve pela sonda");
  // addonId e a identidade canonica: uma URL antiga salva na fonte nao
  // pode impedir o casamento com a base atual nem ressuscitar um addon removido.
  baseAddonAtual = "https://novo-addon";
  assert(col_definir_json("{\"collections\":[{\"id\":\"stable\",\"title\":\"Stable\",\"folders\":[{\"id\":\"f\",\"title\":\"F\",\"sources\":[{\"provider\":\"addon\",\"addonId\":\"org.x\",\"addonBaseUrl\":\"https://addon-antigo/manifest.json\",\"type\":\"movie\",\"catalogId\":\"popular\"}]}]}]}") == 1);
  assert(!strcmp(col_folder(0)->sources[0].base, "https://novo-addon"));
  assert(col_por_catalogo("https://novo-addon", "movie", "popular") != NULL);
  assert(col_por_catalogo("https://addon-antigo", "movie", "popular") == NULL);
  baseAddonAtual = "";
  assert(col_por_catalogo("https://addon-antigo", "movie", "popular") == NULL);
  assert(col_por_catalogo("https://novo-addon", "movie", "popular") == NULL);
  baseAddonAtual = "https://resolvido";
  puts("ok  identidade addonId vence URL antiga; addon ausente nao e ressuscitado");
  // a RPC real: collections_json e o array direto
  assert(col_definir_json("[{\"profile_id\":1,\"collections_json\":[{\"id\":\"r\",\"title\":\"R\",\"folders\":[{\"id\":\"g\",\"title\":\"G\",\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}],\"updated_at\":\"x\"}]") == 1);
  assert(!strcmp(col_folder(0)->groupId, "r"));
  puts("ok  linha da RPC com o array direto");
  // GIF DE FOCO (#29). O pacote nao guarda URL nenhuma — ele converte o GIF em
  // 001.jpg..090.jpg na importacao —, entao a URL so chega por aqui. Sem esta
  // leitura as colecoes da conta nunca animam, que e o issue inteiro.
  assert(col_definir_json("{\"collections\":[{\"id\":\"c\",\"title\":\"T\",\"folders\":["
    "{\"id\":\"g1\",\"title\":\"Com GIF\",\"focusGifUrl\":\"https://cdn/a.gif\","
      "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
    "{\"id\":\"g2\",\"title\":\"Sem campo\","
      "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
    "{\"id\":\"g3\",\"title\":\"Desligado\",\"focusGifUrl\":\"https://cdn/c.gif\",\"focusGifEnabled\":false,"
      "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}") == 3);
  assert(!strcmp(col_folder(0)->focusGif, "https://cdn/a.gif"));
  assert(!col_folder(1)->focusGif[0]);   // campo ausente e o caso comum
  assert(!col_folder(2)->focusGif[0]);   // focusGifEnabled:false desliga
  puts("ok  focusGifUrl entra, campo ausente fica vazio, focusGifEnabled:false desliga");
  // pacote + conta com o mesmo id: fica a arte local, grupo/titulo da conta
  { char dir[NVT_MAX]; nvt_set(dir, sizeof dir, "/nuvio-col-XXXXXX"); char caminho[300]; FILE *f;
    assert(mkdtemp(dir));
    snprintf(caminho, sizeof caminho, "%s/collections.json", dir); f = fopen(caminho, "w");
     fputs("{\"groups\":[{\"id\":\"c1\",\"title\":\"Streaming\",\"folders\":[{\"id\":\"f1\",\"title\":\"Netflix\",\"cover\":\"collections/f1/cover.jpg\",\"hero\":\"collections/f1/hero.jpg\",\"frames\":12,\"sources\":[{\"title\":\"Movies\",\"addonId\":\"org.x\",\"type\":\"movie\",\"catId\":\"nf_movies\"}]}]}]}", f); fclose(f);
    assert(col_carregar(dir) == 1 && col_folder(0)->local && col_folder(0)->frames == 12);
    assert(col_folder(0)->nSources == 1 && !strcmp(col_folder(0)->sources[0].addonId, "org.x") &&
           !strcmp(col_folder(0)->sources[0].base, "https://resolvido"));
    puts("ok  pacote preserva fonte identificada por addonId sem URL salva");
    assert(col_definir_json("{\"collections\":[{\"id\":\"c1\",\"title\":\"Streaming Renomeado\",\"folders\":[{\"id\":\"f1\",\"title\":\"Netflix\",\"coverImageUrl\":\"https://cdn/nf.webp\",\"sources\":[{\"addonId\":\"missing.x\",\"type\":\"movie\",\"catalogId\":\"nf_movies\"}]}]},{\"id\":\"c2\",\"title\":\"Nova\",\"folders\":[{\"id\":\"f9\",\"title\":\"Nova pasta\",\"coverImageUrl\":\"https://cdn/n.webp\",\"sources\":[{\"addonId\":\"missing.x\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}") == 2);
    assert(strstr(col_folder(0)->hero, "/collections/f1/hero.jpg") && col_folder(0)->frames == 12 && col_folder(0)->local);
    assert(!strcmp(col_folder(0)->group, "Streaming Renomeado") &&
           !strcmp(col_folder(0)->sources[0].addonId, "missing.x") && !col_folder(0)->sources[0].base[0]);
    assert(!strcmp(col_folder(1)->cover, "https://cdn/n.webp") && !col_folder(1)->local);
    puts("ok  pasta do pacote guarda arte e quadros; a conta da grupo, titulo e pastas novas"); }
  // "ARTE DAS PASTAS DA CONTA": desligado e o pacote (acima); ligado, capa,
  // fundo e logo da conta vencem campo a campo, e desligar de novo devolve a
  // arte curada na hora (o casamento e refeito com a ultima resposta).
  { char dir[NVT_MAX]; nvt_set(dir, sizeof dir, "/nuvio-colarte-XXXXXX"); char caminho[300]; FILE *f;
    const char *conta =
      "{\"collections\":[{\"id\":\"c1\",\"title\":\"S\",\"folders\":["
      "{\"id\":\"f1\",\"title\":\"Netflix\",\"coverImageUrl\":\"https://cdn/nf.webp\","
        "\"heroBackdropUrl\":\"https://cdn/nf-hero.jpg\",\"titleLogoUrl\":\"https://cdn/nf-logo.png\","
        "\"focusGifUrl\":\"https://cdn/nf.gif\","
        "\"sources\":[{\"addonId\":\"x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
      "{\"id\":\"f2\",\"title\":\"Prime\",\"titleLogoUrl\":\"https://cdn/pv-logo.png\","
        "\"sources\":[{\"addonId\":\"x\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}";
    assert(mkdtemp(dir));
    snprintf(caminho, sizeof caminho, "%s/collections.json", dir); f = fopen(caminho, "w");
    fputs("{\"groups\":[{\"id\":\"c1\",\"title\":\"S\",\"folders\":["
          "{\"id\":\"f1\",\"title\":\"Netflix\",\"cover\":\"c/f1.jpg\",\"hero\":\"c/f1h.jpg\",\"logo\":\"c/f1l.png\",\"frames\":12,"
            "\"sources\":[{\"title\":\"M\",\"base\":\"https://a\",\"type\":\"movie\",\"catId\":\"k\"}]},"
          "{\"id\":\"f2\",\"title\":\"Prime\",\"cover\":\"c/f2.jpg\",\"hero\":\"c/f2h.jpg\",\"logo\":\"c/f2l.png\","
            "\"sources\":[{\"title\":\"M\",\"base\":\"https://a\",\"type\":\"movie\",\"catId\":\"k\"}]}]}]}", f);
    fclose(f);
    col_arte_conta(0);
    assert(col_carregar(dir) == 2);
    assert(col_definir_json(conta) == 2);
    assert(strstr(col_folder(0)->cover, "c/f1.jpg") && col_folder(0)->frames == 12);
    assert(strstr(col_folder(1)->logo, "c/f2l.png"));
    col_arte_conta(1);   // refaz na hora, sem nova resposta da conta
    assert(col_n() == 2 && col_folder(0)->local && col_folder(1)->local);
    assert(!strcmp(col_folder(0)->cover, "https://cdn/nf.webp") && col_folder(0)->frames == 0 &&
           !strcmp(col_folder(0)->focusGif, "https://cdn/nf.gif"));
    assert(!strcmp(col_folder(0)->hero, "https://cdn/nf-hero.jpg") && !col_folder(0)->editorial);
    assert(!strcmp(col_folder(0)->logo, "https://cdn/nf-logo.png"));
    // So o logo na conta: capa e fundo continuam os do pacote.
    assert(strstr(col_folder(1)->cover, "c/f2.jpg") && strstr(col_folder(1)->hero, "c/f2h.jpg") &&
           !strcmp(col_folder(1)->logo, "https://cdn/pv-logo.png"));
    assert(col_definir_json(conta) == 2);   // ciclo de sync com o ajuste ligado: estavel
    assert(!strcmp(col_folder(0)->cover, "https://cdn/nf.webp"));
    col_arte_conta(0);   // desligar devolve a arte curada
    assert(strstr(col_folder(0)->cover, "c/f1.jpg") && col_folder(0)->frames == 12 &&
           strstr(col_folder(0)->hero, "c/f1h.jpg") && strstr(col_folder(1)->logo, "c/f2l.png"));
    remove(caminho); rmdir(dir);
    puts("ok  arte das pastas da conta: desligado o pacote, ligado a conta campo a campo, volta na hora"); }
  // O GIF DA CONTA CONTRA A ARTE DO PACOTE. A regra e "a versao local fica
  // inteira", e o GIF nao a contradiz: o pacote nao TEM focusGif para perder.
  //   frames > 0  — ja anima pela sequencia curada; a URL da conta e ignorada.
  //   frames == 0 — nao ha animacao nenhuma; a URL entra, senao a pasta ficaria
  //                 parada tendo GIF disponivel.
  { char dir[NVT_MAX]; nvt_set(dir, sizeof dir, "/nuvio-colgif-XXXXXX"); char caminho[300]; FILE *f;
    const char *conta =
      "{\"collections\":[{\"id\":\"c1\",\"title\":\"G\",\"folders\":["
      "{\"id\":\"comq\",\"title\":\"Com quadros\",\"focusGifUrl\":\"https://cdn/q.gif\","
        "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
      "{\"id\":\"semq\",\"title\":\"Sem quadros\",\"focusGifUrl\":\"https://cdn/s.gif\","
        "\"sources\":[{\"addonId\":\"a\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}";
    assert(mkdtemp(dir));
    snprintf(caminho, sizeof caminho, "%s/collections.json", dir); f = fopen(caminho, "w");
    fputs("{\"groups\":[{\"id\":\"c1\",\"title\":\"G\",\"folders\":["
          "{\"id\":\"comq\",\"title\":\"Com quadros\",\"frames\":12,"
            "\"sources\":[{\"title\":\"M\",\"base\":\"https://a\",\"type\":\"movie\",\"catId\":\"k\"}]},"
          "{\"id\":\"semq\",\"title\":\"Sem quadros\",\"frames\":0,"
            "\"sources\":[{\"title\":\"M\",\"base\":\"https://a\",\"type\":\"movie\",\"catId\":\"k\"}]}]}]}", f);
    fclose(f);
    assert(col_carregar(dir) == 2);
    assert(!col_folder(0)->focusGif[0] && !col_folder(1)->focusGif[0]);
    assert(col_definir_json(conta) == 2);
    assert(col_folder(0)->frames == 12 && !col_folder(0)->focusGif[0]);
    assert(col_folder(1)->frames == 0 && !strcmp(col_folder(1)->focusGif, "https://cdn/s.gif"));
    assert(col_folder(0)->local && col_folder(1)->local);
    remove(caminho); rmdir(dir);
    puts("ok  o GIF da conta so entra na pasta local que nao tem sequencia de quadros"); }
  // OS QUATRO NIVEIS DE col_diagnostico. O que este teste guarda nao e a
  // funcao e sim a CAPACIDADE DE SEPARAR causas: os quatro casos abaixo
  // produzem hoje o mesmo sintoma na tela (fileira solta na home, #18) e o
  // nivel e a unica coisa que diz qual deles aconteceu. Se dois deles voltarem
  // a devolver o mesmo numero, o diagnostico volta a ser inutil e o relator
  // volta a mandar log que nao decide nada.
  { char g[64];
    assert(col_definir_json("{\"collections\":[{\"id\":\"c\",\"title\":\"Streaming\","
           "\"folders\":[{\"id\":\"g\",\"title\":\"G\",\"sources\":["
           "{\"addonBaseUrl\":\"https://x\",\"type\":\"movie\",\"catalogId\":\"top\"}]}]}]}") == 1);
    assert(col_diagnostico("https://outro", "movie", "top", g, sizeof g) == 0);
    assert(col_diagnostico("https://x", "series", "top", g, sizeof g) == 1);
    assert(col_diagnostico("https://x", "movie", "imdbRating", g, sizeof g) == 2);
    assert(col_diagnostico("https://x", "movie", "top", g, sizeof g) == 3);
    // O grupo sai junto: sem ele o nivel 3 diz "casou" e nao diz ONDE, e o
    // nivel 3 e exatamente o caso em que a pessoa precisa ir desocultar algo.
    assert(!strcmp(g, "Streaming"));
    // Base vazia nao casa com nada. Sem esta guarda uma fileira sem base
    // casaria com toda fonte ainda nao resolvida — o falso positivo que
    // col_por_catalogo ja evita, repetido aqui porque sao duas varreduras.
    assert(col_diagnostico("", "movie", "top", g, sizeof g) == 0);
    puts("ok  col_diagnostico separa os quatro motivos de nao engolir"); }

  // FORMA DA PASTA (tileShape do web): POSTER, LANDSCAPE/WIDE, e o resto —
  // inclusive ausente — quadrado. A fileira usa a da maioria.
  { const char *f = "\"sources\":[{\"addonBaseUrl\":\"https://a/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]";
    char js[2000];
    snprintf(js, sizeof js,
      "{\"collections\":[{\"id\":\"cf\",\"title\":\"Formas\",\"folders\":["
      "{\"id\":\"p\",\"title\":\"P\",\"tileShape\":\"POSTER\",%s},"
      "{\"id\":\"w\",\"title\":\"W\",\"tileShape\":\"wide\",%s},"
      "{\"id\":\"l\",\"title\":\"L\",\"posterShape\":\"LANDSCAPE\",%s},"
      "{\"id\":\"s\",\"title\":\"S\",%s}]}]}", f, f, f, f);
    assert(col_definir_json(js) == 4);
    assert(col_folder(0)->forma == COL_FORMA_POSTER);
    assert(col_folder(1)->forma == COL_FORMA_PAISAGEM);
    assert(col_folder(2)->forma == COL_FORMA_PAISAGEM);
    assert(col_folder(3)->forma == COL_FORMA_QUADRADO);
    assert(col_grupo_forma("Formas") == COL_FORMA_PAISAGEM);
    assert(col_grupo_forma("nao existe") == COL_FORMA_PAISAGEM);
    assert(col_forma_texto("square") == COL_FORMA_QUADRADO);
    puts("ok  tileShape da pasta vira a forma do cartao, na regra do web"); }
  // Empty account snapshots preserve the last local state, as in Nuvio original.
  // A profile switch explicitly forgets the previous account; then empty stays empty.
  { char dir[NVT_MAX], arq[700]; nvt_set(dir, sizeof dir, "/nuvio-col-perfil-XXXXXX");
    FILE *f;
    int i, achou = 0;
    assert(mkdtemp(dir));
    snprintf(arq, sizeof arq, "%s/collections.json", dir);
    f = fopen(arq, "w"); assert(f);
    fputs("{\"groups\":[{\"title\":\"G\",\"id\":\"g1\",\"folders\":[{\"id\":\"f1\",\"title\":\"Netflix\","
          "\"cover\":\"c.jpg\",\"sources\":[{\"base\":\"https://a\",\"type\":\"movie\",\"catId\":\"m\"}]}]}]}", f);
    fclose(f);
    assert(col_carregar(dir) == 1);
    assert(col_definir_json("[{\"collections_json\":[]}]") == 1 && col_n() == 1);
    assert(col_tem_conta());
    assert(!strcmp(col_folder(0)->id, "f1"));
    col_esquecer_perfil();
    assert(col_n() == 0);
    assert(col_definir_json("[{\"collections_json\":[]}]") == 0 && col_n() == 0);
    assert(col_definir_json("[{\"collections_json\":{\"collections\":[]}}]") == 0 && col_n() == 0);
    assert(col_definir_json(web) == 1);
    for (i = 0; i < col_n(); i++)
      if (!strcmp(col_folder(i)->id, "f1")) {
        achou = 1;
        assert(col_folder(i)->local == 1 && strstr(col_folder(i)->cover, "/c.jpg"));
      }
    assert(achou);
    unlink(arq); rmdir(dir);
    puts("ok  troca de perfil: colecoes do anterior saem, arte do pacote volta"); }

  { ColSource s; char n[128];
    memset(&s, 0, sizeof s);
    snprintf(s.catId, sizeof s.catId, "streaming_netflix_movies");
    snprintf(s.title, sizeof s.title, "streaming_netflix_movies");   // conta sem titulo: colecoes.c copia o catId
    col_nome_fonte(&s, "", n, sizeof n); assert(!n[0]);
    col_nome_fonte(&s, NULL, n, sizeof n); assert(!n[0]);
    col_nome_fonte(&s, "streaming_netflix_movies", n, sizeof n); assert(!n[0]);
    col_nome_fonte(&s, "outro_id_cru", n, sizeof n); assert(!n[0]);
    col_nome_fonte(&s, "Netflix", n, sizeof n); assert(!strcmp(n, "Netflix"));
    snprintf(s.title, sizeof s.title, "Originais");
    col_nome_fonte(&s, "Netflix", n, sizeof n); assert(!strcmp(n, "Originais"));
    snprintf(s.title, sizeof s.title, "mdblist.13914"); snprintf(s.catId, sizeof s.catId, "mdblist.13914");
    col_nome_fonte(&s, "Top 2024", n, sizeof n); assert(!strcmp(n, "Top 2024"));
    puts("ok  nome da aba: titulo da conta, senao manifesto, nunca id cru"); }

  puts("colecoes: tudo ok");
  return 0;
}
