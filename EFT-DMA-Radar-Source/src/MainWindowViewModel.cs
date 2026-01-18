/*
 * Lone EFT DMA Radar
 * Brought to you by Lone (Lone DMA)
 * 
MIT License

Copyright (c) 2025 Lone DMA

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 *
*/

using LoneEftDmaRadar.UI.Hotkeys;
using LoneEftDmaRadar.UI.Radar.ViewModels;
using LoneEftDmaRadar.UI.ESP;
using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.UI.Misc;

namespace LoneEftDmaRadar
{
    public sealed class MainWindowViewModel
    {
        private readonly MainWindow _parent;
        //public event PropertyChangedEventHandler PropertyChanged;

        public MainWindowViewModel(MainWindow parent)
        {
            _parent = parent ?? throw new ArgumentNullException(nameof(parent));
            EnsureHotkeysRegistered();
        }

        public void ToggleFullscreen(bool toFullscreen)
        {
            if (toFullscreen)
            {
                // Full‐screen
                _parent.WindowStyle = WindowStyle.None;
                _parent.ResizeMode = ResizeMode.NoResize;
                _parent.Topmost = true;
                _parent.WindowState = WindowState.Maximized;
            }
            else
            {
                _parent.WindowStyle = WindowStyle.SingleBorderWindow;
                _parent.ResizeMode = ResizeMode.CanResize;
                _parent.Topmost = false;
                _parent.WindowState = WindowState.Normal;
            }
        }

        #region Hotkey Manager

        private const int HK_ZOOMTICKAMT = 1; // amt to zoom
        private const int HK_ZOOMTICKDELAY = 25; // ms

        /// <summary>
        /// Loads Hotkey Manager resources.
        /// Only call from Primary Thread/Window (ONCE!)
        /// </summary>
        private bool _hotkeysRegistered;

        internal void EnsureHotkeysRegistered()
        {
            if (_hotkeysRegistered)
                return;
            LoadHotkeyManager();
            _hotkeysRegistered = true;
        }

        private void LoadHotkeyManager()
        {
            var zoomIn = new HotkeyActionController("Zoom In");
            zoomIn.Delay = HK_ZOOMTICKDELAY;
            zoomIn.HotkeyDelayElapsed += ZoomIn_HotkeyDelayElapsed;
            var zoomOut = new HotkeyActionController("Zoom Out");
            zoomOut.Delay = HK_ZOOMTICKDELAY;
            zoomOut.HotkeyDelayElapsed += ZoomOut_HotkeyDelayElapsed;
            var switchFollowTarget = new HotkeyActionController("Switch Follow Target");
            switchFollowTarget.HotkeyStateChanged += SwitchFollowTarget_HotkeyStateChanged;
            HotkeyAction.RegisterController(switchFollowTarget);
            var toggleLoot = new HotkeyActionController("Toggle Loot");
            toggleLoot.HotkeyStateChanged += ToggleLoot_HotkeyStateChanged;
            var toggleAimviewWidget = new HotkeyActionController("Toggle Aimview Widget");
            toggleAimviewWidget.HotkeyStateChanged += ToggleAimviewWidget_HotkeyStateChanged;
            var toggleInfo = new HotkeyActionController("Toggle Game Info Tab");
            toggleInfo.HotkeyStateChanged += ToggleInfo_HotkeyStateChanged;
            var toggleShowFood = new HotkeyActionController("Toggle Show Food");
            toggleShowFood.HotkeyStateChanged += ToggleShowFood_HotkeyStateChanged;
            var toggleShowMeds = new HotkeyActionController("Toggle Show Meds");
            toggleShowMeds.HotkeyStateChanged += ToggleShowMeds_HotkeyStateChanged;
            var toggleShowQuestItems = new HotkeyActionController("Toggle Show Quest Items");
            toggleShowQuestItems.HotkeyStateChanged += ToggleShowQuestItems_HotkeyStateChanged;
            var engageAimbotDeviceAimbot = new HotkeyActionController("Engage Aimbot");
            engageAimbotDeviceAimbot.HotkeyStateChanged += EngageAimbotDeviceAimbot_HotkeyStateChanged;
            var toggleESPLoot = new HotkeyActionController("Toggle ESP Loot");
            toggleESPLoot.HotkeyStateChanged += ToggleESPLoot_HotkeyStateChanged;
            var toggleStaticContainers = new HotkeyActionController("Toggle Static Containers");
            toggleStaticContainers.HotkeyStateChanged += ToggleStaticContainers_HotkeyStateChanged;
            var addNearbyTeammates = new HotkeyActionController("Add Nearby Players as Teammates");
            addNearbyTeammates.HotkeyStateChanged += AddNearbyTeammates_HotkeyStateChanged;

            // Add to Static Collection:
            HotkeyAction.RegisterController(zoomIn);
            HotkeyAction.RegisterController(zoomOut);
            HotkeyAction.RegisterController(toggleLoot);
            HotkeyAction.RegisterController(toggleAimviewWidget);
            HotkeyAction.RegisterController(toggleInfo);
            HotkeyAction.RegisterController(toggleShowFood);
            HotkeyAction.RegisterController(toggleShowMeds);
            HotkeyAction.RegisterController(toggleShowQuestItems);
            HotkeyAction.RegisterController(toggleESPLoot);
            HotkeyAction.RegisterController(toggleStaticContainers);
            HotkeyAction.RegisterController(engageAimbotDeviceAimbot);
            HotkeyAction.RegisterController(addNearbyTeammates);
            HotkeyManagerViewModel.NotifyControllersRegistered();
        }

        internal static void EnsureHotkeysRegisteredStatic()
        {
            MainWindow.Instance?.ViewModel?.EnsureHotkeysRegistered();
        }

        private void ToggleAimviewWidget_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Settings?.ViewModel is SettingsViewModel vm)
                vm.AimviewWidget = !vm.AimviewWidget;
        }

        private void ToggleShowMeds_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Radar?.Overlay?.ViewModel is RadarOverlayViewModel vm)
            {
                vm.ShowMeds = !vm.ShowMeds;
            }
        }

        private void ToggleShowQuestItems_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Radar?.Overlay?.ViewModel is RadarOverlayViewModel vm)
            {
                vm.ShowQuestItems = !vm.ShowQuestItems;
            }
        }

        private void EngageAimbotDeviceAimbot_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            // Set the engaged state directly on the DeviceAimbot instance.
            // Fallback to ViewModel-based approach if direct access fails.
            try
            {
                if (Memory.DeviceAimbot != null)
                {
                    Memory.DeviceAimbot.IsEngaged = e.State;
                    return;
                }
            }
            catch { /* fallthrough to ViewModel approach */ }

            // Legacy fallback via ViewModel
            if (_parent.DeviceAimbot?.ViewModel is DeviceAimbotViewModel DeviceAimbotAim)
            {
                DeviceAimbotAim.IsEngaged = e.State;
            }
        }

        private void ToggleShowFood_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Radar?.Overlay?.ViewModel is RadarOverlayViewModel vm)
            {
                vm.ShowFood = !vm.ShowFood;
            }
        }

        private void ToggleInfo_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Settings?.ViewModel is SettingsViewModel vm)
                vm.PlayerInfoWidget = !vm.PlayerInfoWidget;
        }

        private void ToggleLoot_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Settings?.ViewModel is SettingsViewModel vm)
                vm.ShowLoot = !vm.ShowLoot;
        }

        private void ZoomOut_HotkeyDelayElapsed(object sender, EventArgs e)
        {
            _parent.Radar?.ViewModel?.ZoomOut(HK_ZOOMTICKAMT);
        }

        private void ZoomIn_HotkeyDelayElapsed(object sender, EventArgs e)
        {
            _parent.Radar?.ViewModel?.ZoomIn(HK_ZOOMTICKAMT);
        }

        private void SwitchFollowTarget_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State)
            {
                _parent.Radar?.ViewModel?.SwitchFollowTarget();
            }
        }

        private void ToggleESPLoot_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State)
            {
                App.Config.UI.EspLoot = !App.Config.UI.EspLoot;
            }
        }

        private void ToggleStaticContainers_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (e.State && _parent.Settings?.ViewModel is SettingsViewModel vm)
            {
                vm.ShowStaticContainers = !vm.ShowStaticContainers;
            }
        }

        private void AddNearbyTeammates_HotkeyStateChanged(object sender, HotkeyEventArgs e)
        {
            if (!e.State)
                return;

            var localPlayer = Memory.LocalPlayer;
            if (localPlayer == null)
                return;

            const float maxDistance = 15f;
            int count = 0;

            foreach (var player in Memory.Players ?? Enumerable.Empty<AbstractPlayer>())
            {
                if (player == localPlayer || player is LocalPlayer)
                    continue;
                if (!player.IsAlive || !player.IsActive)
                    continue;
                if (!player.IsHuman)
                    continue;
                if (AbstractPlayer.IsTempTeammate(player))
                    continue;

                float distance = Vector3.Distance(localPlayer.Position, player.Position);
                if (distance <= maxDistance)
                {
                    AbstractPlayer.AddTempTeammate(player);
                    count++;
                    DebugLogger.LogDebug($"Added {player.Name ?? "Unknown"} (dist: {distance:F1}m) as temporary teammate");
                }
            }

            DebugLogger.LogDebug($"Added {count} nearby players as temporary teammates");
        }

        #endregion
    }
}
