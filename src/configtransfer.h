#ifndef NV_CONFIGTRANSFER_H
#define NV_CONFIGTRANSFER_H
#include <stddef.h>

// Pacote de preferencias sem credenciais. Exporta para um arquivo temporario
// em dados_dir(); dst recebe o caminho completo para o seletor da plataforma.
int configtransfer_exportar(char *dst, size_t tam);
// Valida o pacote inteiro antes de gravar; arquivos por perfil sao remapeados
// para o perfil ativo neste aparelho.
int configtransfer_importar(const char *caminho);
// Copia apenas preferencias de perfil, nunca credenciais ou dados da conta.
int configtransfer_copiar_perfil(int origem, int destino);
#endif
