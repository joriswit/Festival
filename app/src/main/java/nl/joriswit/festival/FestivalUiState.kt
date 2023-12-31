package nl.joriswit.festival

enum class Mode {
    EDIT,
    SOLVING,
    SOLVED
}

enum class PlaybackMode {
    STOPPED,
    PLAYING,
    PAUSED
}

data class FestivalUiState(
    val mode: Mode = Mode.EDIT,
    val activeTool: Char = Board.WALL,
    val board: Board = Board.createEmpty(),
    val playbackBoard: Board = board,
    val solution: String? = null,
    val playbackSolutionPosition: Int = 0,
    val playback: PlaybackMode = PlaybackMode.STOPPED,
    var solverTimeout: Int = 600,
) {
    fun countSolutionMoves(): Int {
        return solution?.length ?: 0
    }

    fun countSolutionPushes(): Int {
        return solution?.count { c -> c.isUpperCase() } ?: 0
    }
}