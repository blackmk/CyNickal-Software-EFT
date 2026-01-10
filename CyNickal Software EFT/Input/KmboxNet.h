#pragma once

#include <string>
#include <cstdint>

namespace KmboxNet
{
    enum class KmCommand : uint32_t
    {
        CmdConnect = 0xaf3c2828,
        CmdMouseMove = 0xaede7345
    };

    #pragma pack(push, 1)
    struct CmdHead
    {
        uint32_t mac;
        uint32_t rand;
        uint32_t indexpts;
        KmCommand cmd;
    };

    struct MouseAction
    {
        int32_t Buttons;
        int32_t X;
        int32_t Y;
        int32_t Wheel;
        int32_t Points[10];
    };
    #pragma pack(pop)

    class KmboxNetClient
    {
    public:
        KmboxNetClient();
        ~KmboxNetClient();

        bool Connect(const std::string& ip, int port, const std::string& macHex, int timeoutMs = 2000);
        void Disconnect();
        bool MouseMove(int16_t dx, int16_t dy);
        bool IsConnected() const { return m_Connected; }

    private:
        CmdHead NextHead(KmCommand cmd);
        bool SendAndReceive(const void* data, size_t dataSize, CmdHead& outResponse);
        static uint32_t MacToUInt(const std::string& macHex);

        uintptr_t m_ClientSocket;
        uint8_t m_TargetAddr[16];
        uint32_t m_Mac;
        uint32_t m_Index;
        bool m_Connected;
        bool m_WsaInitialized;
        int m_TimeoutMs;
    };
}
