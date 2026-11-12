// "OCULTAR NAO LANCADOS" (hideUnreleasedContent) para catalogos e colecoes.
// Prefere a data ISO completa; quando a fonte so informa o ano, conserva o
// comportamento seguro anterior: so esconde anos posteriores ao atual.
#ifndef NV_NLANC_H
#define NV_NLANC_H
#include <stddef.h>
#include <string.h>

static inline int nlanc_iso_valida(const char *s) {
  if (!s || strlen(s) < 10 || s[0] < '0' || s[0] > '9' ||
      s[1] < '0' || s[1] > '9' || s[2] < '0' || s[2] > '9' ||
      s[3] < '0' || s[3] > '9' || s[4] != '-' ||
      s[5] < '0' || s[5] > '9' || s[6] < '0' || s[6] > '9' ||
      s[7] != '-' || s[8] < '0' || s[8] > '9' ||
      s[9] < '0' || s[9] > '9') return 0;
  int mes = (s[5]-'0')*10 + s[6]-'0';
  int dia = (s[8]-'0')*10 + s[9]-'0';
  static const int dias[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
  int ano = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + s[3]-'0';
  int maxDia;
  if (mes < 1 || mes > 12) return 0;
  maxDia = dias[mes-1] + (mes == 2 && (ano % 400 == 0 || (ano % 4 == 0 && ano % 100 != 0)));
  return dia >= 1 && dia <= maxDia;
}

// 1 quando a data completa e posterior a hoje. Datas ISO YYYY-MM-DD podem
// ser comparadas como texto porque seus campos estao em ordem decrescente.
static inline int nlanc_data_futura(const char *data, const char *hoje) {
  char a[11], b[11];
  if (!nlanc_iso_valida(data) || !nlanc_iso_valida(hoje)) return 0;
  for (int i = 0; i < 10; i++) { a[i] = data[i]; b[i] = hoje[i]; }
  a[10] = b[10] = 0;
  return a[0] && b[0] && strcmp(a, b) > 0;
}

// Fallback para fontes que so enviam o ano (por exemplo, listas do Trakt).
static inline int nlanc_meta_tem_ano(const char *meta) {
  if (!meta) return 0;
  for (int i = 0; i < 4; i++)
    if (meta[i] < '0' || meta[i] > '9') return 0;
  return !(meta[4] >= '0' && meta[4] <= '9');
}
static inline int nlanc_meta_futuro(const char *meta, int anoAtual) {
  int ano = 0;
  if (!nlanc_meta_tem_ano(meta)) return 0;
  for (int i = 0; i < 4; i++) ano = ano * 10 + (meta[i] - '0');
  return ano > anoAtual;
}

// TMDB/Trakt sem data ou ano nao permitem provar que o titulo ja estreou.
static inline int nlanc_tem_info(const char *data, const char *meta) {
  return nlanc_iso_valida(data) || nlanc_meta_tem_ano(meta);
}

/* Prefere a data autoritativa da fonte; releaseInfo continua sendo o fallback
   visual quando tambem carrega uma data ISO completa. */
static inline int nlanc_data_escolher(char *dst, size_t tam,
                                      const char *released,
                                      const char *firstAired,
                                      const char *releaseInfo) {
  const char *escolhida = NULL;
  if (!dst || !tam) return 0;
  dst[0] = 0;
  if (nlanc_iso_valida(released)) escolhida = released;
  else if (nlanc_iso_valida(firstAired)) escolhida = firstAired;
  else if (nlanc_iso_valida(releaseInfo)) escolhida = releaseInfo;
  if (!escolhida || tam < 11) return 0;
  snprintf(dst, tam, "%.10s", escolhida);
  return 1;
}

static inline int nlanc_futuro(const char *data, const char *meta,
                               const char *hoje, int anoAtual) {
  if (nlanc_iso_valida(data) && nlanc_iso_valida(hoje))
    return nlanc_data_futura(data, hoje);
  return nlanc_meta_futuro(meta, anoAtual);
}
#endif
