import Foundation
import Network

/**
 * Metin2 PvP Network Client for iOS
 * Hibrit protokol: WebSocket + UDP
 * Swift 5.0+, iOS 13.0+
 */

class NetworkClient {

    // MARK: - Constants

    enum PacketType: UInt8 {
        case cgLogin = 2
        case gcLoginSuccess = 3
        case cgCharacterSelect = 10
        case gcCharacterInfo = 11
        case cgMove = 20
        case gcMove = 21
        case cgAttack = 30
        case gcDamage = 32
        case cgChat = 40
        case gcChat = 41
        case cgPing = 50
        case gcPong = 51
    }

    // MARK: - Properties

    private let serverIP: String
    private let wsPort: Int
    private let udpPort: Int

    // WebSocket
    private var webSocketTask: URLSessionWebSocketTask?
    private var urlSession: URLSession?

    // UDP (Using Network.framework)
    private var udpConnection: NWConnection?
    private let udpQueue = DispatchQueue(label: "com.metin2.udp")

    // State
    private var isConnected = false
    private var sessionId: UInt32 = 0
    private var characterId: UInt32 = 0
    private var sequenceNumber: UInt32 = 0

    // Callbacks
    var onConnected: (() -> Void)?
    var onDisconnected: ((String) -> Void)?
    var onPacketReceived: ((UInt8, Data) -> Void)?
    var onError: ((String) -> Void)?

    // MARK: - Initialization

    init(serverIP: String, wsPort: Int = 8080, udpPort: Int = 8081) {
        self.serverIP = serverIP
        self.wsPort = wsPort
        self.udpPort = udpPort
    }

    // MARK: - Connection

    func connectWebSocket() {
        let urlString = "ws://\(serverIP):\(wsPort)"
        guard let url = URL(string: urlString) else {
            onError?("Invalid URL")
            return
        }

        urlSession = URLSession(configuration: .default)
        webSocketTask = urlSession?.webSocketTask(with: url)

        webSocketTask?.resume()
        isConnected = true
        onConnected?()

        // Start receiving messages
        receiveWebSocketMessage()

        print("[WS] Connected to \(urlString)")
    }

    func startUDP() {
        let host = NWEndpoint.Host(serverIP)
        let port = NWEndpoint.Port(integerLiteral: UInt16(udpPort))

        udpConnection = NWConnection(host: host, port: port, using: .udp)

        udpConnection?.stateUpdateHandler = { [weak self] state in
            switch state {
            case .ready:
                print("[UDP] Connected")
                self?.receiveUDPMessage()

            case .failed(let error):
                print("[UDP] Failed: \(error)")
                self?.onError?("UDP connection failed")

            case .cancelled:
                print("[UDP] Cancelled")

            default:
                break
            }
        }

        udpConnection?.start(queue: udpQueue)
    }

    func disconnect() {
        // Close WebSocket
        webSocketTask?.cancel(with: .goingAway, reason: nil)
        webSocketTask = nil
        urlSession = nil

        // Close UDP
        udpConnection?.cancel()
        udpConnection = nil

        isConnected = false
        print("[Network] Disconnected")
    }

    // MARK: - Packet Sending

    func sendLogin(username: String, password: String) {
        var buffer = Data()

        // Header
        buffer.append(PacketType.cgLogin.rawValue)
        buffer.append(UInt16(8 + 56).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

        // Body
        buffer.append(username.paddedData(to: 24))
        buffer.append(password.paddedData(to: 32))

        sendWebSocket(data: buffer)
    }

    func sendCharacterSelect(charId: UInt32) {
        var buffer = Data()

        buffer.append(PacketType.cgCharacterSelect.rawValue)
        buffer.append(UInt16(8 + 4).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)
        buffer.append(charId.littleEndianData)

        characterId = charId
        sendWebSocket(data: buffer)
    }

    func sendMove(x: Int32, y: Int32, dir: UInt8) {
        var buffer = Data()

        buffer.append(PacketType.cgMove.rawValue)
        buffer.append(UInt16(8 + 9).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

        buffer.append(x.littleEndianData)
        buffer.append(y.littleEndianData)
        buffer.append(dir)

        sendUDP(data: buffer)
    }

    func sendAttack(targetId: UInt32) {
        var buffer = Data()

        buffer.append(PacketType.cgAttack.rawValue)
        buffer.append(UInt16(8 + 6).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

        buffer.append(targetId.littleEndianData)
        buffer.append(UInt8(0)) // attack_type
        buffer.append(UInt16(0).littleEndianData) // skill_id

        sendUDP(data: buffer)
    }

    func sendChat(message: String) {
        var buffer = Data()

        buffer.append(PacketType.cgChat.rawValue)
        buffer.append(UInt16(8 + 261).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

        buffer.append(UInt8(0)) // type
        buffer.append(UInt32(0).littleEndianData) // target_id
        buffer.append(message.paddedData(to: 256))

        sendWebSocket(data: buffer)
    }

    func sendPing() {
        var buffer = Data()

        let clientTime = UInt32(Date().timeIntervalSince1970 * 1000)

        buffer.append(PacketType.cgPing.rawValue)
        buffer.append(UInt16(8 + 4).littleEndianData)
        sequenceNumber += 1
        buffer.append(sequenceNumber.littleEndianData)
        buffer.append(clientTime.littleEndianData)
        buffer.append(clientTime.littleEndianData)

        sendWebSocket(data: buffer)
    }

    // MARK: - Low-level Send

    private func sendWebSocket(data: Data) {
        let message = URLSessionWebSocketTask.Message.data(data)
        webSocketTask?.send(message) { error in
            if let error = error {
                print("[WS] Send error: \(error)")
                self.onError?("WebSocket send failed")
            }
        }
    }

    private func sendUDP(data: Data) {
        udpConnection?.send(content: data, completion: .contentProcessed { error in
            if let error = error {
                print("[UDP] Send error: \(error)")
            }
        })
    }

    // MARK: - Receiving

    private func receiveWebSocketMessage() {
        webSocketTask?.receive { [weak self] result in
            guard let self = self else { return }

            switch result {
            case .success(let message):
                switch message {
                case .data(let data):
                    self.handleWebSocketMessage(data: data)

                case .string(let text):
                    print("[WS] Received text: \(text)")

                @unknown default:
                    break
                }

                // Continue receiving
                self.receiveWebSocketMessage()

            case .failure(let error):
                print("[WS] Receive error: \(error)")
                self.onDisconnected?("WebSocket error")
            }
        }
    }

    private func receiveUDPMessage() {
        udpConnection?.receiveMessage { [weak self] data, context, isComplete, error in
            guard let self = self else { return }

            if let data = data, !data.isEmpty {
                self.handleUDPMessage(data: data)
            }

            if let error = error {
                print("[UDP] Receive error: \(error)")
            } else {
                // Continue receiving
                self.receiveUDPMessage()
            }
        }
    }

    // MARK: - Packet Handling

    private func handleWebSocketMessage(data: Data) {
        guard data.count >= 8 else { return }

        let type = data[0]
        let size = data.readUInt16(at: 1)
        let sequence = data.readUInt32(at: 3)
        let timestamp = data.readUInt32(at: 7)

        print("[WS] Packet: type=\(type), size=\(size), seq=\(sequence)")

        let body = data.subdata(in: 8..<data.count)
        onPacketReceived?(type, body)

        // Specific handlers
        switch type {
        case PacketType.gcLoginSuccess.rawValue:
            handleLoginSuccess(data: body)
        case PacketType.gcCharacterInfo.rawValue:
            handleCharacterInfo(data: body)
        case PacketType.gcChat.rawValue:
            handleChat(data: body)
        case PacketType.gcPong.rawValue:
            handlePong(data: body)
        default:
            break
        }
    }

    private func handleUDPMessage(data: Data) {
        guard data.count >= 8 else { return }

        let type = data[0]
        let size = data.readUInt16(at: 1)

        print("[UDP] Packet: type=\(type), size=\(size)")

        let body = data.subdata(in: 8..<data.count)
        onPacketReceived?(type, body)

        // Specific handlers
        switch type {
        case PacketType.gcMove.rawValue:
            handleMove(data: body)
        case PacketType.gcDamage.rawValue:
            handleDamage(data: body)
        default:
            break
        }
    }

    private func handleLoginSuccess(data: Data) {
        let accountId = data.readUInt32(at: 0)
        let charCount = data[4]

        print("[Login] Success! Account: \(accountId), Characters: \(charCount)")
    }

    private func handleCharacterInfo(data: Data) {
        let id = data.readUInt32(at: 0)
        let name = data.readString(at: 4, length: 24)
        let job = data[28]
        let level = data[29]
        let hp = data.readUInt32(at: 30)
        let maxHp = data.readUInt32(at: 34)

        print("[Character] \(name) (ID:\(id)) Lv.\(level) HP:\(hp)/\(maxHp)")
    }

    private func handleChat(data: Data) {
        let senderId = data.readUInt32(at: 0)
        let senderName = data.readString(at: 4, length: 24)
        let type = data[28]
        let message = data.readString(at: 29, length: 256)

        print("[CHAT] \(senderName): \(message)")
    }

    private func handlePong(data: Data) {
        let clientTime = data.readUInt32(at: 0)
        let serverTime = data.readUInt32(at: 4)
        let latency = UInt32(Date().timeIntervalSince1970 * 1000) - clientTime

        print("[Pong] Latency: \(latency)ms")
    }

    private func handleMove(data: Data) {
        let id = data.readUInt32(at: 0)
        let x = data.readInt32(at: 4)
        let y = data.readInt32(at: 8)
        let dir = data[12]

        print("[Move] Player \(id) moved to (\(x), \(y))")
    }

    private func handleDamage(data: Data) {
        let attackerId = data.readUInt32(at: 0)
        let victimId = data.readUInt32(at: 4)
        let damage = data.readUInt32(at: 8)
        let isCritical = data[12]

        let critText = isCritical == 1 ? " CRIT!" : ""
        print("[Damage] \(attackerId) -> \(victimId) = \(damage)\(critText)")
    }
}

// MARK: - Data Extensions

extension Data {
    func readUInt8(at offset: Int) -> UInt8 {
        return self[offset]
    }

    func readUInt16(at offset: Int) -> UInt16 {
        return withUnsafeBytes { $0.load(fromByteOffset: offset, as: UInt16.self) }
    }

    func readUInt32(at offset: Int) -> UInt32 {
        return withUnsafeBytes { $0.load(fromByteOffset: offset, as: UInt32.self) }
    }

    func readInt32(at offset: Int) -> Int32 {
        return withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
    }

    func readString(at offset: Int, length: Int) -> String {
        let subdata = self.subdata(in: offset..<(offset + length))
        // Remove null terminators
        let cleanData = subdata.prefix(while: { $0 != 0 })
        return String(data: cleanData, encoding: .utf8) ?? ""
    }
}

extension String {
    func paddedData(to length: Int) -> Data {
        var data = self.data(using: .utf8) ?? Data()
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

// MARK: - Usage Example

class NetworkExample {
    let client = NetworkClient(serverIP: "192.168.1.100")

    func example() {
        client.onConnected = {
            print("Connected!")
            self.client.sendLogin(username: "testuser", password: "password123")
        }

        client.onDisconnected = { reason in
            print("Disconnected: \(reason)")
        }

        client.onPacketReceived = { type, data in
            print("Packet received: type=\(type), size=\(data.count)")
        }

        client.connectWebSocket()
        client.startUDP()

        // Movement example
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
            self.client.sendMove(x: 957300, y: 245000, dir: 0)
        }

        // Ping example
        Timer.scheduledTimer(withTimeInterval: 5.0, repeats: true) { _ in
            self.client.sendPing()
        }
    }
}
