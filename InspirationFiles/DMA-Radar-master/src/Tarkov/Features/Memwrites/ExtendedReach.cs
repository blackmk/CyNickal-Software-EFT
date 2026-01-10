using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.UI.Misc;
using SDK;
using System;
using VmmSharpEx.Extensions;

namespace LoneEftDmaRadar.Tarkov.Features.MemWrites
{
    public sealed class ExtendedReach : MemWriteFeature<ExtendedReach>
    {
        private bool _lastEnabledState;
        private float _lastDistance;
        private ulong _cachedEFTHardSettingsInstance;

        private const float ORIGINAL_LOOT_RAYCAST_DISTANCE = 1.3f;
        private const float ORIGINAL_DOOR_RAYCAST_DISTANCE = 1.2f;

        public override bool Enabled
        {
            get => App.Config.MemWrites.ExtendedReach.Enabled;
            set => App.Config.MemWrites.ExtendedReach.Enabled = value;
        }

        protected override TimeSpan Delay => TimeSpan.FromMilliseconds(500);

        public override void TryApply(LocalPlayer localPlayer)
        {
            try
            {
                var hardSettingsInstance = GetEFTHardSettingsInstance();
                if (!hardSettingsInstance.IsValidUserVA())
                {
                    return;
                }

                var currentDistance = App.Config.MemWrites.ExtendedReach.Distance;
                var stateChanged = Enabled != _lastEnabledState;
                var distanceChanged = Math.Abs(currentDistance - _lastDistance) > 0.001f;

                if ((Enabled && (stateChanged || distanceChanged)) || (!Enabled && stateChanged))
                {
                    ApplyReachSettings(hardSettingsInstance, Enabled, currentDistance);

                    var wasEnabled = _lastEnabledState;
                    _lastEnabledState = Enabled;
                    _lastDistance = currentDistance;

                    if (Enabled)
                    {
                        if (!wasEnabled)
                            DebugLogger.LogDebug($"[ExtendedReach] Enabled (Distance: {currentDistance:F1})");
                        else if (distanceChanged)
                            DebugLogger.LogDebug($"[ExtendedReach] Distance Updated to {currentDistance:F1}");
                    }
                    else
                    {
                        DebugLogger.LogDebug("[ExtendedReach] Disabled");
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[ExtendedReach]: {ex}");
                _cachedEFTHardSettingsInstance = default;
            }
        }

        private ulong GetEFTHardSettingsInstance()
        {
            if (_cachedEFTHardSettingsInstance.IsValidUserVA())
                return _cachedEFTHardSettingsInstance;

            try
            {
                //TODO: Implement
                return 0x0;
            }
            catch
            {
                return 0x0;
            }
        }

        private static void ApplyReachSettings(ulong hardSettingsInstance, bool enabled, float distance)
        {
            var (lootDistance, doorDistance) = enabled
                ? (distance, distance)
                : (ORIGINAL_LOOT_RAYCAST_DISTANCE, ORIGINAL_DOOR_RAYCAST_DISTANCE);

            Memory.WriteValue<float>(hardSettingsInstance + Offsets.EFTHardSettings.LOOT_RAYCAST_DISTANCE, lootDistance);
            Memory.WriteValue<float>(hardSettingsInstance + Offsets.EFTHardSettings.DOOR_RAYCAST_DISTANCE, doorDistance);
        }

        public override void OnRaidStart()
        {
            _lastEnabledState = default;
            _lastDistance = default;
            _cachedEFTHardSettingsInstance = default;
        }
    }
}
