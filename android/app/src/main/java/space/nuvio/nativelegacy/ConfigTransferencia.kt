package space.nuvio.nativelegacy

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import java.io.File
import java.io.FileOutputStream

/** Seletor SAF do pacote de preferencias; somente arquivos permitidos pelo nucleo C. */
class ConfigTransferencia(private val activity: NuvioActivity) {
    private data class Pedido(val tipo: Int, val arquivo: String)
    private var pedido: Pedido? = null
    private var fechado = false
    var escolhendoArquivo = false
        private set

    fun exportar(arquivo: String) = abrir(Pedido(EXPORTAR, arquivo))
    fun importar() = abrir(Pedido(IMPORTAR, ""))

    private fun abrir(novo: Pedido) {
        if (fechado || pedido != null || activity.isFinishing || activity.isDestroyed) return
        pedido = novo
        try {
            escolhendoArquivo = true
            @Suppress("DEPRECATION")
            activity.startActivityForResult(
                if (novo.tipo == EXPORTAR) documentoExportar() else documentoImportar(),
                if (novo.tipo == EXPORTAR) PEDIDO_EXPORTAR else PEDIDO_IMPORTAR
            )
        } catch (_: Exception) {
            pedido = null
            escolhendoArquivo = false
            avisar("Não foi possível abrir o seletor de arquivos.")
        }
    }

    fun resultado(codigo: Int, resultado: Int, data: Intent?): Boolean {
        if (codigo != PEDIDO_EXPORTAR && codigo != PEDIDO_IMPORTAR) return false
        if (!escolhendoArquivo) return true
        escolhendoArquivo = false
        val atual = pedido ?: return true
        pedido = null
        val uri = data?.data
        if (resultado != Activity.RESULT_OK || uri == null) return true
        Thread({
            if (atual.tipo == EXPORTAR) {
                val ok = try {
                    val destino = activity.contentResolver.openOutputStream(uri, "wt")
                        ?: throw IllegalStateException()
                    File(atual.arquivo).inputStream().use { origem ->
                        destino.use { saida -> origem.copyTo(saida) }
                    }
                    true
                } catch (_: Exception) { false }
                activity.runOnUiThread {
                    avisar(if (ok) "Configurações exportadas." else "Não foi possível exportar as configurações.")
                }
            } else {
                val ok = try {
                    val arquivo = File(activity.cacheDir, "nuvio-config-import.json")
                    val entrada = activity.contentResolver.openInputStream(uri)
                        ?: throw IllegalStateException()
                    entrada.use { origem -> FileOutputStream(arquivo).use { destino ->
                        val buffer = ByteArray(8192)
                        var total = 0L
                        while (true) {
                            val n = origem.read(buffer)
                            if (n < 0) break
                            total += n
                            if (total > MAX_BYTES) throw IllegalArgumentException("arquivo grande")
                            destino.write(buffer, 0, n)
                        }
                    } }
                    activity.adicionarEventoConfig(arquivo.absolutePath)
                    true
                } catch (_: Exception) { false }
                if (!ok) activity.runOnUiThread { avisar("Não foi possível ler esse arquivo JSON.") }
            }
        }, "nuvio-config-transfer").start()
        return true
    }

    fun notificar(sucesso: Boolean, tipo: Int) {
        val mensagem = when {
            !sucesso -> if (tipo == NOTIF_EXPORTAR) "Não foi possível exportar as configurações." else "Arquivo inválido ou não foi possível aplicar as configurações."
            tipo == NOTIF_IMPORTAR -> "Configurações importadas. Reinicie o Nuvio para aplicar."
            tipo == NOTIF_COPIAR -> "Configurações copiadas. Reinicie o Nuvio para aplicar."
            tipo == NOTIF_EXPORTAR -> "Configurações exportadas."
            else -> "Operação concluída."
        }
        avisar(mensagem)
    }

    fun salvar(estado: Bundle) {
        val atual = pedido ?: return
        if (!escolhendoArquivo) return
        estado.putBundle("configTransferencia", Bundle().apply {
            putInt("tipo", atual.tipo)
            putString("arquivo", atual.arquivo)
        })
    }

    fun restaurar(estado: Bundle?) {
        val salvo = estado?.getBundle("configTransferencia") ?: return
        val tipo = salvo.getInt("tipo", 0)
        if (tipo != EXPORTAR && tipo != IMPORTAR) return
        pedido = Pedido(tipo, salvo.getString("arquivo") ?: "")
        escolhendoArquivo = true
    }

    fun fechar() {
        fechado = true
        pedido = null
        escolhendoArquivo = false
    }

    private fun avisar(mensagem: String) {
        if (!fechado && !activity.isFinishing && !activity.isDestroyed)
            Toast.makeText(activity, mensagem, Toast.LENGTH_LONG).show()
    }

    companion object {
        private const val EXPORTAR = 1
        private const val IMPORTAR = 2
        private const val NOTIF_IMPORTAR = 1
        private const val NOTIF_COPIAR = 2
        private const val NOTIF_EXPORTAR = 3
        private const val PEDIDO_EXPORTAR = 49118
        private const val PEDIDO_IMPORTAR = 49119
        private const val MAX_BYTES = 512 * 1024

        private fun documentoExportar() = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, "Nuvio-Configuracoes.json")
        }

        private fun documentoImportar() = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_MIME_TYPES, arrayOf("application/json", "text/json"))
        }
    }
}
