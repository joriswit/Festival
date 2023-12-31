package nl.joriswit.festival

import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.content.Intent
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Backspace
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.BottomAppBar
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color.Companion.Unspecified
import androidx.compose.ui.platform.ClipboardManager
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringArrayResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.tooling.preview.Preview
import androidx.lifecycle.viewmodel.compose.viewModel
import nl.joriswit.festival.ui.theme.FestivalTheme


class FestivalComponentActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            FestivalTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    FestivalScaffoldComposable()
                }
            }
        }
    }
}

fun Context.getActivity(): FestivalComponentActivity? {
    var currentContext = this
    while (currentContext is ContextWrapper) {
        if (currentContext is FestivalComponentActivity) {
            return currentContext
        }
        currentContext = currentContext.baseContext
    }
    return null
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FestivalScaffoldComposable(viewModel: FestivalViewModel = viewModel()) {
    val state by viewModel.uiState.collectAsState()
    var showBackToEditModeConfirm by remember { mutableStateOf(false) }
    val activity = LocalContext.current.getActivity()
    if (activity != null) {
        if (state.mode == Mode.SOLVED) {
            val resultIntent = Intent()
            resultIntent.putExtra("SOLUTION", state.solution)
            activity.setResult(Activity.RESULT_OK, resultIntent)
        }
        // Keep the screen on when the user is passively watching the solver "best so far" state,
        // or the final solution animation.
        if (state.mode == Mode.SOLVING || (state.mode == Mode.SOLVED && state.playback == PlaybackMode.PLAYING)) {
            activity.window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        } else {
            activity.window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }

    Scaffold(
        topBar = {
            var showMenu by remember { mutableStateOf(false) }
            var showTimeoutMenu by remember { mutableStateOf(false) }
            TopAppBar(
                title = {
                    Text(stringResource(R.string.activity_name))
                },
                actions = {
                    IconButton(onClick = { showMenu = true }) {
                        Icon(
                            imageVector = Icons.Filled.Menu,
                            contentDescription = stringResource(R.string.menu)
                        )
                    }
                    DropdownMenu(
                        expanded = showMenu,
                        onDismissRequest = { showMenu = false }
                    ) {
                        val clipboardManager: ClipboardManager = LocalClipboardManager.current
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.action_paste_level)) },
                            onClick = {
                                clipboardManager.getText()?.text?.let {
                                    viewModel.pasteLevel(it)
                                }
                                showMenu = false
                            },
                            enabled = state.mode == Mode.EDIT && clipboardManager.hasText()
                        )
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.action_copy_level)) },
                            onClick = {
                                clipboardManager.setText(AnnotatedString(state.board.toXSB()))
                                showMenu = false
                            })
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.action_copy_solution)) },
                            onClick = {
                                clipboardManager.setText(AnnotatedString(state.solution!!))
                                showMenu = false
                            },
                            enabled = state.mode == Mode.SOLVED
                        )
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.preferences_solver_search_time_title_text)) },
                            onClick = {
                                showTimeoutMenu = true
                                showMenu = false
                            },
                            enabled = state.mode != Mode.SOLVING
                        )
                    }
                    DropdownMenu(
                        expanded = showTimeoutMenu,
                        onDismissRequest = { showTimeoutMenu = false }
                    ) {
                        val timeoutList =
                            stringArrayResource(R.array.preferences_solver_search_time_entry_text_array)
                                .zip(stringArrayResource(R.array.preferences_solver_search_time_entry_key_array))
                        for (time in timeoutList) {
                            val timeout = time.second.toInt()
                            DropdownMenuItem(text = { Text(time.first) }, leadingIcon = {
                                if (timeout == state.solverTimeout) {
                                    Icon(
                                        Icons.Filled.Check,
                                        contentDescription = stringResource(R.string.checked)
                                    )
                                }
                            }, onClick = {
                                viewModel.setSolverTimeout(timeout)
                                showTimeoutMenu = false
                            })
                        }
                    }
                },

                )
        },
        bottomBar = {
            BottomAppBar(
                actions = {
                    if (state.mode == Mode.EDIT) {
                        data class EditTool(
                            val tile: Char,
                            val drawable: Int,
                            val contentDescription: Int
                        )

                        val editTools = listOf(
                            EditTool(Board.WALL, R.drawable.wall, R.string.edit_wall),
                            EditTool(Board.BOX, R.drawable.box, R.string.edit_box),
                            EditTool(Board.TARGET, R.drawable.target, R.string.edit_target),
                            EditTool(Board.SOKOBAN, R.drawable.sokoban, R.string.edit_sokoban),
                            EditTool(Board.SPACE, R.drawable.space, R.string.edit_space),
                        )
                        for (editTool in editTools) {
                            IconButton(
                                onClick = { viewModel.changeActiveTool(editTool.tile) },
                                modifier = Modifier.background(if (state.activeTool == editTool.tile) MaterialTheme.colorScheme.primaryContainer else Unspecified)
                            ) {
                                Icon(
                                    painterResource(editTool.drawable),
                                    tint = Unspecified,
                                    contentDescription = stringResource(id = editTool.contentDescription),
                                )
                            }
                        }
                    }
                    if (state.mode == Mode.SOLVED) {
                        IconButton(onClick = { showBackToEditModeConfirm = true }) {
                            Icon(
                                Icons.Filled.Backspace,
                                contentDescription = stringResource(id = R.string.back_to_edit_mode)
                            )
                        }
                        if (state.playback != PlaybackMode.PLAYING) {
                            IconButton(onClick = { viewModel.playSolution() }) {
                                Icon(
                                    Icons.Filled.PlayArrow,
                                    contentDescription = stringResource(id = R.string.play_solution)
                                )
                            }
                        } else {
                            IconButton(onClick = { viewModel.pauseSolution() }) {
                                Icon(
                                    Icons.Filled.Pause,
                                    contentDescription = stringResource(id = R.string.pause_solution)
                                )
                            }
                        }
                        IconButton(
                            onClick = { viewModel.stopSolution() },
                            enabled = state.playback != PlaybackMode.STOPPED || state.playbackSolutionPosition > 0
                        ) {
                            Icon(
                                Icons.Filled.Stop,
                                contentDescription = stringResource(id = R.string.stop_solution)
                            )
                        }
                        Column {
                            Text(stringResource(R.string.current_solution_label_text))
                            Text(
                                stringResource(
                                    R.string.current_solution_text,
                                    state.countSolutionMoves(),
                                    state.countSolutionPushes()
                                )
                            )
                        }
                    }
                }
            )
        },
        floatingActionButton = {
            if (state.mode == Mode.EDIT) {
                FloatingActionButton(onClick = {
                    viewModel.solve()
                }) {
                    Text(stringResource(R.string.action_solve))
                }
            }
            if (state.mode == Mode.SOLVING) {
                FloatingActionButton(onClick = {
                    viewModel.cancelSolve()
                }) {
                    Icon(
                        Icons.Default.Close,
                        contentDescription = stringResource(R.string.action_solve_cancel)
                    )
                }
            }
        }
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding),
        ) {
            if (state.mode == Mode.EDIT) {
                BoardComposable(state.board, false, onClickTile = fun(x, y) {
                    viewModel.drawUsingTool(x, y)
                })
            } else {
                BoardComposable(state.playbackBoard, state.mode == Mode.SOLVING)
            }
        }
    }

    if (showBackToEditModeConfirm) {
        AlertDialog(
            title = {
                Text(text = "Festival")
            },
            text = {
                Text(text = stringResource(id = R.string.back_to_edit_mode_confirm))
            },
            onDismissRequest = {
                showBackToEditModeConfirm = false
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        showBackToEditModeConfirm = false
                        viewModel.cancelSolve()
                    }
                ) {
                    Text(stringResource(id = android.R.string.ok))
                }
            },
            dismissButton = {
                TextButton(
                    onClick = {
                        showBackToEditModeConfirm = false
                    }
                ) {
                    Text(stringResource(id = android.R.string.cancel))
                }
            }
        )
    }
}

@Preview(showBackground = true)
@Composable
fun FestivalScaffoldComposablePreview() {
    FestivalTheme {
        FestivalScaffoldComposable()
    }
}
