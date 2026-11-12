#include <assert.h>
#include <stdio.h>
#include "../src/nlanc.h"

int main(void) {
  const char *hoje = "2026-11-11";
  assert(nlanc_iso_valida("2026-11-11"));
  assert(nlanc_iso_valida("2026-11-12T00:00:00Z"));
  assert(!nlanc_iso_valida("2026-02-29"));
  assert(nlanc_iso_valida("2028-02-29"));
  assert(!nlanc_iso_valida("2026-13-01"));
  assert(!nlanc_iso_valida("2026-11-00"));
  assert(!nlanc_iso_valida("2026-1-01"));
  assert(nlanc_data_futura("2026-11-12", hoje));
  assert(!nlanc_data_futura("2026-11-11", hoje));
  assert(!nlanc_data_futura("2026-11-10", hoje));
  assert(nlanc_futuro("2027-01-01", "2027", hoje, 2026));
  assert(nlanc_futuro("2026-12-01", "2026", hoje, 2026));
  assert(!nlanc_futuro("", "2026", hoje, 2026));
  assert(nlanc_futuro("", "2027", hoje, 2026));
  assert(!nlanc_futuro("", "2025", hoje, 2026));
  assert(!nlanc_meta_futuro("20245", 2026));
  puts("nlanc: full ISO release-date filtering PASS");
  return 0;
}
