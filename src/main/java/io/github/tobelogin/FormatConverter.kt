package io.github.tobelogin

class FormatConverter {
    companion object {
        init {
            System.loadLibrary("av-helper")
        }

        fun list2webp(listPath: String, webpPath: String): Int {
            return nativeList2Webp(listPath.toByteArray(Charsets.UTF_8), webpPath.toByteArray(Charsets.UTF_8))
        }
        external fun nativeList2Webp(listPath: ByteArray, webpPath: ByteArray): Int
    }
}