package com.metin2.pvp.network

import android.util.Log
import okhttp3.*
import okio.ByteString
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.PriorityBlockingQueue
import java.util.concurrent.TimeUnit
import kotlin.math.max
import kotlin.math.min

/**
 * Optimize edilmiş Metin2 Network Client
 *
 * Optimizasyonlar:
 * - Client-side prediction
 * - Server reconciliation
 * - Entity interpolation
 * - Delta compression
 * - Adaptive rate control
 * - Packet prioritization
 * - Connection quality monitoring
 */
class OptimizedNetworkClient(
    private val serverIP: String,
    private val wsPort: Int = 8080,
    private val udpPort: Int = 8081
) {
    companion object {
        private const val TAG = "OptimizedNetwork"

        // Packet types
        const val HEADER_CG_LOGIN: Byte = 2
        const val HEADER_GC_LOGIN_SUCCESS: Byte = 3
        const val HEADER_CG_MOVE: Byte = 20
        const val HEADER_GC_MOVE: Byte = 21
        const val HEADER_CG_ATTACK: Byte = 30
        const val HEADER_GC_DAMAGE: Byte = 32
        const val HEADER_CG_PING: Byte = 50
        const val HEADER_GC_PONG: Byte = 51

        // Optimizasyon sabitleri
        const val MAX_PREDICTION_TIME_MS = 200  // Maksimum client prediction süresi
        const val INTERPOLATION_DELAY_MS = 100   // Interpolasyon gecikmesi
        const val POSITION_TOLERANCE = 100       // Pozisyon farkı toleransı (cm)
        const val MIN_UPDATE_INTERVAL_MS = 50    // Minimum güncelleme aralığı
        const val MAX_UPDATE_INTERVAL_MS = 200   // Maksimum güncelleme aralığı
    }

    // Network bileşenleri
    private var webSocket: WebSocket? = null
    private val okHttpClient = OkHttpClient.Builder()
        .pingInterval(15, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .writeTimeout(10, TimeUnit.SECONDS)
        .build()

    private var udpSocket: DatagramSocket? = null
    private var udpThread: Thread? = null
    private var isUDPRunning = false
    private var udpAddress: InetAddress? = null

    // State
    private var isConnected = false
    private var sessionId: Int = 0
    private var characterId: Int = 0
    private var sequenceNumber: Int = 0

    // Network metrics
    private val networkMetrics = NetworkMetrics()
    private var lastPingTime = 0L
    private var currentLatency = 0
    private var jitter = 0
    private var packetLoss = 0f

    // Adaptive rate control
    private var currentUpdateInterval = MIN_UPDATE_INTERVAL_MS
    private var lastUpdateTime = 0L

    // Client-side prediction
    private val predictedStates = ConcurrentHashMap<Int, PredictedState>()
    private val pendingInputs = mutableListOf<PlayerInput>()

    // Entity interpolation
    private val entityStates = ConcurrentHashMap<Int, EntityStateBuffer>()

    // Delta compression
    private val lastSentPositions = ConcurrentHashMap<Int, Position>()

    // Packet prioritization
    private val priorityQueue = PriorityBlockingQueue<PrioritizedPacket>()
    private var packetSenderThread: Thread? = null

    // Callbacks
    var onConnected: (() -> Unit)? = null
    var onDisconnected: ((String) -> Unit)? = null
    var onPacketReceived: ((Byte, ByteArray) -> Unit)? = null
    var onError: ((String) -> Unit)? = null
    var onLatencyUpdate: ((Int) -> Unit)? = null

    /**
     * Network metrikleri
     */
    data class NetworkMetrics(
        var packetsSent: Long = 0,
        var packetsReceived: Long = 0,
        var packetsLost: Long = 0,
        var bytesSent: Long = 0,
        var bytesReceived: Long = 0,
        var avgLatency: Int = 0,
        var minLatency: Int = Int.MAX_VALUE,
        var maxLatency: Int = 0
    )

    /**
     * Client-side prediction state
     */
    data class PredictedState(
        val sequenceNumber: Int,
        val timestamp: Long,
        val x: Int,
        val y: Int,
        val velocityX: Float,
        val velocityY: Float
    )

    /**
     * Player input için
     */
    data class PlayerInput(
        val sequenceNumber: Int,
        val timestamp: Long,
        val x: Int,
        val y: Int,
        val dir: Byte
    )

    /**
     * Pozisyon bilgisi
     */
    data class Position(
        val x: Int,
        val y: Int,
        val dir: Byte = 0,
        val timestamp: Long = System.currentTimeMillis()
    )

    /**
     * Entity state buffer (interpolasyon için)
     */
    class EntityStateBuffer {
        private val states = mutableListOf<TimestampedPosition>()
        private val maxStates = 10

        data class TimestampedPosition(
            val x: Int,
            val y: Int,
            val dir: Byte,
            val timestamp: Long
        )

        fun addState(x: Int, y: Int, dir: Byte, timestamp: Long) {
            states.add(TimestampedPosition(x, y, dir, timestamp))
            if (states.size > maxStates) {
                states.removeAt(0)
            }
        }

        fun getInterpolatedPosition(renderTime: Long): Position? {
            if (states.size < 2) return states.lastOrNull()?.let {
                Position(it.x, it.y, it.dir, it.timestamp)
            }

            // Interpolation: renderTime'dan INTERPOLATION_DELAY_MS öncesindeki pozisyonu al
            val targetTime = renderTime - INTERPOLATION_DELAY_MS

            // targetTime için uygun iki state bul
            var before: TimestampedPosition? = null
            var after: TimestampedPosition? = null

            for (i in 0 until states.size - 1) {
                if (states[i].timestamp <= targetTime && states[i + 1].timestamp >= targetTime) {
                    before = states[i]
                    after = states[i + 1]
                    break
                }
            }

            if (before == null || after == null) {
                return states.lastOrNull()?.let {
                    Position(it.x, it.y, it.dir, it.timestamp)
                }
            }

            // Linear interpolation
            val t = (targetTime - before.timestamp).toFloat() /
                    (after.timestamp - before.timestamp).toFloat()

            val x = (before.x + (after.x - before.x) * t).toInt()
            val y = (before.y + (after.y - before.y) * t).toInt()

            return Position(x, y, after.dir, targetTime)
        }
    }

    /**
     * Öncelikli paket
     */
    data class PrioritizedPacket(
        val priority: Int,
        val data: ByteArray,
        val size: Int,
        val timestamp: Long = System.currentTimeMillis()
    ) : Comparable<PrioritizedPacket> {
        override fun compareTo(other: PrioritizedPacket): Int {
            // Yüksek priority önce (ters sıralama)
            return other.priority.compareTo(this.priority)
        }
    }

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
                startPacketSender()
                startPingMonitor()
                onConnected?.invoke()
            }

            override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
                networkMetrics.packetsReceived++
                networkMetrics.bytesReceived += bytes.size
                handleWebSocketMessage(bytes.toByteArray())
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

            udpThread = Thread {
                val buffer = ByteArray(2048)
                var lastSeq = 0

                while (isUDPRunning) {
                    try {
                        val packet = DatagramPacket(buffer, buffer.size)
                        udpSocket?.receive(packet)

                        networkMetrics.packetsReceived++
                        networkMetrics.bytesReceived += packet.length

                        val data = packet.data.copyOfRange(0, packet.length)

                        // Packet loss detection
                        val seq = extractSequence(data)
                        if (seq > lastSeq + 1) {
                            networkMetrics.packetsLost += (seq - lastSeq - 1)
                            updatePacketLoss()
                        }
                        lastSeq = seq

                        handleUDPMessage(data)
                    } catch (e: Exception) {
                        if (isUDPRunning) {
                            Log.e(TAG, "UDP receive error: ${e.message}")
                        }
                    }
                }
            }.apply { start() }

            Log.i(TAG, "UDP started")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to start UDP: ${e.message}")
            onError?.invoke("UDP init failed")
        }
    }

    /**
     * Öncelikli paket gönderici thread
     */
    private fun startPacketSender() {
        packetSenderThread = Thread {
            while (isConnected) {
                try {
                    val packet = priorityQueue.poll(10, TimeUnit.MILLISECONDS)
                    if (packet != null) {
                        sendUDPRaw(packet.data, packet.size)
                        networkMetrics.packetsSent++
                        networkMetrics.bytesSent += packet.size
                    }
                } catch (e: InterruptedException) {
                    break
                } catch (e: Exception) {
                    Log.e(TAG, "Packet sender error: ${e.message}")
                }
            }
        }.apply { start() }
    }

    /**
     * Ping monitor (latency tracking)
     */
    private fun startPingMonitor() {
        Thread {
            while (isConnected) {
                Thread.sleep(1000)
                sendPing()
            }
        }.start()
    }

    /**
     * Disconnect
     */
    fun disconnect() {
        webSocket?.close(1000, "Client disconnect")
        webSocket = null

        isUDPRunning = false
        udpSocket?.close()
        udpThread?.join(1000)

        isConnected = false
        packetSenderThread?.interrupt()

        Log.i(TAG, "Disconnected")
    }

    // ========== OPTIMIZED PACKET SENDING ==========

    /**
     * Login (WebSocket - güvenilir)
     */
    fun sendLogin(username: String, password: String) {
        val buffer = ByteBuffer.allocate(256).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_LOGIN)
        buffer.putShort((8 + 56).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.put(username.toByteArray().copyOf(24))
        buffer.put(password.toByteArray().copyOf(32))

        sendWebSocketRaw(buffer.array(), buffer.position())
    }

    /**
     * Move with client-side prediction ve delta compression
     */
    fun sendMoveOptimized(x: Int, y: Int, dir: Byte) {
        val now = System.currentTimeMillis()

        // Adaptive rate control
        if (now - lastUpdateTime < currentUpdateInterval) {
            // Henüz göndermek için erken, local olarak predict et
            applyClientSidePrediction(x, y, dir)
            return
        }

        lastUpdateTime = now

        // Delta compression: sadece değişiklikleri gönder
        val lastPos = lastSentPositions[characterId]

        val buffer = if (lastPos != null && shouldUseDelta(x, y, lastPos)) {
            // Delta encoding
            createDeltaMovePacket(x, y, dir, lastPos)
        } else {
            // Full position
            createFullMovePacket(x, y, dir)
        }

        // Client-side prediction: locally apply immediately
        applyClientSidePrediction(x, y, dir)

        // Store input for reconciliation
        val input = PlayerInput(sequenceNumber, now, x, y, dir)
        pendingInputs.add(input)
        if (pendingInputs.size > 20) {
            pendingInputs.removeAt(0)
        }

        // Save last position
        lastSentPositions[characterId] = Position(x, y, dir, now)

        // Send with priority
        queuePacket(buffer.array(), buffer.position(), priority = 2) // HIGH priority

        // Adaptive rate adjustment
        adjustUpdateRate()
    }

    /**
     * Delta move packet oluştur (daha küçük)
     */
    private fun createDeltaMovePacket(x: Int, y: Int, dir: Byte, lastPos: Position): ByteBuffer {
        val buffer = ByteBuffer.allocate(32).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put((HEADER_CG_MOVE.toInt() or 0x80).toByte()) // Delta flag
        buffer.putShort((8 + 5).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        // Delta değerler (16-bit yeterli)
        buffer.putShort((x - lastPos.x).toShort())
        buffer.putShort((y - lastPos.y).toShort())
        buffer.put(dir)

        return buffer
    }

    /**
     * Full move packet
     */
    private fun createFullMovePacket(x: Int, y: Int, dir: Byte): ByteBuffer {
        val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_MOVE)
        buffer.putShort((8 + 9).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.putInt(x)
        buffer.putInt(y)
        buffer.put(dir)

        return buffer
    }

    /**
     * Delta kullanılmalı mı?
     */
    private fun shouldUseDelta(x: Int, y: Int, lastPos: Position): Boolean {
        val dx = Math.abs(x - lastPos.x)
        val dy = Math.abs(y - lastPos.y)
        // Delta 16-bit'e sığıyorsa kullan
        return dx < 32768 && dy < 32768
    }

    /**
     * Client-side prediction uygula
     */
    private fun applyClientSidePrediction(x: Int, y: Int, dir: Byte) {
        val now = System.currentTimeMillis()
        val predicted = PredictedState(
            sequenceNumber = sequenceNumber,
            timestamp = now,
            x = x,
            y = y,
            velocityX = 0f, // TODO: velocity hesapla
            velocityY = 0f
        )
        predictedStates[characterId] = predicted
    }

    /**
     * Server reconciliation (sunucu pozisyonunu aldığında)
     */
    private fun reconcileWithServer(serverX: Int, serverY: Int, serverSeq: Int) {
        val predicted = predictedStates[characterId] ?: return

        // Pozisyon farkını hesapla
        val dx = Math.abs(predicted.x - serverX)
        val dy = Math.abs(predicted.y - serverY)
        val distance = Math.sqrt((dx * dx + dy * dy).toDouble()).toInt()

        if (distance > POSITION_TOLERANCE) {
            Log.w(TAG, "Position mismatch! Predicted: (${predicted.x}, ${predicted.y}), " +
                      "Server: ($serverX, $serverY), Distance: $distance")

            // Sunucu pozisyonunu kabul et
            predictedStates[characterId] = predicted.copy(x = serverX, y = serverY)

            // Pending input'ları yeniden uygula (server sequence'dan sonrakiler)
            replayInputs(serverSeq)
        }
    }

    /**
     * Pending input'ları yeniden uygula
     */
    private fun replayInputs(afterSequence: Int) {
        val toReplay = pendingInputs.filter { it.sequenceNumber > afterSequence }
        for (input in toReplay) {
            // Input'u yeniden uygula (basitleştirilmiş)
            // Gerçek uygulamada movement logic'i tekrar çalıştırılır
        }
    }

    /**
     * Attack (UDP - düşük gecikme)
     */
    fun sendAttack(targetId: Int) {
        val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put(HEADER_CG_ATTACK)
        buffer.putShort((8 + 5).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(System.currentTimeMillis().toInt())

        buffer.putInt(targetId)
        buffer.put(0)

        queuePacket(buffer.array(), buffer.position(), priority = 3) // CRITICAL
    }

    /**
     * Ping
     */
    private fun sendPing() {
        val buffer = ByteBuffer.allocate(32).order(ByteOrder.LITTLE_ENDIAN)
        val clientTime = System.currentTimeMillis()

        buffer.put(HEADER_CG_PING)
        buffer.putShort((8 + 4).toShort())
        buffer.putInt(++sequenceNumber)
        buffer.putInt(clientTime.toInt())
        buffer.putInt(clientTime.toInt())

        lastPingTime = clientTime
        sendWebSocketRaw(buffer.array(), buffer.position())
    }

    /**
     * Paketi priority queue'ya ekle
     */
    private fun queuePacket(data: ByteArray, size: Int, priority: Int) {
        priorityQueue.offer(PrioritizedPacket(priority, data, size))
    }

    // ========== LOW-LEVEL SEND ==========

    private fun sendWebSocketRaw(data: ByteArray, size: Int) {
        webSocket?.send(ByteString.of(data, 0, size))
        networkMetrics.packetsSent++
        networkMetrics.bytesSent += size
    }

    private fun sendUDPRaw(data: ByteArray, size: Int) {
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

        val body = ByteArray(size - 8)
        buffer.get(body)

        onPacketReceived?.invoke(type, body)

        when (type) {
            HEADER_GC_LOGIN_SUCCESS -> handleLoginSuccess(body)
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

        val body = ByteArray(size - 8)
        buffer.get(body)

        onPacketReceived?.invoke(type, body)

        when (type) {
            HEADER_GC_MOVE -> handleMoveWithInterpolation(body, timestamp.toLong())
            HEADER_GC_DAMAGE -> handleDamage(body)
        }
    }

    private fun handleLoginSuccess(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val accountId = buffer.int

        Log.i(TAG, "Login success! Account: $accountId")
    }

    private fun handlePong(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val clientTime = buffer.int
        val serverTime = buffer.int

        val now = System.currentTimeMillis().toInt()
        val latency = now - clientTime

        // Latency metrics güncelle
        updateLatencyMetrics(latency)

        currentLatency = latency
        onLatencyUpdate?.invoke(latency)

        Log.d(TAG, "Latency: ${latency}ms, Jitter: ${jitter}ms, Loss: ${String.format("%.2f", packetLoss)}%")
    }

    /**
     * Move ile interpolation
     */
    private fun handleMoveWithInterpolation(data: ByteArray, serverTimestamp: Long) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val id = buffer.int
        val x = buffer.int
        val y = buffer.int
        val dir = buffer.get()

        // Entity state buffer'a ekle
        var stateBuffer = entityStates[id]
        if (stateBuffer == null) {
            stateBuffer = EntityStateBuffer()
            entityStates[id] = stateBuffer
        }

        stateBuffer.addState(x, y, dir, System.currentTimeMillis())

        // Eğer bu kendi karakterimizse, reconciliation yap
        if (id == characterId) {
            val sequence = extractSequence(buffer.array())
            reconcileWithServer(x, y, sequence)
        }
    }

    private fun handleDamage(data: ByteArray) {
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val attackerId = buffer.int
        val victimId = buffer.int
        val damage = buffer.int

        Log.i(TAG, "Damage: $attackerId -> $victimId = $damage")
    }

    /**
     * Interpolated pozisyon al (rendering için)
     */
    fun getInterpolatedPosition(entityId: Int): Position? {
        val stateBuffer = entityStates[entityId] ?: return null
        return stateBuffer.getInterpolatedPosition(System.currentTimeMillis())
    }

    // ========== ADAPTIVE OPTIMIZATION ==========

    /**
     * Update rate'i network kalitesine göre ayarla
     */
    private fun adjustUpdateRate() {
        currentUpdateInterval = when {
            packetLoss > 5.0f -> MAX_UPDATE_INTERVAL_MS // Kötü: daha az paket
            currentLatency > 150 -> (MAX_UPDATE_INTERVAL_MS + MIN_UPDATE_INTERVAL_MS) / 2
            currentLatency < 50 -> MIN_UPDATE_INTERVAL_MS // İyi: smooth updates
            else -> 100
        }
    }

    /**
     * Latency metrics güncelle
     */
    private fun updateLatencyMetrics(latency: Int) {
        networkMetrics.avgLatency =
            ((networkMetrics.avgLatency * 9) + latency) / 10 // Moving average

        networkMetrics.minLatency = min(networkMetrics.minLatency, latency)
        networkMetrics.maxLatency = max(networkMetrics.maxLatency, latency)

        // Jitter hesapla
        jitter = Math.abs(latency - networkMetrics.avgLatency)
    }

    /**
     * Packet loss oranını güncelle
     */
    private fun updatePacketLoss() {
        if (networkMetrics.packetsSent > 0) {
            packetLoss = (networkMetrics.packetsLost.toFloat() /
                         networkMetrics.packetsSent.toFloat()) * 100f
        }
    }

    /**
     * Sequence numarasını paketin içinden çıkar
     */
    private fun extractSequence(data: ByteArray): Int {
        if (data.size < 8) return 0
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        buffer.position(3) // type(1) + size(2) = 3
        return buffer.int
    }

    /**
     * Network metrikleri al
     */
    fun getMetrics(): NetworkMetrics = networkMetrics.copy()

    /**
     * Bağlantı kalitesi raporu
     */
    fun getConnectionQuality(): String {
        return when {
            packetLoss > 5.0f || currentLatency > 200 -> "Poor"
            packetLoss > 2.0f || currentLatency > 100 -> "Fair"
            currentLatency < 50 -> "Excellent"
            else -> "Good"
        }
    }
}
