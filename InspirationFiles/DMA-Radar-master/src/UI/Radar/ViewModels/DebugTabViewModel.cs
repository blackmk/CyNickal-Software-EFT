using LoneEftDmaRadar;
using LoneEftDmaRadar.UI.Misc;
using System;
using System.ComponentModel;
using System.Text;
using System.Windows.Input;
using System.Windows.Threading;
using System.Windows;
using System.Threading.Tasks;
using LoneEftDmaRadar.DMA;
using VmmSharpEx.Extensions.Input;

namespace LoneEftDmaRadar.UI.Radar.ViewModels
{
    public sealed class DebugTabViewModel : INotifyPropertyChanged
    {
        private readonly DispatcherTimer _timer;
        private string _DeviceAimbotDebugText = "DeviceAimbot Aimbot: (no data)";
        private bool _showDeviceAimbotDebug = App.Config.Device.ShowDebug;

        /// <summary>
        /// Memory Inspector for browsing and reading offset struct values.
        /// </summary>
        public MemoryInspectorViewModel MemoryInspector { get; } = new();

        public DebugTabViewModel()
        {
            ToggleDebugConsoleCommand = new SimpleCommand(DebugLogger.Toggle);
            ReinitializeInputManagerCommand = new SimpleCommand(() => Task.Run(() => Memory.Input?.Reinitialize()));
            CopyInputReportCommand = new SimpleCommand(() =>
            {
                var report = Memory.Input?.GenerateDebugReport() ?? "No InputManager Instance";
                try
                {
                    Clipboard.SetText(report);
                }
                catch (Exception ex)
                {
                    DebugLogger.LogDebug($"[UI] Start clipboard error: {ex.Message}");
                }
            });

            _timer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(100)
            };
            _timer.Tick += (_, _) => RefreshDebugInfo();
            _timer.Start();
            RefreshDebugInfo();
        }

        public ICommand ToggleDebugConsoleCommand { get; }
        public ICommand ReinitializeInputManagerCommand { get; }
        public ICommand CopyInputReportCommand { get; }

        public bool ShowDeviceAimbotDebug
        {
            get => _showDeviceAimbotDebug;
            set
            {
                if (_showDeviceAimbotDebug == value)
                    return;
                _showDeviceAimbotDebug = value;
                App.Config.Device.ShowDebug = value;
                OnPropertyChanged(nameof(ShowDeviceAimbotDebug));
            }
        }

        public string DeviceAimbotDebugText
        {
            get => _DeviceAimbotDebugText;
            private set
            {
                if (_DeviceAimbotDebugText != value)
                {
                    _DeviceAimbotDebugText = value;
                    OnPropertyChanged(nameof(DeviceAimbotDebugText));
                }
            }
        }

        private string _inputManagerStatus = "Unknown";
        public string InputManagerStatus
        {
            get => _inputManagerStatus;
            private set
            {
                if (_inputManagerStatus != value)
                {
                    _inputManagerStatus = value;
                    OnPropertyChanged(nameof(InputManagerStatus));
                }
            }
        }

        private string _inputManagerKeys = "None";
        public string InputManagerKeys
        {
            get => _inputManagerKeys;
            private set
            {
                if (_inputManagerKeys != value)
                {
                    _inputManagerKeys = value;
                    OnPropertyChanged(nameof(InputManagerKeys));
                }
            }
        }

        private void RefreshDebugInfo()
        {
            var snapshot = Memory.DeviceAimbot?.GetDebugSnapshot();
            if (snapshot == null)
            {
                DeviceAimbotDebugText = "DeviceAimbot Aimbot: not running or no data yet.";
                return;
            }

            var sb = new StringBuilder();
            sb.AppendLine("=== DeviceAimbot Aimbot ===");
            sb.AppendLine($"Status: {snapshot.Status}");
            sb.AppendLine($"Key: {(snapshot.KeyEngaged ? "ENGAGED" : "Idle")} | Enabled: {snapshot.Enabled} | Device: {(snapshot.DeviceConnected ? "Connected" : "Disconnected")}");
            sb.AppendLine($"InRaid: {snapshot.InRaid} | FOV: {snapshot.ConfigFov:F0}px | MaxDist: {snapshot.ConfigMaxDistance:F0}m | Mode: {snapshot.TargetingMode}");
            sb.AppendLine($"Filters -> PMC:{App.Config.Device.TargetPMC} PScav:{App.Config.Device.TargetPlayerScav} AI:{App.Config.Device.TargetAIScav} Boss:{App.Config.Device.TargetBoss} Raider:{App.Config.Device.TargetRaider}");
            sb.AppendLine($"Candidates: total {snapshot.CandidateTotal}, type {snapshot.CandidateTypeOk}, dist {snapshot.CandidateInDistance}, skeleton {snapshot.CandidateWithSkeleton}, w2s {snapshot.CandidateW2S}, final {snapshot.CandidateCount}");
            sb.AppendLine($"Target: {(snapshot.LockedTargetName ?? "None")} [{snapshot.LockedTargetType?.ToString() ?? "-"}] valid={snapshot.TargetValid}");
            if (snapshot.LockedTargetDistance.HasValue)
                sb.AppendLine($"  Dist {snapshot.LockedTargetDistance.Value:F1}m | FOVDist {(float.IsNaN(snapshot.LockedTargetFov) ? "n/a" : snapshot.LockedTargetFov.ToString("F1"))} | Bone {snapshot.TargetBone}");
            sb.AppendLine($"Fireport: {(snapshot.HasFireport ? snapshot.FireportPosition?.ToString() : "None")}");
            var bulletSpeedText = snapshot.BulletSpeed.HasValue ? snapshot.BulletSpeed.Value.ToString("F1") : "?";
            sb.AppendLine($"Ballistics: {(snapshot.BallisticsValid ? $"OK (Speed {bulletSpeedText} m/s, Predict {(snapshot.PredictionEnabled ? "ON" : "OFF")})" : "Invalid/None")}");

            DeviceAimbotDebugText = sb.ToString();

            // InputManager Debug
            var input = Memory.Input;
            if (input != null && input.IsReady)
            {
                InputManagerStatus = input.GetDebugStatus();
                var keys = input.GetPressedKeys();
                InputManagerKeys = keys.Any() ? string.Join(", ", keys.Select(k => ((Win32VirtualKey)k).ToString())) : "None";
            }
            else
            {
                InputManagerStatus = "Not ReadyOrNull";
                InputManagerKeys = "-";
            }
        }

        #region INotifyPropertyChanged
        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string propertyName) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        #endregion
    }
}
