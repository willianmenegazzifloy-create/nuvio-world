#include <assert.h>
#include <stdio.h>
#include "../src/nlanc.h"

int main(void) {
  const char *hoje = "2026-11-12";
  assert(nlanc_iso_valida("2026-11-11"));
  assert(nlanc_iso_valida("2026-11-12T00:00:00Z"));
  assert(!nlanc_iso_valida("2026-02-29"));
  assert(nlanc_iso_valida("2028-02-29"));
  assert(!nlanc_iso_valida("2026-13-01"));
  assert(!nlanc_iso_valida("2026-11-00"));
  assert(!nlanc_iso_valida("2026-1-01"));
  assert(nlanc_data_futura("2026-11-13", hoje));
  assert(!nlanc_data_futura("2026-11-12", hoje));
  assert(!nlanc_data_futura("2026-11-11", hoje));
  assert(nlanc_data_futura("2026-11-13T00:00:00Z", hoje));
  assert(!nlanc_data_futura("2026-11-12T00:00:00Z", hoje));
  assert(nlanc_futuro("2026-12-01", "2026", hoje, 2026));
  assert(nlanc_futuro("2027-01-01", "2027", hoje, 2026));
  assert(!nlanc_futuro("", "2026", hoje, 2026));
  assert(nlanc_futuro("", "2027", hoje, 2026));
  assert(!nlanc_futuro("", "2025", hoje, 2026));
  assert(!nlanc_meta_futuro("20245", 2026));
  assert(nlanc_tem_info("2026-11-12", ""));
  assert(nlanc_tem_info("", "2026"));
  assert(!nlanc_tem_info("", ""));
  { char data[16];
    assert(nlanc_data_escolher(data, sizeof data,
                               "2026-11-20T00:00:00Z", "", "2026"));
    assert(!strcmp(data, "2026-11-20"));
    assert(nlanc_data_escolher(data, sizeof data, "", "2026-11-21", "2026"));
    assert(!strcmp(data, "2026-11-21"));
    assert(nlanc_data_escolher(data, sizeof data, "", "", "2026-11-22"));
    assert(!strcmp(data, "2026-11-22"));
    assert(!nlanc_data_escolher(data, sizeof data, "", "", "2026"));
    assert(!data[0]);
  }
  puts("nlanc: exact premiere-date filtering PASS");
  return 0;
}
