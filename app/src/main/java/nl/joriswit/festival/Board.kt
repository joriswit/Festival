package nl.joriswit.festival

class Board private constructor() : Cloneable {
    private var _boardLines = ArrayList<String>()

    val height get() = _boardLines.size
    val width get() = _boardLines[0].length

    fun getXY(x: Int, y: Int): Char {
        return _boardLines[y][x]
    }

    private fun setXY(x: Int, y: Int, tile: Char) {
        _boardLines[y] = _boardLines[y].replaceRange(x, x + 1, tile.toString())
    }

    private fun inBounds(x: Int, y: Int): Boolean {
        return y >= 0 && x >= 0 && y < height && x < width
    }

    fun toXSB(): String {
        val outputBuffer = StringBuilder()
        for (line in _boardLines) {
            outputBuffer.append(line)
            outputBuffer.append("\n")
        }
        return outputBuffer.toString()
    }

    /**
     * Do a player move, returns true if a valid move is done
     * @param direction the character indicating the direction of the move
     */
    fun move(direction: Char): Boolean {
        var playerX = -1
        var playerY = -1
        for (y in 0 until height) {
            val line = _boardLines[y]
            for (x in line.indices) {
                if (line[x] == SOKOBAN || line[x] == SOKOBAN_ON_TARGET) {
                    playerX = x
                    playerY = y
                }
            }
        }
        if (playerX == -1) {
            return false
        }
        var nextPosX = playerX
        var nextPosY = playerY
        var secondPosX = playerX
        var secondPosY = playerY
        when (direction.lowercaseChar()) {
            'u' -> {
                nextPosY = playerY - 1
                secondPosY = playerY - 2
            }

            'd' -> {
                nextPosY = playerY + 1
                secondPosY = playerY + 2
            }

            'l' -> {
                nextPosX = playerX - 1
                secondPosX = playerX - 2
            }

            'r' -> {
                nextPosX = playerX + 1
                secondPosX = playerX + 2
            }

            else -> return false
        }
        if (!inBounds(nextPosX, nextPosY)) {
            return false
        }
        if (getXY(nextPosX, nextPosY) == BOX || getXY(nextPosX, nextPosY) == PACKED_BOX) {
            // Push
            if (!inBounds(secondPosX, secondPosY)) {
                return false
            }
            when (getXY(secondPosX, secondPosY)) {
                SPACE, SPACE_ALT1, SPACE_ALT2 -> setXY(secondPosX, secondPosY, BOX)
                TARGET -> setXY(secondPosX, secondPosY, PACKED_BOX)
                else -> return false
            }
        }

        when (getXY(nextPosX, nextPosY)) {
            SPACE, SPACE_ALT1, SPACE_ALT2, BOX -> setXY(nextPosX, nextPosY, SOKOBAN)
            TARGET, PACKED_BOX -> setXY(nextPosX, nextPosY, SOKOBAN_ON_TARGET)
            else -> return false
        }

        if (getXY(playerX, playerY) == SOKOBAN_ON_TARGET) {
            setXY(playerX, playerY, TARGET)
        } else {
            setXY(playerX, playerY, SPACE)
        }
        return true
    }

    /**
     * Perform multiple player moves
     * @param lurd the string containing the player moves
     * @return true if all moves are valid
     */
    fun move(lurd: String): Boolean {
        for (c in lurd) {
            if (!move(c)) {
                return false
            }
        }
        return true
    }

    private fun removePlayer() {
        for (y in 0 until height) {
            val line = _boardLines[y]
            for (x in line.indices) {
                if (line[x] == SOKOBAN) {
                    setXY(x, y, SPACE)
                }
                if (line[x] == SOKOBAN_ON_TARGET) {
                    setXY(x, y, TARGET)
                }
            }
        }
    }

    private fun enlargeLeft() {
        for (i in 0 until height) {
            _boardLines[i] = " " + _boardLines[i]
        }
    }

    private fun enlargeRight() {
        for (i in 0 until height) {
            _boardLines[i] = _boardLines[i] + " "
        }
    }

    private fun enlargeTop() {
        _boardLines.add(0, SPACE.toString().repeat(width))
    }

    private fun enlargeBottom() {
        _boardLines.add(SPACE.toString().repeat(width))
    }

    fun draw(xp: Int, yp: Int, activeTool: Char) {
        var x = xp
        var y = yp
        while (x < 0) {
            if (width >= MAX_BOARD_WIDTH) return
            enlargeLeft()
            x++
        }
        while (x >= width) {
            if (width >= MAX_BOARD_WIDTH) return
            enlargeRight()
        }
        while (y < 0) {
            if (height >= MAX_BOARD_WIDTH) return
            enlargeTop()
            y++
        }
        while (y >= height) {
            if (height >= MAX_BOARD_WIDTH) return
            enlargeBottom()
        }
        var value = getXY(x, y)
        when (activeTool) {
            WALL -> value = WALL
            BOX -> {
                value = if (value == TARGET || value == SOKOBAN_ON_TARGET || value == BOX) {
                    PACKED_BOX
                } else {
                    BOX
                }
            }

            SOKOBAN -> {
                removePlayer()
                value = if (value == TARGET || value == PACKED_BOX || value == SOKOBAN) {
                    SOKOBAN_ON_TARGET
                } else {
                    SOKOBAN
                }
            }

            TARGET -> {
                value = when (value) {
                    BOX -> PACKED_BOX
                    SOKOBAN -> SOKOBAN_ON_TARGET
                    TARGET -> PACKED_BOX
                    else -> TARGET
                }
            }

            SPACE -> value = SPACE
        }
        setXY(x, y, value)
    }

    public override fun clone(): Board {
        val b = Board()
        b._boardLines.addAll((_boardLines.clone() as ArrayList<*>).filterIsInstance<String>())
        return b
    }

    companion object {
        const val MAX_BOARD_WIDTH = 51

        const val WALL = '#'
        const val SPACE = ' '
        const val SPACE_ALT1 = '-'
        const val SPACE_ALT2 = '_'
        const val BOX = '$'
        const val SOKOBAN = '@'
        const val TARGET = '.'
        const val PACKED_BOX = '*'
        const val SOKOBAN_ON_TARGET = '+'

        fun fromXSB(level: String): Board {
            val lines =
                ArrayList(listOf(*level.split("\\n".toRegex()).dropLastWhile { it.isEmpty() }
                    .toTypedArray()))

            // Remove CR character (might be left over after splitting CRLF line ending on LF)
            for (row in lines.indices) {
                lines[row] = lines[row].replace("\r", "")
            }
            // Remove leading non-board lines
            while (lines.size > 0 && !isABoardLine(lines[0])) {
                lines.removeAt(0)
            }
            // Remove trailing non-board lines
            while (lines.size > 0 && !isABoardLine(lines[lines.size - 1])) {
                lines.removeAt(lines.size - 1)
            }
            // Find maximum width
            var width = 0
            for (line in lines) {
                width = width.coerceAtLeast(line.length)
            }
            // Make all lines the same length
            for (row in lines.indices) {
                while (lines[row].length < width) {
                    lines[row] = lines[row] + " "
                }
            }
            return if (lines.size > 0 && width > 0) {
                val newBoard = Board()
                newBoard._boardLines.addAll(lines)
                newBoard
            } else {
                createEmpty()
            }
        }

        fun createEmpty(): Board {
            val newBoard = Board()
            newBoard._boardLines.add("      ")
            newBoard._boardLines.add("      ")
            newBoard._boardLines.add("      ")
            newBoard._boardLines.add("      ")
            newBoard._boardLines.add("      ")
            newBoard._boardLines.add("      ")
            return newBoard
        }

        private fun isABoardLine(textLine: String): Boolean {
            // skip trailing spaces and other floor characters, e.g., '_'
            // first and last character in a line belonging to a board must be a wall or a box-on-target
            return textLine.matches("^[\\-_ ]*[#*]([# _\\-.$*@+]*[#*])?[_\\- ]*$".toRegex())
        }

    }

}