package com.metin2.pvp.network

import android.util.Log
import okhttp3.*
import okio.ByteString
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.TimeUnit

/**
 * Metin2 PvP Network Client for Android
 * Hibrit protokol: WebSocket + UDP
 */
class NetworkClient(
    private val serverIP: String,
    private val wsPort: Int = 8080,
    private val udpPort: Int = 8081
) {
    companion object {
        private const val TAG = "NetworkClient"

        // Packet types
        const val HEADER_CG_LOGIN: Byte = 2
        const val HEADER_GC_LOGIN_SUCCESS: Byte = 3
        const val HEADER_CG_CHARACTER_SELECT: Byte = 10
        const val HEADER_GC_CHARACTER_INFO: Byte = 11
        const val HEADER_CG_MOVE: Byte = 20
        const val HEADER_GC_MOVE: Byte = 21
        const val HEADER_CG_ATTACK: Byte = 30
        const val HEADER_GC_DAMAGE: Byte = 32
        const val HEADER_CG_CHAT: Byte = 40
        const val HEADER_GC_CHAT: Byte = 41
        const val HEADER_CG_PING: Byte = 50
        const val HEADER_GC_PONG: Byte = 51
    }

    // WebSocket
    private var webSocket: WebSocket? = null
    private val okHttpClient = OkHttpClient.Builder()
        .pingInterval(15, TimeUnit.SECONDS)
        .build()

    // UDP
    private var udpSocket: DatagramSocket? = null
    private var udpThread: Thread? = null
    private var isUDPRunning = false
    private var udpAddress: InetAddress? = null

    // State
    private var isConnected = false
    private var sessionId: Int = 0
    private var characterId: Int = 0
    private var sequenceNumber: Int = 0

    // Callbacks
    var onConnected: (() -> Unit)? = null
    var onDisconnected: ((String) -> Unit)? = null
    var onPacketReceived: ((Byte, ByteArray) -> Unit)? = null
    var onError: ((String) -> Unit)? = null

    /**
     * WebSocket'e bağlan
     */
    fun connectWebSocket() {
        val url = "ws://$serverIP:$wsPort"
        val request = Request.Builder()
            .url(url)
            .build()

        webSocket = okHttpClient.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                Log.i(TAG, "WebSocket connected")
                isConnected = true
                onConnected?.invoke()
            }

            override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
                handleWebSocketMessage(bytes.toByteArray())
            }

            override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                Log.i(TAG, "WebSocket closing: $reason")
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                Log.i(TAG, "WebSocket closed: $reason")
                isConnected = false
                onDisconnected?.invoke(reason)
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                Log.e(TAG, "WebSocket error: ${t.message}")
                isConnected = false
                onError?.invoke(t.message ?: "Unknown error")
            }
        })
    }

    /**
     * UDP başlat
     */
    fun startUDP() {
        try {
            udpSocket = DatagramSocket()
            udpAddress = InetAddress.getByName(serverIP)
            isUDPRunning = true

            // UDP receive thread
            udpThread = Thread {
                val buffer = ByteArray(2048)
                while (isUDPRunning) {
                    try {
                        val packet = DatagramPacket(buffer, buffer.size)
                        udpSocket?.receive(packet)

                        val data = packet.data.copyOfRange(0, packet.length)
                        handleUDPMessage(data)
                    } catch (e: Exception) {
                        if (isUDPRunning) {
                            Log.e(TAG, "UDP receive error: ${e.message}")
                        }
                    }
                }
            }.apply { start() }

            Log.i(TAG, "UDP started on port $udpPort")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to start UDP: ${e.message}")
            onError?.invoke("UDP init failed")
        }
    }

    /**
     * Bağlantıyı kes
     */
    fun disconnect() {
        // WebSocket kapat
        webSocket?.close(1000, "Client disconnect")
        webSocket = null

        // UDP kapat
        isUDPRunning = false
        udpSocket?.close()
        udpThread?.join(1000)

        isConnected = false
        Log.i(TAG, "Disconnected")
    }

    // ========== PACKET SENDING ==========

    /**
     * Login paketi gönder
     */
    fun sendLogin(username: String, password: String) {
        val buffer = ByteBuffer.allocate(256).order(ByteOrder.LITTLE_ENDIAN)

        // Header
        buffer.put(HEADER_CG_LOGIN)
        buffer.putShort((8 + 56).toShort()) // size
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        // Body
        buffer.put(username.toByteArray().copyOf(24))
        buffer.put(password.toByteArray().copyOf(32))

        sendWebSocket(buffer.array(), buffer.position())
    }

    /**
     * Karakter seçim paketi
     */
    fun sendCharacterSelect(charId: Int) {
        val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_CHARACTER_SELECT)
        buffer.putShort((8 + 4).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())
        buffer.putInt(charId)

        characterId = charId
        sendWebSocket(buffer.array(), buffer.position())
    }

    /**
     * Hareket paketi (UDP)
     */
    fun sendMove(x: Int, y: Int, dir: Byte) {
        val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_MOVE)
        buffer.putShort((8 + 9).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.putInt(x)
        buffer.putInt(y)
        buffer.put(dir)

        sendUDP(buffer.array(), buffer.position())
    }

    /**
     * Saldırı paketi (UDP)
     */
    fun sendAttack(targetId: Int) {
        val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_ATTACK)
        buffer.putShort((8 + 6).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.putInt(targetId)
        buffer.put(0) // attack_type
        buffer.putShort(0) // skill_id

        sendUDP(buffer.array(), buffer.position())
    }

    /**
     * Chat paketi
     */
    fun sendChat(message: String) {
        val buffer = ByteBuffer.allocate(512).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_CHAT)
        buffer.putShort((8 + 261).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.put(0) // type
        buffer.putInt(0) // target_id
        buffer.put(message.toByteArray().copyOf(256))

        sendWebSocket(buffer.array(), buffer.position())
    }

    /**
     * Ping paketi
     */
    fun sendPing() {
        val buffer = ByteBuffer.allocate(32).order(ByteOrder.LITTLE_ENDIAN)

        val clientTime = System.currentTimeMillis().toInt()

        buffer.put(HEADER_CG_PING)
        buffer.putShort((8 + 4).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(clientTime)
        buffer.putInt(clientTime)

        sendWebSocket(buffer.array(), buffer.position())
    }

    // ========== LOW-LEVEL SEND ==========

    private fun sendWebSocket(data: ByteArray, size: Int) {
        webSocket?.send(ByteString.of(data, 0, size))
    }

    private fun sendUDP(data: ByteArray, size: Int) {
        try {
            val packet = DatagramPacket(data, size, udpAddress, udpPort)
            udpSocket?.send(packet)
        } catch (e: Exception) {
            Log.e(TAG, "UDP send error: ${e.message}")
        }
    }

    // ========== PACKET HANDLING ==========

    private fun handleWebSocketMessage(data: ByteArray) {
        if (data.size < 8) return

        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val type = buffer.get()
        val size = buffer.short.toInt()
        val sequence = buffer.int
        val timestamp = buffer.int

        Log.d(TAG, "WS Packet: type=$type, size=$size, seq=$sequence")

        val body = ByteArray(size - 8)
        buffer.get(body)

        onPacketReceived?.invoke(type, body)

        // Specific handlers
        when (type) {
            HEADER_GC_LOGIN_SUCCESS -> handleLoginSuccess(body)
            HEADER_GC_CHARACTER_INFO -> handleCharacterInfo(body)
            HEADER_GC_CHAT -> handleChat(body)
            HEADER_GC_PONG -> handlePong(body)
        }
    }

    private fun handleUDPMessage(data: ByteArray) {
        if (data.size < 8) return

        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val type = buffer.get()
        val size = buffer.short.toInt()
        val sequence = buffer.int
        val timestamp = buffer.int

        Log.d(TAG, "UDP Packet: type=$type, size=$size")

        val body = ByteArray(size - 8)
        buffer.get(body)

        onPacketReceived?.invoke(type, body)

        // Specific handlers
        when (type) {
            HEADER_GC_MOVE -> handleMove(body)
            HEADER_GC_DAMAGE -> handleDamage(body)
        }
    }

    private fun handleLoginSuccess(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val accountId = buffer.int
        val charCount = buffer.get()

        Log.i(TAG, "Login success! Account: $accountId, Characters: $charCount")
    }

    private fun handleCharacterInfo(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val id = buffer.int
        val name = String(ByteArray(24).apply { buffer.get(this) }).trim('\u0000')
        val job = buffer.get()
        val level = buffer.get()
        val hp = buffer.int
        val maxHp = buffer.int

        Log.i(TAG, "Character: $name (ID:$id) Lv.$level HP:$hp/$maxHp")
    }

    private fun handleChat(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val senderId = buffer.int
        val senderName = String(ByteArray(24).apply { buffer.get(this) }).trim('\u0000')
        val type = buffer.get()
        val message = String(ByteArray(256).apply { buffer.get(this) }).trim('\u0000')

        Log.i(TAG, "[CHAT] $senderName: $message")
    }

    private fun handlePong(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val clientTime = buffer.int
        val serverTime = buffer.int
        val latency = System.currentTimeMillis().toInt() - clientTime

        Log.d(TAG, "Pong received. Latency: ${latency}ms")
    }

    private fun handleMove(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val id = buffer.int
        val x = buffer.int
        val y = buffer.int
        val dir = buffer.get()

        Log.d(TAG, "Player $id moved to ($x, $y)")
    }

    private fun handleDamage(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val attackerId = buffer.int
        val victimId = buffer.int
        val damage = buffer.int
        val isCritical = buffer.get()

        Log.i(TAG, "Damage: $attackerId -> $victimId = $damage${if (isCritical.toInt() == 1) " CRIT!" else ""}")
    }
}

/**
 * Kullanım örneği
 */
class NetworkExample {
    fun example() {
        val client = NetworkClient("192.168.1.100")

        client.onConnected = {
            println("Bağlandı!")
            client.sendLogin("testuser", "password123")
        }

        client.onDisconnected = { reason ->
            println("Bağlantı kesildi: $reason")
        }

        client.onPacketReceived = { type, data ->
            println("Paket alındı: type=$type, size=${data.size}")
        }

        client.connectWebSocket()
        client.startUDP()

        // Hareket gönderme örneği
        Thread {
            Thread.sleep(2000)
            client.sendMove(957300, 245000, 0)
        }.start()

        // Ping gönderme örneği
        Thread {
            while (true) {
                Thread.sleep(5000)
                client.sendPing()
            }
        }.start()
    }
}
