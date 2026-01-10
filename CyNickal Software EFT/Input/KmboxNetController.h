#pragma once

#include <string>
#include <mutex>
#include "KmboxNet.h"

namespace KmboxNet
{
    /// <summary>
    /// Static controller for KMbox NET device.
    /// Provides thread-safe access to a single KMbox NET connection.
    /// Matches inspiration project's DeviceNetController.cs pattern.
    /// </summary>
    class KmboxNetController
    {
    public:
        /// <summary>
        /// Connect to KMbox NET device.
        /// </summary>
        /// <param name="ip">Device IP address</param>
        /// <param name="port">Device port</param>
        /// <param name="macHex">Device MAC address (hex string)</param>
        /// <param name="timeoutMs">Connection timeout in milliseconds</param>
        /// <returns>True if connected successfully</returns>
        static bool Connect(const std::string& ip, int port, const std::string& macHex, int timeoutMs = 2000);

        /// <summary>
        /// Disconnect from device and cleanup resources.
        /// </summary>
        static void Disconnect();

        /// <summary>
        /// Send relative mouse movement.
        /// </summary>
        /// <param name="dx">X delta</param>
        /// <param name="dy">Y delta</param>
        static void Move(int dx, int dy);

        /// <summary>
        /// Check if connected to device.
        /// </summary>
        static bool IsConnected();

    private:
        static inline std::unique_ptr<KmboxNetClient> s_Client;
        static inline std::mutex s_Mutex;
        static inline bool s_Connected = false;
    };

} // namespace KmboxNet
