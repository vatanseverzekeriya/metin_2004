import Foundation
import Network
import Starscream

/**
 * Optimize edilmiş iOS Network Client
 *
 * Özellikler:
 * - Client-side prediction
 * - Server reconciliation
 * - Entity interpolation
 * - Delta compression
 * - Adaptive update rate
 * - Connection quality monitoring
 */
class OptimizedNetworkClient {

    // MARK: - Constants

    enum PacketType: UInt8 {
        case cgLogin = 2
        case gcLoginSuccess = 3
        case cgMove = 20
        case gcMove = 21
        case cgAttack = 30
        case gcDamage = 32
        case cgPing = 50
        case gcPong = 51
    }

    private static let MAX_PREDICTION_TIME_MS: Int64 = 200
    private static let INTERPOLATION_DELAY_MS: Int64 = 100
    private static let POSITION_TOLERANCE: Int32 = 100
    private static let MIN_UPDATE_INTERVAL_MS: Int64 = 50
    private static let MAX_UPDATE_INTERVAL_MS: Int64 = 200

    // MARK: - Properties

    private let serverIP: String
    private let wsPort: Int
    private let udpPort: Int

    // Network components
    private var webSocket: WebSocket?
    private var udpConnection: NWConnection?
    private var udpQueue = DispatchQueue(label: "com.metin2.udp", qos: .userInteractive)

    // State
    private var isConnected = false
    private var sessionId: Int32 = 0
    private var characterId: Int32 = 0
    private var sequenceNumber: Int32 = 0

    // Network metrics
    private var networkMetrics = NetworkMetrics()
    private var lastPingTime: Int64 = 0
    private var currentLatency: Int = 0
    private var jitter: Int = 0
    private var packetLoss: Float = 0.0

    // Adaptive rate control
    private var currentUpdateInterval: Int64 = 50
    private var lastUpdateTime: Int64 = 0

    // Client-side prediction
    private var predictedStates = [Int32: PredictedState]()
    private var pendingInputs = [PlayerInput]()

    // Entity interpolation
    private var entityStates = [Int32: EntityStateBuffer]()

    // Delta compression
    private var lastSentPositions = [Int32: Position]()

    // Packet prioritization
    private var priorityQueue = PriorityQueue<PrioritizedPacket>()
    private var packetSenderQueue = DispatchQueue(label: "com.metin2.sender", qos: .userInteractive)
    private var isSenderRunning = false

    // Callbacks
    var onConnected: (() -> Void)?
    var onDisconnected: ((String) -> Void)?
    var onPacketReceived: ((UInt8, Data) -> Void)?
    var onError: ((String) -> Void)?
    var onLatencyUpdate: ((Int) -> Void)?

    // MARK: - Data Structures

    struct NetworkMetrics {
        var packetsSent: Int64 = 0
        var packetsReceived: Int64 = 0
        var packetsLost: Int64 = 0
        var bytesSent: Int64 = 0
        var bytesReceived: Int64 = 0
        var avgLatency: Int = 0
        var minLatency: Int = Int.max
        var maxLatency: Int = 0
    }

    struct PredictedState {
        let sequenceNumber: Int32
        let timestamp: Int64
        let x: Int32
        let y: Int32
        let velocityX: Float
        let velocityY: Float
    }

    struct PlayerInput {
        let sequenceNumber: Int32
        let timestamp: Int64
        let x: Int32
        let y: Int32
        let dir: UInt8
    }

    struct Position {
        let x: Int32
        let y: Int32
        let dir: UInt8
        let timestamp: Int64
    }

    struct PrioritizedPacket: Comparable {
        let priority: Int
        let data: Data
        let timestamp: Int64

        static func < (lhs: PrioritizedPacket, rhs: PrioritizedPacket) -> Bool {
            // Higher priority first
            return lhs.priority > rhs.priority
        }
    }

    class EntityStateBuffer {
        struct TimestampedPosition {
            let x: Int32
            let y: Int32
            let dir: UInt8
            let timestamp: Int64
        }

        private var states = [TimestampedPosition]()
        private let maxStates = 10
        private let lock = NSLock()

        func addState(x: Int32, y: Int32, dir: UInt8, timestamp: Int64) {
            lock.lock()
            defer { lock.unlock() }

            states.append(TimestampedPosition(x: x, y: y, dir: dir, timestamp: timestamp))
            if states.count > maxStates {
                states.removeFirst()
            }
        }

        func getInterpolatedPosition(renderTime: Int64) -> Position? {
            lock.lock()
            defer { lock.unlock() }

            guard states.count >= 2 else {
                return states.last.map { Position(x: $0.x, y: $0.y, dir: $0.dir, timestamp: $0.timestamp) }
            }

            // Interpolation: renderTime - INTERPOLATION_DELAY_MS
            let targetTime = renderTime - OptimizedNetworkClient.INTERPOLATION_DELAY_MS

            // Find two states around targetTime
            var before: TimestampedPosition?
            var after: TimestampedPosition?

            for i in 0..<(states.count - 1) {
                if states[i].timestamp <= targetTime && states[i + 1].timestamp >= targetTime {
                    before = states[i]
                    after = states[i + 1]
                    break
                }
            }

            guard let beforeState = before, let afterState = after else {
                return states.last.map { Position(x: $0.x, y: $0.y, dir: $0.dir, timestamp: $0.timestamp) }
            }

            // Linear interpolation
            let timeDiff = afterState.timestamp - beforeState.timestamp
            guard timeDiff > 0 else {
                return Position(x: afterState.x, y: afterState.y, dir: afterState.dir, timestamp: targetTime)
            }

            let t = Float(targetTime - beforeState.timestamp) / Float(timeDiff)

            let x = Int32(Float(beforeState.x) + Float(afterState.x - beforeState.x) * t)
            let y = Int32(Float(beforeState.y) + Float(afterState.y - beforeState.y) * t)

            return Position(x: x, y: y, dir: afterState.dir, timestamp: targetTime)
        }
    }

    // MARK: - Initialization

    init(serverIP: String, wsPort: Int = 8080, udpPort: Int = 8081) {
        self.serverIP = serverIP
        self.wsPort = wsPort
        self.udpPort = udpPort
    }

    // MARK: - Connection

    func connectWebSocket() {
        let url = URL(string: "ws://\(serverIP):\(wsPort)")!
        var request = URLRequest(url: url)
        request.timeoutInterval = 10

        webSocket = WebSocket(request: request)

        webSocket?.onEvent = { [weak self] event in
            self?.handleWebSocketEvent(event)
        }

        webSocket?.connect()
    }

    func startUDP() {
        let host = NWEndpoint.Host(serverIP)
        let port = NWEndpoint.Port(integerLiteral: UInt16(udpPort))

        udpConnection = NWConnection(host: host, port: port, using: .udp)

        udpConnection?.stateUpdateHandler = { [weak self] state in
            switch state {
            case .ready:
                print("[UDP] Connected")
                self?.startUDPReceive()
            case .failed(let error):
                print("[UDP] Failed: \(error)")
                self?.onError?("UDP connection failed")
            default:
                break
            }
        }

        udpConnection?.start(queue: udpQueue)
    }

    private func startUDPReceive() {
        udpConnection?.receiveMessage { [weak self] data, context, isComplete, error in
            guard let self = self, let data = data else {
                self?.startUDPReceive() // Continue listening
                return
            }

            self.networkMetrics.packetsReceived += 1
            self.networkMetrics.bytesReceived += Int64(data.count)

            self.handleUDPMessage(data)
            self.startUDPReceive() // Continue listening
        }
    }

    func disconnect() {
        webSocket?.disconnect()
        webSocket = nil

        udpConnection?.cancel()
        udpConnection = nil

        isConnected = false
        isSenderRunning = false
    }

    // MARK: - WebSocket Event Handling

    private func handleWebSocketEvent(_ event: WebSocketEvent) {
        switch event {
        case .connected(_):
            print("[WS] Connected")
            isConnected = true
            startPacketSender()
            startPingMonitor()
            onConnected?()

        case .disconnected(let reason, let code):
            print("[WS] Disconnected: \(reason) (\(code))")
            isConnected = false
            onDisconnected?(reason)

        case .binary(let data):
            networkMetrics.packetsReceived += 1
            networkMetrics.bytesReceived += Int64(data.count)
            handleWebSocketMessage(data)

        case .text(_):
            break

        case .error(let error):
            print("[WS] Error: \(error?.localizedDescription ?? "unknown")")
            onError?(error?.localizedDescription ?? "WebSocket error")

        default:
            break
        }
    }

    // MARK: - Packet Sender

    private func startPacketSender() {
        guard !isSenderRunning else { return }
        isSenderRunning = true

        packetSenderQueue.async { [weak self] in
            while self?.isSenderRunning == true {
                if let packet = self?.priorityQueue.dequeue() {
                    self?.sendUDPRaw(packet.data)
                    self?.networkMetrics.packetsSent += 1
                    self?.networkMetrics.bytesSent += Int64(packet.data.count)
                }
                usleep(1000) // 1ms
            }
        }
    }

    private func startPingMonitor() {
        DispatchQueue.global(qos: .utility).async { [weak self] in
            while self?.isConnected == true {
                Thread.sleep(forTimeInterval: 1.0)
                self?.sendPing()
            }
        }
    }

    // MARK: - Optimized Packet Sending

    func sendLogin(username: String, password: String) {
        var data = Data()

        // Header
        data.append(PacketType.cgLogin.rawValue)
        data.append(UInt16(8 + 56).littleEndianData)
        sequenceNumber += 1
        data.append(sequenceNumber.littleEndianData)
        data.append(getCurrentTimeMs().littleEndianData)

        // Body
        data.append(username.paddedData(to: 24))
        data.append(password.paddedData(to: 32))

        sendWebSocketRaw(data)
    }

    func sendMoveOptimized(x: Int32, y: Int32, dir: UInt8) {
        let now = getCurrentTimeMs()

        // Adaptive rate control
        if now - lastUpdateTime < currentUpdateInterval {
            applyClientSidePrediction(x: x, y: y, dir: dir)
            return
        }

        lastUpdateTime = now

        // Delta compression
        let lastPos = lastSentPositions[characterId]
        let data: Data

        if let last = lastPos, shouldUseDelta(x: x, y: y, lastPos: last) {
            data = createDeltaMovePacket(x: x, y: y, dir: dir, lastPos: last)
        } else {
            data = createFullMovePacket(x: x, y: y, dir: dir)
        }

        // Client-side prediction
        applyClientSidePrediction(x: x, y: y, dir: dir)

        // Store input
        let input = PlayerInput(sequenceNumber: sequenceNumber, timestamp: now,
                               x: x, y: y, dir: dir)
        pendingInputs.append(input)
        if pendingInputs.count > 20 {
            pendingInputs.removeFirst()
        }

        // Save last position
        lastSentPositions[characterId] = Position(x: x, y: y, dir: dir, timestamp: now)

        // Queue with priority
        queuePacket(data, priority: 2) // HIGH

        // Adaptive rate adjustment
        adjustUpdateRate()
    }

    private func createDeltaMovePacket(x: Int32, y: Int32, dir: UInt8, lastPos: Position) -> Data {
        var data = Data()

        data.append(UInt8(PacketType.cgMove.rawValue | 0x80)) // Delta flag
        data.append(UInt16(8 + 5).littleEndianData)
        sequenceNumber += 1
        data.append(sequenceNumber.littleEndianData)
        data.append(getCurrentTimeMs().littleEndianData)

        // Delta (16-bit)
        data.append(Int16(x - lastPos.x).littleEndianData)
        data.append(Int16(y - lastPos.y).littleEndianData)
        data.append(dir)

        return data
    }

    private func createFullMovePacket(x: Int32, y: Int32, dir: UInt8) -> Data {
        var data = Data()

        data.append(PacketType.cgMove.rawValue)
        data.append(UInt16(8 + 9).littleEndianData)
        sequenceNumber += 1
        data.append(sequenceNumber.littleEndianData)
        data.append(getCurrentTimeMs().littleEndianData)

        data.append(x.littleEndianData)
        data.append(y.littleEndianData)
        data.append(dir)

        return data
    }

    private func shouldUseDelta(x: Int32, y: Int32, lastPos: Position) -> Bool {
        let dx = abs(x - lastPos.x)
        let dy = abs(y - lastPos.y)
        return dx < 32768 && dy < 32768
    }

    func sendAttack(targetId: Int32) {
        var data = Data()

        data.append(PacketType.cgAttack.rawValue)
        data.append(UInt16(8 + 5).littleEndianData)
        sequenceNumber += 1
        data.append(sequenceNumber.littleEndianData)
        data.append(getCurrentTimeMs().littleEndianData)

        data.append(targetId.littleEndianData)
        data.append(UInt8(0))

        queuePacket(data, priority: 3) // CRITICAL
    }

    private func sendPing() {
        var data = Data()
        let clientTime = getCurrentTimeMs()

        data.append(PacketType.cgPing.rawValue)
        data.append(UInt16(8 + 4).littleEndianData)
        sequenceNumber += 1
        data.append(sequenceNumber.littleEndianData)
        data.append(clientTime.littleEndianData)
        data.append(clientTime.littleEndianData)

        lastPingTime = Int64(clientTime)
        sendWebSocketRaw(data)
    }

    // MARK: - Client-Side Prediction

    private func applyClientSidePrediction(x: Int32, y: Int32, dir: UInt8) {
        let now = getCurrentTimeMs()
        let predicted = PredictedState(
            sequenceNumber: sequenceNumber,
            timestamp: Int64(now),
            x: x,
            y: y,
            velocityX: 0,
            velocityY: 0
        )
        predictedStates[characterId] = predicted
    }

    private func reconcileWithServer(serverX: Int32, serverY: Int32, serverSeq: Int32) {
        guard let predicted = predictedStates[characterId] else { return }

        let dx = abs(predicted.x - serverX)
        let dy = abs(predicted.y - serverY)
        let distance = Int32(sqrt(Double(dx * dx + dy * dy)))

        if distance > Self.POSITION_TOLERANCE {
            print("[Reconciliation] Mismatch: predicted(\(predicted.x),\(predicted.y)) " +
                  "server(\(serverX),\(serverY)) distance=\(distance)")

            // Accept server position
            predictedStates[characterId] = PredictedState(
                sequenceNumber: predicted.sequenceNumber,
                timestamp: predicted.timestamp,
                x: serverX,
                y: serverY,
                velocityX: predicted.velocityX,
                velocityY: predicted.velocityY
            )

            // Replay inputs after server sequence
            replayInputs(afterSequence: serverSeq)
        }
    }

    private func replayInputs(afterSequence: Int32) {
        let toReplay = pendingInputs.filter { $0.sequenceNumber > afterSequence }
        // In real implementation, replay movement logic
        for _ in toReplay {
            // Replay input
        }
    }

    // MARK: - Low-Level Send

    private func sendWebSocketRaw(_ data: Data) {
        webSocket?.write(data: data)
        networkMetrics.packetsSent += 1
        networkMetrics.bytesSent += Int64(data.count)
    }

    private func sendUDPRaw(_ data: Data) {
        udpConnection?.send(content: data, completion: .contentProcessed({ error in
            if let error = error {
                print("[UDP] Send error: \(error)")
            }
        }))
    }

    private func queuePacket(_ data: Data, priority: Int) {
        let packet = PrioritizedPacket(priority: priority, data: data,
                                      timestamp: Int64(getCurrentTimeMs()))
        priorityQueue.enqueue(packet)
    }

    // MARK: - Message Handling

    private func handleWebSocketMessage(_ data: Data) {
        guard data.count >= 8 else { return }

        var offset = 0
        let type = data[offset]; offset += 1
        let size = data.readUInt16(at: offset); offset += 2
        let sequence = data.readInt32(at: offset); offset += 4
        let timestamp = data.readInt32(at: offset); offset += 4

        let body = data.subdata(in: offset..<data.count)

        onPacketReceived?(type, body)

        switch PacketType(rawValue: type) {
        case .gcLoginSuccess:
            handleLoginSuccess(body)
        case .gcPong:
            handlePong(body)
        default:
            break
        }
    }

    private func handleUDPMessage(_ data: Data) {
        guard data.count >= 8 else { return }

        var offset = 0
        let type = data[offset]; offset += 1
        let size = data.readUInt16(at: offset); offset += 2
        let sequence = data.readInt32(at: offset); offset += 4
        let timestamp = data.readInt32(at: offset); offset += 4

        let body = data.subdata(in: offset..<data.count)

        onPacketReceived?(type, body)

        switch PacketType(rawValue: type) {
        case .gcMove:
            handleMoveWithInterpolation(body, timestamp: Int64(timestamp))
        case .gcDamage:
            handleDamage(body)
        default:
            break
        }
    }

    private func handleLoginSuccess(_ data: Data) {
        let accountId = data.readInt32(at: 0)
        print("[Login] Success! AccountID: \(accountId)")
    }

    private func handlePong(_ data: Data) {
        let clientTime = data.readInt32(at: 0)
        let serverTime = data.readInt32(at: 4)

        let now = Int32(getCurrentTimeMs())
        let latency = Int(now - clientTime)

        updateLatencyMetrics(latency: latency)
        currentLatency = latency
        onLatencyUpdate?(latency)

        print("[Ping] Latency: \(latency)ms, Jitter: \(jitter)ms, Loss: \(String(format: "%.2f", packetLoss))%")
    }

    private func handleMoveWithInterpolation(_ data: Data, timestamp: Int64) {
        let id = data.readInt32(at: 0)
        let x = data.readInt32(at: 4)
        let y = data.readInt32(at: 8)
        let dir = data[12]

        // Add to interpolation buffer
        if entityStates[id] == nil {
            entityStates[id] = EntityStateBuffer()
        }

        entityStates[id]?.addState(x: x, y: y, dir: dir, timestamp: getCurrentTimeMs64())

        // Reconcile if own character
        if id == characterId {
            let sequence = data.readInt32(at: 0)
            reconcileWithServer(serverX: x, serverY: y, serverSeq: sequence)
        }
    }

    private func handleDamage(_ data: Data) {
        let attackerId = data.readInt32(at: 0)
        let victimId = data.readInt32(at: 4)
        let damage = data.readInt32(at: 8)

        print("[Damage] \(attackerId) -> \(victimId) = \(damage)")
    }

    // MARK: - Public API

    func getInterpolatedPosition(entityId: Int32) -> Position? {
        return entityStates[entityId]?.getInterpolatedPosition(renderTime: getCurrentTimeMs64())
    }

    func getMetrics() -> NetworkMetrics {
        return networkMetrics
    }

    func getConnectionQuality() -> String {
        if packetLoss > 5.0 || currentLatency > 200 {
            return "Poor"
        } else if packetLoss > 2.0 || currentLatency > 100 {
            return "Fair"
        } else if currentLatency < 50 {
            return "Excellent"
        } else {
            return "Good"
        }
    }

    // MARK: - Adaptive Optimization

    private func adjustUpdateRate() {
        if packetLoss > 5.0 {
            currentUpdateInterval = Self.MAX_UPDATE_INTERVAL_MS
        } else if currentLatency > 150 {
            currentUpdateInterval = (Self.MAX_UPDATE_INTERVAL_MS + Self.MIN_UPDATE_INTERVAL_MS) / 2
        } else if currentLatency < 50 {
            currentUpdateInterval = Self.MIN_UPDATE_INTERVAL_MS
        } else {
            currentUpdateInterval = 100
        }
    }

    private func updateLatencyMetrics(latency: Int) {
        networkMetrics.avgLatency = (networkMetrics.avgLatency * 9 + latency) / 10
        networkMetrics.minLatency = min(networkMetrics.minLatency, latency)
        networkMetrics.maxLatency = max(networkMetrics.maxLatency, latency)

        jitter = abs(latency - networkMetrics.avgLatency)
    }

    // MARK: - Utilities

    private func getCurrentTimeMs() -> Int32 {
        return Int32(Date().timeIntervalSince1970 * 1000)
    }

    private func getCurrentTimeMs64() -> Int64 {
        return Int64(Date().timeIntervalSince1970 * 1000)
    }
}

// MARK: - Extensions

extension Data {
    func readUInt16(at offset: Int) -> UInt16 {
        return withUnsafeBytes { $0.load(fromByteOffset: offset, as: UInt16.self) }
    }

    func readInt32(at offset: Int) -> Int32 {
        return withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
    }
}

extension String {
    func paddedData(to length: Int) -> Data {
        var data = Data(self.utf8)
        if data.count < length {
            data.append(Data(repeating: 0, count: length - data.count))
        } else if data.count > length {
            data = data.prefix(length)
        }
        return data
    }
}

extension FixedWidthInteger {
    var littleEndianData: Data {
        var value = self.littleEndian
        return Data(bytes: &value, count: MemoryLayout<Self>.size)
    }
}

// MARK: - Priority Queue

struct PriorityQueue<Element: Comparable> {
    private var heap = [Element]()
    private let lock = NSLock()

    mutating func enqueue(_ element: Element) {
        lock.lock()
        defer { lock.unlock() }

        heap.append(element)
        siftUp(from: heap.count - 1)
    }

    mutating func dequeue() -> Element? {
        lock.lock()
        defer { lock.unlock() }

        guard !heap.isEmpty else { return nil }

        if heap.count == 1 {
            return heap.removeFirst()
        }

        let value = heap[0]
        heap[0] = heap.removeLast()
        siftDown(from: 0)
        return value
    }

    private mutating func siftUp(from index: Int) {
        var child = index
        var parent = (child - 1) / 2

        while child > 0 && heap[child] < heap[parent] {
            heap.swapAt(child, parent)
            child = parent
            parent = (child - 1) / 2
        }
    }

    private mutating func siftDown(from index: Int) {
        var parent = index

        while true {
            let left = 2 * parent + 1
            let right = 2 * parent + 2
            var candidate = parent

            if left < heap.count && heap[left] < heap[candidate] {
                candidate = left
            }

            if right < heap.count && heap[right] < heap[candidate] {
                candidate = right
            }

            if candidate == parent {
                return
            }

            heap.swapAt(parent, candidate)
            parent = candidate
        }
    }
}
