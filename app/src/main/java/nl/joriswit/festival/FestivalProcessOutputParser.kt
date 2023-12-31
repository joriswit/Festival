package nl.joriswit.festival

import android.util.Log
import java.io.BufferedReader
import java.io.InputStream
import java.io.InputStreamReader
import java.nio.charset.StandardCharsets

class FestivalProcessOutputParser(private val inputStream: InputStream) {

    suspend fun readBoards() = sequence {
        val reader =
            BufferedReader(InputStreamReader(inputStream, StandardCharsets.UTF_8), 8192)

        var xsb: StringBuilder? = null
        var readingBoard = false

        while (true) {
            val line = reader.readLine() ?: break
            Log.i("FestivalOutput", line)
            if (readingBoard) {
                if (line.matches("^\\d .*$".toRegex())) {
                    val tileRegex = "\u001B\\[(\\d+);(\\d+)m(.)".toRegex()
                    tileRegex.findAll(line).forEach { res ->
                        val forgC = res.groups[1]!!.value.toInt()
                        val backC = res.groups[2]!!.value.toInt()
                        val character = res.groups[3]!!.value

                        xsb!!.append(getTile(forgC, backC, character))
                    }
                    xsb!!.appendLine()
                } else {
                    readingBoard = false
                    val board = Board.fromXSB(xsb!!.toString())
                    yield(board)
                }
            }
            if (line.matches("^  \\d+$".toRegex())) {
                xsb = StringBuilder()
                readingBoard = true
            }
        }

        reader.close()
    }

    private fun getTile(
        forgC: Int,
        backC: Int,
        character: String
    ): Char {
        if (forgC == getTerminalColorCode(TerminalColor.WHITE, 0)
            && backC == getTerminalColorCode(TerminalColor.WHITE, 1)
            && character == " "
        ) {
            return Board.SPACE
        } else if (forgC == getTerminalColorCode(TerminalColor.CYAN, 0)
            && backC == getTerminalColorCode(TerminalColor.CYAN, 1)
            && character == " "
        ) {
            return Board.WALL
        } else if (forgC == getTerminalColorCode(TerminalColor.DARKGRAY, 0)
            && backC == getTerminalColorCode(TerminalColor.WHITE, 1)
            && character == "\u25AE"
        ) {
            return Board.BOX
        } else if (forgC == getTerminalColorCode(TerminalColor.RED, 0)
            && backC == getTerminalColorCode(TerminalColor.RED, 1)
            && character == " "
        ) {
            return Board.TARGET
        } else if (forgC == getTerminalColorCode(TerminalColor.BLUE, 0)
            && backC == getTerminalColorCode(TerminalColor.WHITE, 1)
            && character == "."
        ) {
            return Board.SOKOBAN
        } else if (forgC == getTerminalColorCode(TerminalColor.DARKGRAY, 0)
            && backC == getTerminalColorCode(TerminalColor.RED, 1)
            && character == "\u25AE"
        ) {
            return Board.PACKED_BOX
        } else if (forgC == getTerminalColorCode(TerminalColor.BLUE, 0)
            && backC == getTerminalColorCode(TerminalColor.RED, 1)
            && character == "."
        ) {
            return Board.SOKOBAN_ON_TARGET
        } else if (forgC == getTerminalColorCode(TerminalColor.BLUE, 0)
            && backC == getTerminalColorCode(TerminalColor.BLUE, 1)
            && character == " "
        ) {
            return Board.SPACE
        } else if (forgC == getTerminalColorCode(TerminalColor.LIGHTBLUE, 0)
            && backC == getTerminalColorCode(TerminalColor.BLUE, 1)
            && character == "."
        ) {
            return Board.SOKOBAN
        } else if (forgC == getTerminalColorCode(TerminalColor.DARKGRAY, 0)
            && backC == getTerminalColorCode(TerminalColor.BLUE, 1)
            && character == "\u25AE"
        ) {
            return Board.BOX
        } else if (forgC == getTerminalColorCode(TerminalColor.DARKGRAY, 0)
            && backC == getTerminalColorCode(TerminalColor.MAGENTA, 1)
            && character == "\u25AE"
        ) {
            return Board.PACKED_BOX
        } else if (forgC == getTerminalColorCode(TerminalColor.DARKGRAY, 0)
            && backC == getTerminalColorCode(TerminalColor.MAGENTA, 1)
            && character == "."
        ) {
            return Board.TARGET
        } else if (forgC == getTerminalColorCode(TerminalColor.LIGHTBLUE, 0)
            && backC == getTerminalColorCode(TerminalColor.MAGENTA, 1)
            && character == "."
        ) {
            return Board.SOKOBAN_ON_TARGET
        } else {
            throw Exception("unknown tile")
        }
    }

    private fun getTerminalColorCode(color: TerminalColor, background: Int): Int {
        when (color) {
            TerminalColor.BLACK -> return 30 + background * 10
            TerminalColor.BLUE -> return 34 + background * 10
            TerminalColor.GREEN -> return 32 + background * 10
            TerminalColor.CYAN -> return 36 + background * 10
            TerminalColor.RED -> return 31 + background * 10
            TerminalColor.MAGENTA -> return 35 + background * 10
            TerminalColor.BROWN -> return 33 + background * 10
            TerminalColor.LIGHTGRAY -> return 37 + background * 10
            TerminalColor.DARKGRAY -> return 90 + background * 10
            TerminalColor.LIGHTBLUE -> return 94 + background * 10
            TerminalColor.LIGHTGREEN -> return 92 + background * 10
            TerminalColor.LIGHTCYAN -> return 96 + background * 10
            TerminalColor.LIGHTRED -> return 91 + background * 10
            TerminalColor.LIGHTMAGENTA -> return 95 + background * 10
            TerminalColor.YELLOW -> return 93 + background * 10
            TerminalColor.WHITE -> return 97 + background * 10
        }
    }

    enum class TerminalColor {
        BLACK,
        BLUE,
        GREEN,
        CYAN,
        RED,
        MAGENTA,
        BROWN,
        LIGHTGRAY,
        DARKGRAY,
        LIGHTBLUE,
        LIGHTGREEN,
        LIGHTCYAN,
        LIGHTRED,
        LIGHTMAGENTA,
        YELLOW,
        WHITE
    }
}