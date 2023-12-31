package nl.joriswit.festival

import android.graphics.BitmapFactory
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.FilterQuality
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import kotlin.math.floor

private val wallResIds = intArrayOf(
    R.drawable.wall,
    R.drawable.wall_u,
    R.drawable.wall_d,
    R.drawable.wall_ud,
    R.drawable.wall_l,
    R.drawable.wall_ul,
    R.drawable.wall_dl,
    R.drawable.wall_udl,
    R.drawable.wall_r,
    R.drawable.wall_ur,
    R.drawable.wall_dr,
    R.drawable.wall_udr,
    R.drawable.wall_lr,
    R.drawable.wall_ulr,
    R.drawable.wall_dlr,
    R.drawable.wall_udlr,
)

private fun Board.getWallIndex(x: Int, y: Int): Int {
    var wallIndex = 0
    if (y > 0 && this.getXY(x, y - 1) == Board.WALL) wallIndex += 1
    if (y < this.height - 1 && this.getXY(x, y + 1) == Board.WALL) wallIndex += 2
    if (x > 0 && this.getXY(x - 1, y) == Board.WALL) wallIndex += 4
    if (x < this.width - 1 && this.getXY(x + 1, y) == Board.WALL) wallIndex += 8
    return wallIndex
}

private fun Board.shouldDrawWallTop(x: Int, y: Int): Boolean {
    return x > 0 && y > 0
            && this.getXY(x, y) == Board.WALL
            && this.getXY(x, y - 1) == Board.WALL
            && this.getXY(x - 1, y) == Board.WALL
            && this.getXY(x - 1, y - 1) == Board.WALL
}

@Composable
fun BoardComposable(
    board: Board,
    useSmallSokoban: Boolean,
    modifier: Modifier = Modifier,
    onClickTile: (x: Int, y: Int) -> Unit = fun(_, _) {}
) {
    val boardWidth by rememberUpdatedState(newValue = board.width)
    val boardHeight by rememberUpdatedState(newValue = board.height)

    val context = LocalContext.current
    val options = BitmapFactory.Options()
    options.inScaled = false
    val spaceColor = colorResource(id = R.color.space)
    val targetImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.target, options).asImageBitmap()
    }
    val boxImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.box, options).asImageBitmap()
    }
    val packedBoxImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.packed_box, options)
            .asImageBitmap()
    }
    val sokobanImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.sokoban, options).asImageBitmap()
    }
    val sokobanOnTargetImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.sokoban_on_target, options)
            .asImageBitmap()
    }
    val smallSokobanImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.sokoban_small, options)
            .asImageBitmap()
    }
    val smallSokobanOnTargetImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.sokoban_on_target_small, options)
            .asImageBitmap()
    }
    val wallImageBitmaps = remember {
        wallResIds.map { r ->
            BitmapFactory.decodeResource(context.resources, r, options).asImageBitmap()
        }
    }
    val wallTopImageBitmap = remember {
        BitmapFactory.decodeResource(context.resources, R.drawable.wall_top, options)
            .asImageBitmap()
    }
    Canvas(modifier = modifier
        .fillMaxSize()
        .pointerInput(Unit) {
            detectTapGestures(
                onTap = { tapOffset ->
                    val tileSize =
                        ( size.width / (boardWidth + 2)).coerceAtMost(size.height / (boardHeight + 2))
                    val originX = (size.width - tileSize * boardWidth) / 2
                    val originY = (size.height - tileSize * boardHeight) / 2
                    val x = floor(((tapOffset.x - originX) / tileSize).toDouble()).toInt()
                    val y = floor(((tapOffset.y - originY) / tileSize).toDouble()).toInt()
                    onClickTile(x, y)
                }
            )
        }, onDraw = {

        drawRect(spaceColor, Offset.Zero, this.size)

        val canvasWidth = size.width.toInt()
        val canvasHeight = size.height.toInt()
        val tileSize = (canvasWidth / (boardWidth + 2)).coerceAtMost(canvasHeight / (boardHeight + 2))
        val originX = (canvasWidth - tileSize * boardWidth) / 2
        val originY = (canvasHeight - tileSize * boardHeight) / 2
        for (y in 0 until boardHeight) {
            for (x in 0 until boardWidth) {
                val bitmap = when (board.getXY(x, y)) {
                    Board.TARGET -> targetImageBitmap
                    Board.BOX -> boxImageBitmap
                    Board.PACKED_BOX -> packedBoxImageBitmap
                    Board.SOKOBAN -> if (useSmallSokoban) smallSokobanImageBitmap else sokobanImageBitmap
                    Board.SOKOBAN_ON_TARGET -> if (useSmallSokoban) smallSokobanOnTargetImageBitmap else sokobanOnTargetImageBitmap
                    Board.WALL -> wallImageBitmaps[board.getWallIndex(x, y)]
                    else -> null
                }
                if (bitmap != null) {
                    val left = originX + x * tileSize
                    val top = originY + y * tileSize
                    drawImage(
                        bitmap,
                        IntOffset.Zero,
                        IntSize(50, 50),
                        IntOffset(left, top),
                        IntSize(tileSize, tileSize),
                        filterQuality = FilterQuality.High
                    )
                    if (board.shouldDrawWallTop(x, y)) {
                        drawImage(
                            wallTopImageBitmap,
                            IntOffset.Zero,
                            IntSize(50, 50),
                            IntOffset(left - tileSize / 2, top - tileSize / 2),
                            IntSize(tileSize, tileSize)
                        )
                    }
                }
            }
        }
    })
}
