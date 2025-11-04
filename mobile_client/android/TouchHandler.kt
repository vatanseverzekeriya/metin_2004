package com.metin2.pvp.ui

import android.view.MotionEvent
import android.view.View

/**
 * Android Touch Event Handler
 * Metin2 PvP C++ UI sistemine entegrasyon
 */
class TouchHandler(private val nativePtr: Long) : View.OnTouchListener {

    // Touch event tipleri (C++ ile eşleşmeli)
    private val TOUCH_DOWN = 0
    private val TOUCH_MOVE = 1
    private val TOUCH_UP = 2
    private val TOUCH_CANCEL = 3

    override fun onTouch(v: View?, event: MotionEvent?): Boolean {
        if (event == null) return false

        val actionMasked = event.actionMasked
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)
        val x = event.getX(pointerIndex)
        val y = event.getY(pointerIndex)

        when (actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                nativeOnTouchDown(nativePtr, pointerId, x, y)
            }

            MotionEvent.ACTION_MOVE -> {
                // Tüm aktif pointer'ları gönder
                for (i in 0 until event.pointerCount) {
                    val id = event.getPointerId(i)
                    val px = event.getX(i)
                    val py = event.getY(i)
                    nativeOnTouchMove(nativePtr, id, px, py)
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                nativeOnTouchUp(nativePtr, pointerId, x, y)
            }

            MotionEvent.ACTION_CANCEL -> {
                nativeOnTouchCancel(nativePtr, pointerId)
            }
        }

        return true
    }

    // Native fonksiyonlar (C++ JNI)
    private external fun nativeOnTouchDown(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchMove(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchUp(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchCancel(ptr: Long, id: Int)

    companion object {
        init {
            System.loadLibrary("metin2_mobile")
        }
    }
}

/**
 * Game View - OpenGL ES rendering surface
 */
class GameView(context: android.content.Context) : android.opengl.GLSurfaceView(context) {

    private val renderer: GameRenderer
    private val touchHandler: TouchHandler

    init {
        // OpenGL ES 3.0
        setEGLContextClientVersion(3)

        // Renderer
        renderer = GameRenderer()
        setRenderer(renderer)

        // Touch handler
        touchHandler = TouchHandler(renderer.getNativePtr())
        setOnTouchListener(touchHandler)

        renderMode = RENDERMODE_CONTINUOUSLY
    }

    fun pauseGame() {
        nativePause(renderer.getNativePtr())
    }

    fun resumeGame() {
        nativeResume(renderer.getNativePtr())
    }

    private external fun nativePause(ptr: Long)
    private external fun nativeResume(ptr: Long)
}

/**
 * OpenGL ES Renderer
 */
class GameRenderer : android.opengl.GLSurfaceView.Renderer {

    private var nativePtr: Long = 0

    override fun onSurfaceCreated(gl: javax.microedition.khronos.opengles.GL10?, config: javax.microedition.khronos.egl.EGLConfig?) {
        nativePtr = nativeInit()
    }

    override fun onSurfaceChanged(gl: javax.microedition.khronos.opengles.GL10?, width: Int, height: Int) {
        nativeResize(nativePtr, width, height)
    }

    override fun onDrawFrame(gl: javax.microedition.khronos.opengles.GL10?) {
        nativeRender(nativePtr)
    }

    fun getNativePtr(): Long = nativePtr

    private external fun nativeInit(): Long
    private external fun nativeResize(ptr: Long, width: Int, height: Int)
    private external fun nativeRender(ptr: Long)
}

/**
 * Kullanım örneği
 */
class MainActivity : android.app.Activity() {

    private lateinit var gameView: GameView

    override fun onCreate(savedInstanceState: android.os.Bundle?) {
        super.onCreate(savedInstanceState)

        // Game view oluştur
        gameView = GameView(this)
        setContentView(gameView)

        // Tam ekran
        window.decorView.systemUiVisibility = (
            android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            or android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            or android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
            or android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        )
    }

    override fun onPause() {
        super.onPause()
        gameView.pauseGame()
    }

    override fun onResume() {
        super.onResume()
        gameView.resumeGame()
    }
}
