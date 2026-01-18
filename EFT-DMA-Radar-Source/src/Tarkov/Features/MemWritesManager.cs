using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.UI.Misc;
using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace LoneEftDmaRadar.Tarkov.Features.MemWrites
{
    /// <summary>
    /// Manages all memory write features.
    /// </summary>
    public sealed class MemWritesManager
    {
        private readonly List<Action<LocalPlayer>> _raidFeatures = new();
        private DateTime _lastAntiAfkRun = DateTime.MinValue;
        private static readonly TimeSpan AntiAfkDelay = TimeSpan.FromSeconds(5);

        public MemWritesManager()
        {
            // Register raid-only features
            _raidFeatures.Add(lp => NoRecoil.Instance.ApplyIfReady(lp));
            _raidFeatures.Add(lp => InfiniteStamina.Instance.ApplyIfReady(lp));
            _raidFeatures.Add(lp => MemoryAim.Instance.ApplyIfReady(lp));
            _raidFeatures.Add(lp => ExtendedReach.Instance.ApplyIfReady(lp));
            _raidFeatures.Add(lp => MuleMode.Instance.ApplyIfReady(lp));
        }

        /// <summary>
        /// Apply all enabled memory write features.
        /// Called from the main game loop.
        /// </summary>
        public void Apply(LocalPlayer localPlayer)
        {
            if (!App.Config.MemWrites.Enabled)
                return;

            TryApplyAntiAfk();

            if (!Memory.InRaid || localPlayer == null)
                return;

            try
            {
                foreach (var feature in _raidFeatures)
                {
                    try
                    {
                        feature(localPlayer);
                    }
                    catch (Exception ex)
                    {
                        DebugLogger.LogDebug($"[MemWritesManager] Feature error: {ex}");
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[MemWritesManager] Apply error: {ex}");
            }
        }

        /// <summary>
        /// Try to apply AntiAfk (runs in menu).
        /// </summary>
        private void TryApplyAntiAfk()
        {
            if (Memory.InRaid)
                return;

            try
            {
                var now = DateTime.UtcNow;
                if (now - _lastAntiAfkRun < AntiAfkDelay)
                    return;
                _lastAntiAfkRun = now;
                AntiAfk.Instance.ApplyIfReady(null!);
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[MemWritesManager] AntiAfk error: {ex.Message}");
            }
        }

        /// <summary>
        /// Called when raid starts.
        /// </summary>
        public void OnRaidStart()
        {
            NoRecoil.Instance.OnRaidStart();
            InfiniteStamina.Instance.OnRaidStart();
            MemoryAim.Instance.OnRaidStart();
            ExtendedReach.Instance.OnRaidStart();
            MuleMode.Instance.OnRaidStart();
            AntiAfk.Instance.OnRaidStart();
        }
    }
}
