#include <memory>
#include <mutex>
#include <string>
#include <print>
#include "KmboxNetController.h"
#include "KmboxNet.h"

namespace KmboxNet
{
    bool KmboxNetController::Connect(const std::string& ip, int port, const std::string& macHex, int timeoutMs)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        // Disconnect any existing connection
        if (s_Client)
        {
            s_Client->Disconnect();
            s_Client.reset();
        }

        s_Connected = false;

        // Create new client and connect
        s_Client = std::make_unique<KmboxNetClient>();
        
        if (s_Client->Connect(ip, port, macHex, timeoutMs))
        {
            s_Connected = true;
            std::println("[KmboxNetController] Connected successfully");
            return true;
        }

        std::println("[KmboxNetController] Connection failed");
        s_Client.reset();
        return false;
    }

    void KmboxNetController::Disconnect()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        s_Connected = false;

        if (s_Client)
        {
            s_Client->Disconnect();
            s_Client.reset();
        }

        std::println("[KmboxNetController] Disconnected");
    }

    void KmboxNetController::Move(int dx, int dy)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (!s_Connected || !s_Client)
            return;

        if (!s_Client->MouseMove(static_cast<int16_t>(dx), static_cast<int16_t>(dy)))
        {
            // Connection may have been lost
            std::println("[KmboxNetController] Move failed, connection may be lost");
            s_Connected = false;
        }
    }

    bool KmboxNetController::IsConnected()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        return s_Connected && s_Client && s_Client->IsConnected();
    }

} // namespace KmboxNet
