#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <print>
#include <random>
#include <algorithm>
#include <cctype>
#include "KmboxNet.h"

// Minimal Winsock types to avoid including headers
typedef uintptr_t KM_SOCKET;
#define KM_INVALID_SOCKET (static_cast<KM_SOCKET>(~0))
#define KM_SOCKET_ERROR (-1)
#define KM_AF_INET 2
#define KM_SOCK_DGRAM 2
#define KM_IPPROTO_UDP 17
#define KM_SOL_SOCKET 0xffff
#define KM_SO_RCVTIMEO 0x1006
#define KM_SO_SNDTIMEO 0x1005
#define KM_INADDR_NONE 0xffffffff

#pragma pack(push, 1)
struct km_in_addr {
    uint32_t s_addr;
};
struct km_sockaddr_in {
    int16_t sin_family;
    uint16_t sin_port;
    km_in_addr sin_addr;
    char sin_zero[8];
};
#pragma pack(pop)

// Winsock function pointer types
typedef int (__stdcall* pWSAStartup)(uint16_t, void*);
typedef int (__stdcall* pWSACleanup)(void);
typedef KM_SOCKET (__stdcall* psocket)(int, int, int);
typedef int (__stdcall* pclosesocket)(KM_SOCKET);
typedef int (__stdcall* psendto)(KM_SOCKET, const char*, int, int, const void*, int);
typedef int (__stdcall* precvfrom)(KM_SOCKET, char*, int, int, void*, int*);
typedef int (__stdcall* pWSAGetLastError)(void);
typedef uint16_t (__stdcall* phtons)(uint16_t);
typedef uint32_t (__stdcall* pinet_addr)(const char*);
typedef int (__stdcall* psetsockopt)(KM_SOCKET, int, int, const char*, int);

namespace KmboxNet
{
    // Static helpers for dynamic loading
    static HMODULE hWs2 = nullptr;
    static pWSAStartup _WSAStartup = nullptr;
    static pWSACleanup _WSACleanup = nullptr;
    static psocket _socket = nullptr;
    static pclosesocket _closesocket = nullptr;
    static psendto _sendto = nullptr;
    static precvfrom _recvfrom = nullptr;
    static pWSAGetLastError _WSAGetLastError = nullptr;
    static phtons _htons = nullptr;
    static pinet_addr _inet_addr = nullptr;
    static psetsockopt _setsockopt = nullptr;

    static bool LoadWinsock()
    {
        if (hWs2) return true;
        hWs2 = GetModuleHandleA("ws2_32.dll");
        if (!hWs2) hWs2 = LoadLibraryA("ws2_32.dll");
        if (!hWs2) return false;

        _WSAStartup = (pWSAStartup)GetProcAddress(hWs2, "WSAStartup");
        _WSACleanup = (pWSACleanup)GetProcAddress(hWs2, "WSACleanup");
        _socket = (psocket)GetProcAddress(hWs2, "socket");
        _closesocket = (pclosesocket)GetProcAddress(hWs2, "closesocket");
        _sendto = (psendto)GetProcAddress(hWs2, "sendto");
        _recvfrom = (precvfrom)GetProcAddress(hWs2, "recvfrom");
        _WSAGetLastError = (pWSAGetLastError)GetProcAddress(hWs2, "WSAGetLastError");
        _htons = (phtons)GetProcAddress(hWs2, "htons");
        _inet_addr = (pinet_addr)GetProcAddress(hWs2, "inet_addr");
        _setsockopt = (psetsockopt)GetProcAddress(hWs2, "setsockopt");

        return _WSAStartup && _WSACleanup && _socket && _closesocket && _sendto && 
               _recvfrom && _WSAGetLastError && _htons && _inet_addr && _setsockopt;
    }

    KmboxNetClient::KmboxNetClient()
        : m_ClientSocket(KM_INVALID_SOCKET)
        , m_Mac(0)
        , m_Index(0)
        , m_Connected(false)
        , m_WsaInitialized(false)
        , m_TimeoutMs(2000)
    {
        memset(m_TargetAddr, 0, sizeof(m_TargetAddr));
    }

    KmboxNetClient::~KmboxNetClient()
    {
        Disconnect();
    }

    bool KmboxNetClient::Connect(const std::string& ip, int port, const std::string& macHex, int timeoutMs)
    {
        if (!LoadWinsock())
        {
            std::println("[KMboxNet] Failed to load ws2_32.dll or resolve functions");
            return false;
        }

        Disconnect(); // Cleanup any previous connection
        m_TimeoutMs = timeoutMs;

        // Initialize Winsock
        char wsaData[512]; // Enough for WSADATA
        if (_WSAStartup(0x0202, wsaData) != 0)
        {
            std::println("[KMboxNet] WSAStartup failed");
            return false;
        }
        m_WsaInitialized = true;

        // Create UDP socket
        m_ClientSocket = _socket(KM_AF_INET, KM_SOCK_DGRAM, KM_IPPROTO_UDP);
        if (m_ClientSocket == KM_INVALID_SOCKET)
        {
            std::println("[KMboxNet] Failed to create socket: {}", _WSAGetLastError());
            Disconnect();
            return false;
        }

        // Set socket timeout
        DWORD timeout = static_cast<DWORD>(timeoutMs);
        _setsockopt(m_ClientSocket, KM_SOL_SOCKET, KM_SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        _setsockopt(m_ClientSocket, KM_SOL_SOCKET, KM_SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

        // Setup remote address
        auto addr = reinterpret_cast<km_sockaddr_in*>(m_TargetAddr);
        addr->sin_family = KM_AF_INET;
        addr->sin_port = _htons(static_cast<uint16_t>(port));
        
        addr->sin_addr.s_addr = _inet_addr(ip.c_str());
        if (addr->sin_addr.s_addr == KM_INADDR_NONE)
        {
            std::println("[KMboxNet] Invalid IP address: {}", ip);
            Disconnect();
            return false;
        }

        // Parse MAC address
        m_Mac = MacToUInt(macHex);
        m_Index = 0;

        // Send connect command
        CmdHead head = NextHead(KmCommand::CmdConnect);
        CmdHead response{};

        if (!SendAndReceive(&head, sizeof(head), response))
        {
            std::println("[KMboxNet] Connection handshake failed");
            Disconnect();
            return false;
        }

        if (response.cmd != head.cmd || response.indexpts != head.indexpts)
        {
            std::println("[KMboxNet] Invalid connection response");
            Disconnect();
            return false;
        }

        m_Connected = true;
        std::println("[KMboxNet] Connected to {}:{}", ip, port);
        return true;
    }

    void KmboxNetClient::Disconnect()
    {
        m_Connected = false;

        if (m_ClientSocket != KM_INVALID_SOCKET && _closesocket)
        {
            _closesocket(m_ClientSocket);
            m_ClientSocket = KM_INVALID_SOCKET;
        }

        if (m_WsaInitialized && _WSACleanup)
        {
            _WSACleanup();
            m_WsaInitialized = false;
        }

        m_Index = 0;
    }

    bool KmboxNetClient::MouseMove(int16_t dx, int16_t dy)
    {
        if (!m_Connected)
            return false;

        CmdHead head = NextHead(KmCommand::CmdMouseMove);
        MouseAction action{};
        action.X = dx;
        action.Y = dy;

        struct {
            CmdHead head;
            MouseAction action;
        } payload;
        payload.head = head;
        payload.action = action;

        CmdHead response{};
        if (!SendAndReceive(&payload, sizeof(payload), response))
        {
            std::println("[KMboxNet] MouseMove failed");
            m_Connected = false;
            return false;
        }

        if (response.cmd != head.cmd || response.indexpts != head.indexpts)
        {
            std::println("[KMboxNet] Invalid MouseMove response");
            return false;
        }

        return true;
    }

    CmdHead KmboxNetClient::NextHead(KmCommand cmd)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);

        CmdHead head{};
        head.mac = m_Mac;
        head.rand = dist(gen);
        head.indexpts = m_Index++;
        head.cmd = cmd;

        return head;
    }

    bool KmboxNetClient::SendAndReceive(const void* data, size_t dataSize, CmdHead& outResponse)
    {
        if (m_ClientSocket == KM_INVALID_SOCKET)
            return false;

        int sent = _sendto(m_ClientSocket, 
                          reinterpret_cast<const char*>(data), 
                          static_cast<int>(dataSize), 
                          0,
                          m_TargetAddr,
                          sizeof(km_sockaddr_in));

        if (sent == KM_SOCKET_ERROR)
        {
            std::println("[KMboxNet] Send failed: {}", _WSAGetLastError());
            return false;
        }

        char buffer[256];
        km_sockaddr_in fromAddr{};
        int fromLen = sizeof(fromAddr);

        int received = _recvfrom(m_ClientSocket, 
                                buffer, 
                                sizeof(buffer), 
                                0,
                                &fromAddr,
                                &fromLen);

        if (received == KM_SOCKET_ERROR)
        {
            int err = _WSAGetLastError();
            std::println("[KMboxNet] Receive failed: {}", err);
            return false;
        }

        if (received < static_cast<int>(sizeof(CmdHead)))
        {
            return false;
        }

        memcpy(&outResponse, buffer, sizeof(CmdHead));
        return true;
    }

    uint32_t KmboxNetClient::MacToUInt(const std::string& macHex)
    {
        if (macHex.empty()) return 0;
        std::string cleaned;
        for (char c : macHex)
            if (c != ':' && c != '-' && c != ' ')
                cleaned += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        
        if (cleaned.empty()) return 0;
        try {
            if (cleaned.length() > 8) cleaned = cleaned.substr(cleaned.length() - 8);
            return static_cast<uint32_t>(std::stoul(cleaned, nullptr, 16));
        } catch (...) {
            return 0;
        }
    }

} // namespace KmboxNet
