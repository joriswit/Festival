package nl.joriswit.festival

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.SavedStateHandle
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.Closeable
import java.io.File
import java.io.FileReader
import java.io.IOException
import kotlin.coroutines.CoroutineContext

class CloseableCoroutineScope(
    context: CoroutineContext = SupervisorJob() + Dispatchers.Main.immediate
) : Closeable, CoroutineScope {
    override val coroutineContext: CoroutineContext = context
    override fun close() {
        coroutineContext.cancel()
    }
}

class FestivalViewModel(application: Application, private val state: SavedStateHandle) :
    AndroidViewModel(application) {
    private val coroutineScope: CloseableCoroutineScope = CloseableCoroutineScope()

    init {
        this.addCloseable(coroutineScope)
    }

    private val _uiState: MutableStateFlow<FestivalUiState> = run {
        val levelParam = state.get<String>("LEVEL")
        val newFestivalState: FestivalUiState = if (levelParam != null) {
            FestivalUiState(board = Board.fromXSB(levelParam))
        } else {
            FestivalUiState()
        }
        MutableStateFlow(newFestivalState)
    }

    val uiState: StateFlow<FestivalUiState> = _uiState.asStateFlow()
    private var process: Process? = null
    private var processCancelled: Boolean = false

    fun changeActiveTool(newTool: Char) {
        _uiState.update { currentState -> currentState.copy(activeTool = newTool) }
    }

    fun drawUsingTool(x: Int, y: Int) {
        _uiState.update { currentState ->
            val clone = currentState.board.clone()
            clone.draw(x, y, currentState.activeTool)
            currentState.copy(board = clone)
        }
    }

    fun pasteLevel(newLevel: String) {
        val newBoard = Board.fromXSB(newLevel)
        _uiState.update { currentState ->
            currentState.copy(
                board = newBoard,
                playbackBoard = newBoard,
                playbackSolutionPosition = 0
            )
        }
    }

    fun solve() {
        _uiState.update { currentState ->
            currentState.copy(
                mode = Mode.SOLVING,
                playbackBoard = currentState.board
            )
        }

        val tempLevel = File.createTempFile("level", ".sok.tmp", null)
        tempLevel.writeText(uiState.value.board.toXSB())
        val tempLevelFile = tempLevel.toString()

        val tempSolution = File.createTempFile("level", ".output.sok.tmp", null)
        val tempSolutionFile = tempSolution.toString()

        val nativeLibraryDir = getApplication<Application>().applicationInfo.nativeLibraryDir
        Log.i("Festival", nativeLibraryDir)

        coroutineScope.launch(Dispatchers.IO) {
            val nativeLibraryDirFile = File(nativeLibraryDir!!)
            val libraryFile = File(nativeLibraryDirFile, "libfestival.so")
            val command =
                "$libraryFile $tempLevelFile -cores 1 -time ${uiState.value.solverTimeout} -out_file $tempSolutionFile"
            Log.w("Festival", command)

            process = Runtime.getRuntime().exec(command)
            processCancelled = false

            try {

                val reader = FestivalProcessOutputParser(process!!.inputStream)
                reader.readBoards().forEach { board ->
                    ensureActive()
                    _uiState.update { currentState -> currentState.copy(playbackBoard = board) }
                    Log.i("Festival", board.toXSB())
                }

                process!!.errorStream.close()
                process!!.outputStream.close()
                process!!.waitFor()

                val file = File(tempSolutionFile)

                var solution: String? = null
                BufferedReader(FileReader(file)).use { br ->
                    var solutionLine = false
                    br.forEachLine { line ->
                        if (solutionLine) {
                            solution = line
                            solutionLine = false
                        }
                        if (line == "Solution") {
                            solutionLine = true
                        }
                    }
                }
                ensureActive()
                if (solution != null) {
                    _uiState.update { currentState ->
                        currentState.copy(
                            mode = Mode.SOLVED,
                            playbackBoard = currentState.board,
                            solution = solution,
                            playback = PlaybackMode.PLAYING,
                            playbackSolutionPosition = 0,
                        )
                    }
                    launchPlaybackCoroutine()
                } else {
                    _uiState.update { currentState -> currentState.copy(mode = Mode.EDIT) }
                }
            } catch (e: IOException) {
                // Ignore IOException if cancelled
                if (processCancelled) {
                    if (e.message != null) {
                        Log.e("Festival", e.message!!)
                    }
                } else {
                    throw e
                }
            } finally {
                withContext(NonCancellable) {
                    process?.destroy()
                    tempLevel.delete()
                    tempSolution.delete()
                }
            }
        }
    }

    fun cancelSolve() {
        // Stop the solver process
        processCancelled = true
        process?.destroy()

        // Switch back to Edit mode
        _uiState.update { currentState -> currentState.copy(mode = Mode.EDIT) }
    }

    fun setSolverTimeout(timeout: Int) {
        _uiState.update { currentState -> currentState.copy(solverTimeout = timeout) }
    }

    fun playSolution() {
        _uiState.update { currentState ->
            currentState.copy(
                playback = PlaybackMode.PLAYING,
                playbackBoard = if (currentState.playback == PlaybackMode.STOPPED) currentState.board else currentState.playbackBoard
            )
        }
        launchPlaybackCoroutine()
    }

    fun pauseSolution() {
        _uiState.update { currentState -> currentState.copy(playback = PlaybackMode.PAUSED) }
    }

    fun stopSolution() {
        _uiState.update { currentState ->
            currentState.copy(
                playback = PlaybackMode.STOPPED,
                playbackSolutionPosition = 0,
                playbackBoard = currentState.board
            )
        }
    }

    private fun launchPlaybackCoroutine() {
        coroutineScope.launch {
            while (uiState.value.mode == Mode.SOLVED && uiState.value.playback == PlaybackMode.PLAYING) {
                if (uiState.value.playbackSolutionPosition < uiState.value.solution!!.length) {
                    _uiState.update { currentState ->
                        val clone = currentState.playbackBoard.clone()
                        clone.move(currentState.solution!![currentState.playbackSolutionPosition])
                        currentState.copy(
                            playbackBoard = clone,
                            playbackSolutionPosition = currentState.playbackSolutionPosition + 1
                        )
                    }
                } else {
                    _uiState.update { currentState ->
                        currentState.copy(
                            playback = PlaybackMode.STOPPED,
                            playbackSolutionPosition = 0
                        )
                    }
                }
                delay(100)
            }
        }
    }
}
