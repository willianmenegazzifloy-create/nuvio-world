// "OCULTAR NAO LANCADOS" (hideUnreleasedContent) para catalogos e colecoes.
// Prefere a data ISO completa; quando a fonte so informa o ano, conserva o
// comportamento seguro anterior: so esconde anos posteriores ao atual.
#ifndef NV_NLANC_H
#define NV_NLANC_H
#include <stddef.h>

static inline int nlanc_iso_valida(const char *s) {
  if (!s || s[0] < '0' || s[0] > '9' ||
      s[1] < '0' || s[1] > '9' || s[2] < '0' || s[2] > '9' ||
      s[3] < '0' || s[3] > '9' || s[4] != '-' ||
      s[5] < '0' || s[5] > '9' || s[6] < '0' || s[6] > '9' ||
      s[7] != '-' || s[8] < '0' || s[8] > '9' ||
      s[9] < '0' || s[9] > '9') return 0;
  int mes = (s[5]-'0')*10 + s[6]-'0';
  int dia = (s[8]-'0')*10 + s[9]-'0';
  return mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31;
}

// 1 quando a data completa e posterior a hoje. Datas ISO YYYY-MM-DD podem
// ser comparadas como texto porque seus campos estao em ordem decrescente.
static inline int nlanc_data_futura(const char *data, const char *hoje) {
  char a[11], b[11];
  if (!nlanc_iso_valida(data) || !nlanc_iso_valida(hoje)) return 0;
  for (int i = 0; i < 10; i++) { a[i] = data[i]; b[i] = hoje[i]; }
  a[10] = b[10] = 0;
  return a[0] && b[0] && __builtin_strcmp(a, b) > 0;
}

// Fallback para fontes que so enviam o ano (por exemplo, listas do Trakt).
static inline int nlanc_meta_futuro(const char *meta, int anoAtual) {
  int ano = 0;
  if (!meta) return 0;
  for (int i = 0; i < 4; i++) {
    if (meta[i] < '0' || meta[i] > '9') return 0;
    ano = ano * 10 + (meta[i] - '0');
  }
  if (meta[4] >= '0' && meta[4] <= '9') return 0;
  return ano > anoAtual;
}

static inline int nlanc_futuro(const char *data, const char *meta,
                               const char *hoje, int anoAtual) {
  if (nlanc_iso_valida(data) && nlanc_iso_valida(hoje))
    return nlanc_data_futura(data, hoje);
  return nlanc_meta_futuro(meta, anoAtual);
}
#endif
