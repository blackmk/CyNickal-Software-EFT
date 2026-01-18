using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using LoneEftDmaRadar.UI.Misc;
using SDK;
using System;
using VmmSharpEx.Extensions;

namespace LoneEftDmaRadar.Tarkov.Features.MemWrites
{
    public sealed class MuleMode : MemWriteFeature<MuleMode>
    {
        private bool _lastEnabledState;
        private ulong _cachedPhysical;

        private const float MULE_OVERWEIGHT = 0f;
        private const float MULE_WALK_OVERWEIGHT = 0f;
        private const float MULE_WALK_SPEED_LIMIT = 1f;
        private const float MULE_INERTIA = 0.01f;
        private const float MULE_SPRINT_WEIGHT_FACTOR = 0f;
        private const float MULE_SPRINT_ACCELERATION = 1f;
        private const float MULE_PRE_SPRINT_ACCELERATION = 3f;
        private const float MULE_STATE_SPEED_LIMIT = 1f;
        private const float MULE_STATE_SPRINT_SPEED_LIMIT = 1f;
        private const byte MULE_IS_OVERWEIGHT = 0;

        public override bool Enabled
        {
            get => App.Config.MemWrites.MuleModeEnabled;
            set => App.Config.MemWrites.MuleModeEnabled = value;
        }

        protected override TimeSpan Delay => TimeSpan.FromSeconds(1);

        public override void TryApply(LocalPlayer localPlayer)
        {
            try
            {
                if (localPlayer == null)
                    return;

                if (Enabled && !_lastEnabledState)
                {
                    _lastEnabledState = true;
                    DebugLogger.LogDebug("[MuleMode] Enabled - starting continuous application");
                }
                else if (!Enabled && _lastEnabledState)
                {
                    _lastEnabledState = false;
                    _cachedPhysical = default;
                    DebugLogger.LogDebug("[MuleMode] Disabled");
                    return;
                }

                if (!Enabled)
                    return;

                var physical = GetPhysical(localPlayer);
                if (!physical.IsValidUserVA())
                {
                    DebugLogger.LogDebug("[MuleMode] Physical address invalid!");
                    return;
                }

                var movementContext = Memory.ReadPtr(localPlayer + Offsets.Player.MovementContext);
                if (!movementContext.IsValidUserVA())
                {
                    DebugLogger.LogDebug("[MuleMode] MovementContext address invalid!");
                    return;
                }

                ApplyMuleSettings(physical, movementContext);
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[MuleMode] Error: {ex.Message}");
                _cachedPhysical = default;
            }
        }

        private ulong GetPhysical(LocalPlayer localPlayer)
        {
            if (_cachedPhysical.IsValidUserVA())
                return _cachedPhysical;

            var physical = Memory.ReadPtr(localPlayer + Offsets.Player.Physical);
            if (!physical.IsValidUserVA())
            {
                DebugLogger.LogDebug($"[MuleMode] Failed to read Physical from LocalPlayer+0x{Offsets.Player.Physical:X}");
                return 0x0;
            }

            DebugLogger.LogDebug($"[MuleMode] Found Physical at 0x{physical:X}");
            _cachedPhysical = physical;
            return physical;
        }

        private static void ApplyMuleSettings(ulong physical, ulong movementContext)
        {
            try 
            {
                var currentOverweight = Memory.ReadValue<float>(physical + Offsets.Physical.Overweight);
            
                var currentBaseOverweightLimits = Memory.ReadValue<Vector2>(physical + Offsets.Physical.BaseOverweightLimits);
                var overweightLimits = new Vector2(currentBaseOverweightLimits.Y - 1f, currentBaseOverweightLimits.Y);

                Memory.WriteValue(physical + Offsets.Physical.Overweight, MULE_OVERWEIGHT);
                Memory.WriteValue(physical + Offsets.Physical.WalkOverweight, MULE_WALK_OVERWEIGHT);
                Memory.WriteValue(physical + Offsets.Physical.WalkSpeedLimit, MULE_WALK_SPEED_LIMIT);
                Memory.WriteValue(physical + Offsets.Physical.Inertia, MULE_INERTIA);
                Memory.WriteValue(physical + Offsets.Physical.SprintWeightFactor, MULE_SPRINT_WEIGHT_FACTOR);
                Memory.WriteValue(physical + Offsets.Physical.SprintAcceleration, MULE_SPRINT_ACCELERATION);
                Memory.WriteValue(physical + Offsets.Physical.PreSprintAcceleration, MULE_PRE_SPRINT_ACCELERATION);
                Memory.WriteValue(physical + Offsets.Physical.BaseOverweightLimits, overweightLimits);
                Memory.WriteValue(physical + Offsets.Physical.SprintOverweightLimits, overweightLimits);
                Memory.WriteValue(physical + Offsets.Physical.IsOverweightA, MULE_IS_OVERWEIGHT);
                Memory.WriteValue(physical + Offsets.Physical.IsOverweightB, MULE_IS_OVERWEIGHT);
                
                Memory.WriteValue(movementContext + Offsets.MovementContext.StateSpeedLimit, MULE_STATE_SPEED_LIMIT);
                Memory.WriteValue(movementContext + Offsets.MovementContext.StateSprintSpeedLimit, MULE_STATE_SPRINT_SPEED_LIMIT);
            }
            catch (Exception ex)
            {
                 DebugLogger.LogDebug($"[MuleMode] Apply failed: {ex.Message}");
            }
        }

        public override void OnRaidStart()
        {
            _lastEnabledState = default;
            _cachedPhysical = default;
        }
    }
}
