/*
 * Lone EFT DMA Radar
 * Brought to you by Lone (Lone DMA)
 * 
 * MIT License
 * 
 * InputManager - Windows 10/11 Compatible Keyboard Input via DMA
 * Uses signature-based approach for W11 and EAT/PDB for W10
 */

using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using System.Text;
using LoneEftDmaRadar.Misc.Workers;
using LoneEftDmaRadar.UI.Hotkeys;
using LoneEftDmaRadar.UI.Misc;
using VmmSharpEx;
using VmmSharpEx.Extensions.Input;
using VmmSharpEx.Options;

namespace LoneEftDmaRadar.DMA
{
    /// <summary>
    /// Central input poller for hotkeys.
    /// Compatible with both Windows 10 and Windows 11 on the Game PC.
    /// Uses DMA to read keyboard state directly from win32kbase.sys.
    /// Falls back to VmmInputManager if custom implementation fails.
    /// </summary>
    public sealed class InputManager : IDisposable
    {
        // Kernel memory access flag from standard Vmmsharp (0x80000000)
        private const uint PID_PROCESS_WITH_KERNELMEMORY = 0x80000000;
        
        private readonly Vmm _vmm;
        private WorkerThread _thread;
        private string _initMethod = "Not Initialized";
        
        // Custom keyboard reader state
        private bool _customInitialized = false;
        private ulong _gafAsyncKeyStateExport;
        private uint _winLogonPid;
        
        // Debugging info fields
        private ulong _dmaWin32kBase;
        private ulong _dmaWin32ksgdBase;
        private ulong _dmaWin32k;
        private ulong _dmaSessionGlobalPtr;
        private ulong _dmaUserSessionState;
        private ulong _dmaGafAsyncOffset;
        
        private byte[] _currentStateBitmap = new byte[64];
        private byte[] _previousStateBitmap = new byte[64];
        private readonly ConcurrentDictionary<int, byte> _pressedKeys = new();
        
        // Fallback to VmmInputManager
        private VmmInputManager _fallbackInput;
        private bool _useFallback = false;

        /// <summary>
        /// True if the input backend is available.
        /// </summary>
        public bool IsReady => _customInitialized || _fallbackInput is not null;

        public InputManager(Vmm vmm)
        {
            _vmm = vmm ?? throw new ArgumentNullException(nameof(vmm));
            InitializeBackend();
            StartWorker();
        }

        public void Reinitialize()
        {
            DebugLogger.LogDebug("[InputManager] Reinitializing...");
            try
            {
                _thread?.Dispose();
                _thread = null;
                
                _customInitialized = false;
                _useFallback = false;
                _initMethod = "Reinitializing...";
                _fallbackInput = null;
                _gafAsyncKeyStateExport = 0;
                _winLogonPid = 0;
                
                InitializeBackend();
                StartWorker();
                DebugLogger.LogDebug($"[InputManager] Reinitialization complete. Method: {_initMethod}");
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] Reinitialization failed: {ex}");
                _initMethod = $"Error: {ex.Message}";
            }
        }

        private void InitializeBackend()
        {
            // Try custom implementation first (W10/W11 compatible)
            if (InitCustomKeyboard())
            {
                DebugLogger.LogDebug($"[InputManager] Custom keyboard handler initialized: {_initMethod}");
            }
            else
            {
                // Fall back to VmmInputManager
                DebugLogger.LogDebug("[InputManager] Custom init failed, trying VmmInputManager fallback...");
                try
                {
                    _fallbackInput = new VmmInputManager(_vmm);
                    _useFallback = true;
                    _initMethod = "Fallback (VmmInputManager)";
                    DebugLogger.LogDebug("[InputManager] VmmInputManager fallback initialized.");
                }
                catch (Exception ex)
                {
                    DebugLogger.LogDebug($"[InputManager] VmmInputManager fallback also failed: {ex.Message}");
                    DebugLogger.LogDebug("[InputManager] Hotkeys will use DeviceAimbot/local mouse only.");
                    _initMethod = "Failed (Local Only)";
                }
            }
        }

        private void StartWorker()
        {
            _thread = new WorkerThread
            {
                Name = nameof(InputManager),
                SleepDuration = TimeSpan.FromMilliseconds(12),
                SleepMode = WorkerThreadSleepMode.DynamicSleep
            };
            _thread.PerformWork += InputManager_PerformWork;
            _thread.Start();
        }

        /// <summary>
        /// Initialize custom keyboard reader that works on both W10 and W11.
        /// </summary>
        private bool InitCustomKeyboard()
        {
            try
            {
                // Get winlogon.exe PID (we need kernel memory access through this process)
                if (!_vmm.PidGetFromName("winlogon.exe", out uint winlogonPid))
                {
                    DebugLogger.LogDebug("[InputManager] winlogon.exe not found");
                    return false;
                }
                
                // Add kernel memory access flag
                _winLogonPid = winlogonPid | PID_PROCESS_WITH_KERNELMEMORY;
                
                // Try Windows 11 approach first (signature-based)
                if (TryInitWindows11())
                {
                    _customInitialized = true;
                    return true;
                }
                
                // Fall back to Windows 10 approach (EAT export)
                if (TryInitWindows10())
                {
                    _customInitialized = true;
                    return true;
                }
                
                return false;
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] Custom init error: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Windows 11 initialization using signature scanning.
        /// </summary>
        private bool TryInitWindows11()
        {
            try
            {
                DebugLogger.LogDebug("[InputManager] Trying Windows 11 signature-based approach...");
                
                // Get win32kbase.sys base address
                var win32kbaseBase = _vmm.ProcessGetModuleBase(_winLogonPid, "win32kbase.sys");
                _dmaWin32kBase = win32kbaseBase;
                if (win32kbaseBase == 0)
                {
                    DebugLogger.LogDebug("[InputManager] win32kbase.sys not found via winlogon");
                    return false;
                }
                
                DebugLogger.LogDebug($"[InputManager] Found win32kbase.sys at 0x{win32kbaseBase:X}");
                
                // Try to find win32ksgd.sys first (Windows 11 24H2+)
                var win32ksgdBase = _vmm.ProcessGetModuleBase(_winLogonPid, "win32ksgd.sys");
                _dmaWin32ksgdBase = win32ksgdBase;
                ulong searchBase;
                ulong searchSize;
                string moduleName;
                
                if (win32ksgdBase != 0)
                {
                    DebugLogger.LogDebug($"[InputManager] Found win32ksgd.sys at 0x{win32ksgdBase:X}");
                    searchBase = win32ksgdBase;
                    // Estimate module size (we'll scan a reasonable range)
                    searchSize = 0x100000; // 1MB should be plenty
                    moduleName = "win32ksgd.sys";
                }
                else
                {
                    // Try win32k.sys
                    var win32kBase = _vmm.ProcessGetModuleBase(_winLogonPid, "win32k.sys");
                    _dmaWin32k = win32kBase;
                    if (win32kBase == 0)
                    {
                        DebugLogger.LogDebug("[InputManager] Neither win32ksgd.sys nor win32k.sys found");
                        return false;
                    }
                    DebugLogger.LogDebug($"[InputManager] Found win32k.sys at 0x{win32kBase:X}");
                    searchBase = win32kBase;
                    searchSize = 0x200000; // 2MB
                    moduleName = "win32k.sys";
                }
                
                // Search for session globals pointer pattern
                // Pattern: 48 8B 05 ?? ?? ?? ?? 48 8B 04 C8 (or alternative: 48 8B 05 ?? ?? ?? ?? FF C9)
                ulong gSessionPtr = FindPattern(_winLogonPid, searchBase, searchSize,
                    "48 8B 05 ?? ?? ?? ?? 48 8B 04 C8");
                _dmaSessionGlobalPtr = gSessionPtr;
                string sigUsed = "Sig 1";
                
                if (gSessionPtr == 0)
                {
                    gSessionPtr = FindPattern(_winLogonPid, searchBase, searchSize,
                        "48 8B 05 ?? ?? ?? ?? FF C9");
                    _dmaSessionGlobalPtr = gSessionPtr;
                    sigUsed = "Sig 2";
                }
                
                if (gSessionPtr == 0)
                {
                    DebugLogger.LogDebug("[InputManager] Session pointer pattern not found");
                    return false;
                }
                
                DebugLogger.LogDebug($"[InputManager] Found session pattern at 0x{gSessionPtr:X}");
                
                // Read relative offset (at pattern + 3)
                int relOffset = _vmm.MemReadValue<int>(_winLogonPid, gSessionPtr + 3, VmmFlags.NOCACHE);
                if (relOffset == 0)
                {
                    DebugLogger.LogDebug("[InputManager] Failed to read relative offset");
                    return false;
                }
                
                // Calculate absolute address (RIP-relative addressing)
                ulong sessionGlobalSlots = gSessionPtr + 7 + (ulong)relOffset;
                DebugLogger.LogDebug($"[InputManager] Session global slots at 0x{sessionGlobalSlots:X}");
                
                // Resolve user session state by following pointers
                ulong userSessionState = 0;
                for (int i = 0; i < 8; i++)
                {
                    ulong t1 = _vmm.MemReadValue<ulong>(_winLogonPid, sessionGlobalSlots, VmmFlags.NOCACHE);
                    if (t1 == 0) continue;
                    
                    ulong t2 = _vmm.MemReadValue<ulong>(_winLogonPid, t1 + (ulong)(8 * i), VmmFlags.NOCACHE);
                    if (t2 == 0) continue;
                    
                    ulong t3 = _vmm.MemReadValue<ulong>(_winLogonPid, t2, VmmFlags.NOCACHE);
                    if (t3 == 0) continue;
                    
                    if (t3 > 0x7FFFFFFFFFFF) // Valid kernel address
                    {
                        userSessionState = t3;
                        _dmaUserSessionState = userSessionState;
                        break;
                    }
                }
                
                if (userSessionState == 0)
                {
                    DebugLogger.LogDebug("[InputManager] Failed to resolve user session state");
                    return false;
                }
                
                DebugLogger.LogDebug($"[InputManager] User session state at 0x{userSessionState:X}");
                
                // Find gafAsyncKeyState offset in win32kbase.sys
                // Pattern: 48 8D 90 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F 57 C0
                ulong offsetPattern = FindPattern(_winLogonPid, win32kbaseBase, 0x200000,
                    "48 8D 90 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F 57 C0");
                
                if (offsetPattern == 0)
                {
                    DebugLogger.LogDebug("[InputManager] Async key state offset pattern not found");
                    return false;
                }
                
                // Read the offset from pattern + 3
                uint keyStateOffset = _vmm.MemReadValue<uint>(_winLogonPid, offsetPattern + 3, VmmFlags.NOCACHE);
                _dmaGafAsyncOffset = keyStateOffset;
                if (keyStateOffset == 0)
                {
                    DebugLogger.LogDebug("[InputManager] Failed to read key state offset");
                    return false;
                }
                
                _gafAsyncKeyStateExport = userSessionState + keyStateOffset;
                
                if (_gafAsyncKeyStateExport < 0x7FFFFFFFFFFF)
                {
                    DebugLogger.LogDebug("[InputManager] Invalid key state address");
                    return false;
                }
                
                DebugLogger.LogDebug($"[InputManager] gafAsyncKeyState at 0x{_gafAsyncKeyStateExport:X}");
                
                _initMethod = $"W11 ({moduleName}) - {sigUsed}";
                return true;
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] Windows 11 init failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Windows 10 initialization using EAT export.
        /// </summary>
        private bool TryInitWindows10()
        {
            try
            {
                DebugLogger.LogDebug("[InputManager] Trying Windows 10 EAT approach...");
                
                // Get win32kbase.sys base address
                var win32kbaseBase = _vmm.ProcessGetModuleBase(_winLogonPid, "win32kbase.sys");
                _dmaWin32kBase = win32kbaseBase;
                if (win32kbaseBase == 0)
                {
                    DebugLogger.LogDebug("[InputManager] win32kbase.sys not found");
                    return false;
                }
                
                // Alternative pattern for older Windows
                ulong keyStateAddr = FindPattern(_winLogonPid, win32kbaseBase, 0x200000,
                    "48 8D 0D ?? ?? ?? ?? 8B 04 81");
                
                if (keyStateAddr != 0)
                {
                    int relOffset = _vmm.MemReadValue<int>(_winLogonPid, keyStateAddr + 3, VmmFlags.NOCACHE);
                    if (relOffset != 0)
                    {
                        _gafAsyncKeyStateExport = keyStateAddr + 7 + (ulong)relOffset;
                        if (_gafAsyncKeyStateExport > 0x7FFFFFFFFFFF)
                        {
                        if (_gafAsyncKeyStateExport > 0x7FFFFFFFFFFF)
                        {
                            DebugLogger.LogDebug($"[InputManager] Found via W10 pattern at 0x{_gafAsyncKeyStateExport:X}");
                            _initMethod = "W10 (EAT Export)";
                            return true;
                        }
                        }
                    }
                }
                
                DebugLogger.LogDebug("[InputManager] Windows 10 approach failed");
                return false;
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] Windows 10 init failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Find a byte pattern in process memory.
        /// </summary>
        private ulong FindPattern(uint pid, ulong startAddr, ulong size, string pattern)
        {
            try
            {
                // Parse pattern
                var parts = pattern.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                var bytes = new List<byte>();
                var mask = new List<bool>();
                
                foreach (var part in parts)
                {
                    if (part == "?" || part == "??")
                    {
                        bytes.Add(0);
                        mask.Add(false); // wildcard
                    }
                    else
                    {
                        bytes.Add(Convert.ToByte(part, 16));
                        mask.Add(true); // must match
                    }
                }
                
                var patternBytes = bytes.ToArray();
                var patternMask = mask.ToArray();
                
                // Read memory in chunks
                const int chunkSize = 0x10000; // 64KB
                var buffer = new byte[chunkSize];
                
                for (ulong addr = startAddr; addr < startAddr + size; addr += (ulong)(chunkSize - patternBytes.Length))
                {
                    try
                    {
                        var bufferSpan = buffer.AsSpan();
                        if (!_vmm.MemReadSpan(pid, addr, bufferSpan, VmmFlags.NOCACHE))
                            continue;
                        
                        // Search in buffer
                        for (int i = 0; i <= buffer.Length - patternBytes.Length; i++)
                        {
                            bool found = true;
                            for (int j = 0; j < patternBytes.Length; j++)
                            {
                                if (patternMask[j] && buffer[i + j] != patternBytes[j])
                                {
                                    found = false;
                                    break;
                                }
                            }
                            
                            if (found)
                            {
                                return addr + (ulong)i;
                            }
                        }
                    }
                    catch
                    {
                        continue;
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] FindPattern error: {ex.Message}");
            }
            
            return 0;
        }

        /// <summary>
        /// Update keyboard state by reading from kernel memory.
        /// </summary>
        private void UpdateCustomKeys()
        {
            if (!_customInitialized || _gafAsyncKeyStateExport < 0x7FFFFFFFFFFF)
                return;
            
            try
            {
                Array.Copy(_currentStateBitmap, _previousStateBitmap, 64);
                
                var bufferSpan = _currentStateBitmap.AsSpan();
                if (!_vmm.MemReadSpan(_winLogonPid, _gafAsyncKeyStateExport, bufferSpan, VmmFlags.NOCACHE))
                    return;
                
                _pressedKeys.Clear();
                
                for (int vk = 0; vk < 256; ++vk)
                {
                    // Key state is stored as 2 bits per key
                    if ((_currentStateBitmap[(vk * 2 / 8)] & (1 << (vk % 4 * 2))) != 0)
                    {
                        _pressedKeys.TryAdd(vk, 1);
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[InputManager] UpdateCustomKeys error: {ex.Message}");
            }
        }

        /// <summary>
        /// Check if a key is currently held down (custom implementation).
        /// </summary>
        private bool IsCustomKeyDown(int key)
        {
            return _pressedKeys.ContainsKey(key);
        }

        private void InputManager_PerformWork(object sender, WorkerThreadArgs e)
        {
            var hotkeys = HotkeyManagerViewModel.Hotkeys.AsEnumerable();
            if (!hotkeys.Any())
                return;

            // Update key states
            if (_customInitialized)
            {
                UpdateCustomKeys();
            }
            else if (_useFallback && _fallbackInput is not null)
            {
                try
                {
                    _fallbackInput.UpdateKeys();
                }
                catch (Exception ex)
                {
                    DebugLogger.LogDebug($"[InputManager] Fallback UpdateKeys failed: {ex}");
                }
            }

            foreach (var kvp in hotkeys)
            {
                var vk = kvp.Key;
                var action = kvp.Value;

                bool isDownDMA = false;
                
                if (_customInitialized)
                {
                    isDownDMA = IsCustomKeyDown((int)vk);
                }
                else if (_useFallback && _fallbackInput is not null)
                {
                    try
                    {
                        isDownDMA = _fallbackInput.IsKeyDown(vk);
                    }
                    catch
                    {
                        isDownDMA = false;
                    }
                }

                bool isDownMouseFallback = IsMouseVirtualKey(vk) && IsMouseAsyncDown(vk);

                // FINAL state: key is considered down if ANY backend reports it.
                bool isKeyDown = isDownDMA || isDownMouseFallback;

                action.Execute(isKeyDown);
            }
        }

        private static bool IsMouseVirtualKey(Win32VirtualKey vk) =>
            vk is Win32VirtualKey.LBUTTON
            or Win32VirtualKey.RBUTTON
            or Win32VirtualKey.MBUTTON
            or Win32VirtualKey.XBUTTON1
            or Win32VirtualKey.XBUTTON2;

        [DllImport("user32.dll")]
        private static extern short GetAsyncKeyState(int vKey);

        public string GetDebugStatus()
        {
            return _initMethod;
        }

        public IEnumerable<int> GetPressedKeys()
        {
            if (_customInitialized)
            {
                return _pressedKeys.Keys.ToList();
            }
            if (_useFallback && _fallbackInput != null)
            {
                var keys = new List<int>();
                // Only checking mouse buttons for fallback as checking all 256 keys might be slow
                // mapped to Win32VirtualKey enum values for mouse
                var mouseKeys = new[] { 1, 2, 4, 5, 6 }; 
                foreach (var k in mouseKeys)
                {
                    if (_fallbackInput.IsKeyDown((Win32VirtualKey)k)) keys.Add(k);
                }
                return keys;
            }
            return Enumerable.Empty<int>();
        }

        public string GenerateDebugReport()
        {
            var sb = new StringBuilder();
            sb.AppendLine("=== InputManager Debug Report ===");
            sb.AppendLine($"Time: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            sb.AppendLine($"Status: {_initMethod}");
            sb.AppendLine($"IsReady: {IsReady}");
            sb.AppendLine($"Custom Init: {_customInitialized}");
            sb.AppendLine($"Use Fallback: {_useFallback}");
            sb.AppendLine();
            
            sb.AppendLine("--- DMA Info ---");
            sb.AppendLine($"WinLogon PID: {_winLogonPid} (Hex: {_winLogonPid:X})");
            sb.AppendLine($"win32kbase Base: 0x{_dmaWin32kBase:X16}");
            sb.AppendLine($"win32ksgd Base: 0x{_dmaWin32ksgdBase:X16}");
            sb.AppendLine($"win32k Base: 0x{_dmaWin32k:X16}");
            
            sb.AppendLine();
            sb.AppendLine("--- Pointer Info ---");
            sb.AppendLine($"Session Global Ptr (Sig Result): 0x{_dmaSessionGlobalPtr:X16}");
            sb.AppendLine($"User Session State: 0x{_dmaUserSessionState:X16}");
            sb.AppendLine($"gafAsyncKeyState Export: 0x{_gafAsyncKeyStateExport:X16}");
            sb.AppendLine($"Key State Offset: 0x{_dmaGafAsyncOffset:X}");
            
            if (_useFallback)
            {
                sb.AppendLine();
                sb.AppendLine("--- Fallback Info ---");
                sb.AppendLine($"Fallback Object: {(_fallbackInput != null ? "Active" : "Null")}");
            }

            return sb.ToString();
        }

        private static bool IsMouseAsyncDown(Win32VirtualKey vk)
        {
            var state = GetAsyncKeyState((int)vk);
            return (state & 0x8000) != 0;
        }

        public void Dispose()
        {
            _thread?.Dispose();
            _customInitialized = false;
        }
    }
}