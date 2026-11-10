#!/usr/bin/env python3
"""Recover this exact Nuvio Legacy 2.0.8 build's public app config from its APK.

The file written to the repository root is local.properties, which is ignored by
.gitignore. Values are intentionally never printed to stdout or stderr.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import zipfile

EXPECTED_SHA256 = "0fddaf1ee1c0f71679884e87e41038ead4ef6cd99ad82f04210bd9e797845353"
APK_MEMBER = "lib/armeabi-v7a/libmain.so"
SUPABASE_URL = "https://api.nuvio.tv"
TV_LOGIN_URL = "https://nuvio.tv/tv-login"


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"extract-config-from-apk: {message}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("apk", type=Path)
    parser.add_argument("--output", type=Path, default=Path("local.properties"))
    args = parser.parse_args()

    if not args.apk.is_file():
        fail("APK de referência não encontrado")

    digest = hashlib.sha256(args.apk.read_bytes()).hexdigest()
    if digest != EXPECTED_SHA256:
        fail("o APK baixado não corresponde ao Nuvio 2.0.8 de referência; nada foi gerado")

    try:
        with zipfile.ZipFile(args.apk) as archive:
            native = archive.read(APK_MEMBER)
    except (zipfile.BadZipFile, KeyError, OSError):
        fail("não foi possível ler a biblioteca nativa ARMv7 do APK")

    if not native.startswith(b"\x7fELF"):
        fail("a biblioteca nativa não é um ELF válido")

    if native.count(SUPABASE_URL.encode()) < 1:
        fail("URL do serviço original não encontrada")
    if native.count(TV_LOGIN_URL.encode()) < 1:
        fail("URL original de login da TV não encontrada")

    jwt_matches = re.findall(
        rb"eyJ[A-Za-z0-9_-]{20,}\.[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}",
        native,
    )
    if len(jwt_matches) != 1:
        fail("não foi possível identificar unicamente a chave anônima do serviço")
    anon_key = jwt_matches[0].decode("ascii")
    try:
        payload = anon_key.split(".")[1]
        payload += "=" * ((4 - len(payload) % 4) % 4)
        claims = json.loads(base64.urlsafe_b64decode(payload))
    except (ValueError, UnicodeError, json.JSONDecodeError):
        fail("a chave do serviço não passou na validação de formato")
    if claims.get("iss") != "supabase" or claims.get("role") != "anon":
        fail("a chave recuperada não corresponde ao tipo esperado")

    # nuvem.c declares its compile-time app credentials in this exact order:
    # Trakt client ID, Trakt client secret, Simkl client ID. The 2.0.8 APK
    # contains exactly these three 64-character hexadecimal values.
    app_ids = list(
        dict.fromkeys(
            match.group().decode("ascii")
            for match in re.finditer(
                rb"(?<![A-Fa-f0-9])[A-Fa-f0-9]{64}(?![A-Fa-f0-9])", native
            )
        )
    )
    if len(app_ids) != 3:
        fail("as credenciais de aplicativo Trakt/Simkl não passaram na validação")

    # In this build the 32-hex TMDB v3 key is stored immediately before the
    # compiled "%s/tmdb.txt" path in the native library. Bind to that anchor
    # rather than guessing among unrelated 32-character strings.
    tmdb_match = re.search(rb"([A-Fa-f0-9]{32})\x00%s/tmdb\.txt\x00", native)
    if not tmdb_match:
        fail("chave TMDB v3 do pacote não encontrada")
    tmdb_key = tmdb_match.group(1).decode("ascii")

    lines = [
        f"NUVIO_SUPABASE_URL={SUPABASE_URL}",
        f"NUVIO_SUPABASE_ANON_KEY={anon_key}",
        f"TV_LOGIN_WEB_BASE_URL={TV_LOGIN_URL}",
        f"TRAKT_CLIENT_ID={app_ids[0]}",
        f"TRAKT_CLIENT_SECRET={app_ids[1]}",
        f"SIMKL_CLIENT_ID={app_ids[2]}",
        "SIMKL_APP_NAME=nuvio",
        f"TMDB_API_KEY={tmdb_key}",
        "NUVIO_REC_URL=",
        "DISCORD_CLIENT_ID=",
    ]

    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    try:
        os.chmod(args.output, 0o600)
    except OSError:
        pass
    # No recovered credential values are ever logged.
    print("Configuração original recuperada e validada; local.properties criado (valores ocultos).")


if __name__ == "__main__":
    main()
