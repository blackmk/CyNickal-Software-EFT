using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.Unity;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using LoneEftDmaRadar.UI.Misc;
using SDK;
using System;
using VmmSharpEx.Extensions;

namespace LoneEftDmaRadar.Tarkov.Features.MemWrites
{
    public sealed class AntiAfk : MemWriteFeature<AntiAfk>
    {
        private bool _lastEnabledState;
        private bool _applied;
        private const float AFK_DELAY = 604800f; // 1 week

        public override bool Enabled
        {
            get => App.Config.MemWrites.AntiAfkEnabled;
            set => App.Config.MemWrites.AntiAfkEnabled = value;
        }

        protected override TimeSpan Delay => TimeSpan.FromSeconds(5);

        public override void TryApply(LocalPlayer localPlayer)
        {
            try
            {
                if (Enabled && !_applied)
                {
                    Apply();
                    _lastEnabledState = Enabled;
                    _applied = true;
                    DebugLogger.LogDebug("[AntiAfk] Successfully applied!");
                }
                else if (!Enabled && _lastEnabledState)
                {
                    _lastEnabledState = false;
                    _applied = false;
                    DebugLogger.LogDebug("[AntiAfk] Disabled");
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[AntiAfk] Error: {ex.Message}");
                _applied = false;
            }
        }

        private void Apply()
        {
            DebugLogger.LogDebug("[AntiAfk] Starting apply...");
            
            var gomAddress = GameObjectManager.GetAddr(Memory.UnityBase);
            var gom = Memory.ReadValue<GameObjectManager>(gomAddress);
            DebugLogger.LogDebug($"[AntiAfk] GOM at 0x{gomAddress:X}");
            
            var applicationGO = gom.GetObjectFromList("Application (Main Client)");
            if (applicationGO == 0)
                throw new Exception("Application GO not found in GOM");
            DebugLogger.LogDebug($"[AntiAfk] Application GO at 0x{applicationGO:X}");

            var go = Memory.ReadValue<GameObject>(applicationGO);
            DebugLogger.LogDebug($"[AntiAfk] GameObject.Components at 0x{go.Components:X}");
            
            var componentArr = Memory.ReadValue<DynamicArray>(go.Components);
            DebugLogger.LogDebug($"[AntiAfk] ComponentArray Size={componentArr.Size}, FirstValue=0x{componentArr.FirstValue:X}");
            
            var size = Math.Min((int)componentArr.Size, 50);
            if (size <= 0) 
                throw new Exception($"No components found (size={componentArr.Size})");

            using var components = Memory.ReadPooled<DynamicArray.Entry>(componentArr.FirstValue, size);
            DebugLogger.LogDebug($"[AntiAfk] Read {size} component entries");
            
            ulong tarkovAppPtr = 0;
            int componentIndex = 0;
            
            foreach (var comp in components.Memory.Span)
            {
                var compPtr = comp.Component;
                if (compPtr == 0) 
                {
                    componentIndex++;
                    continue;
                }
                 
                try
                {
                    var objectClass = Memory.ReadPtr(compPtr + UnitySDK.UnityOffsets.Component_ObjectClassOffset);
                    if (!objectClass.IsValidUserVA()) 
                    {
                        componentIndex++;
                        continue;
                    }
                     
                    var name = ObjectClass.ReadName(objectClass);
                    if (name.Equals("TarkovApplication", StringComparison.OrdinalIgnoreCase))
                    {
                        tarkovAppPtr = objectClass; 
                        DebugLogger.LogDebug($"[AntiAfk] Found TarkovApplication at component[{componentIndex}], ObjectClass=0x{objectClass:X}");
                        break;
                    }
                }
                catch
                {
                }
                componentIndex++;
            }

            if (!tarkovAppPtr.IsValidUserVA())
                throw new Exception($"TarkovApplication not found in {size} components");
            DebugLogger.LogDebug($"[AntiAfk] Reading MenuOperation from 0x{tarkovAppPtr:X} + 0x{Offsets.TarkovApplication.MenuOperation:X}");
            var menuOp = Memory.ReadPtr(tarkovAppPtr + Offsets.TarkovApplication.MenuOperation);
            if (!menuOp.IsValidUserVA())
                throw new Exception($"MenuOperation is null (read from 0x{tarkovAppPtr + Offsets.TarkovApplication.MenuOperation:X})");
            DebugLogger.LogDebug($"[AntiAfk] MenuOperation at 0x{menuOp:X}");

            DebugLogger.LogDebug($"[AntiAfk] Reading AfkMonitor from 0x{menuOp:X} + 0x{Offsets.MenuOperation.AfkMonitor:X}");
            var afkMonitor = Memory.ReadPtr(menuOp + Offsets.MenuOperation.AfkMonitor);
            if (!afkMonitor.IsValidUserVA())
            {
                DebugLogger.LogDebug($"[AntiAfk] AfkMonitor is null - may not be available in-raid (expected in menu only)");
                _applied = true;
                return;
            }
            DebugLogger.LogDebug($"[AntiAfk] AfkMonitor at 0x{afkMonitor:X}");
            DebugLogger.LogDebug($"[AntiAfk] Writing {AFK_DELAY}f to 0x{afkMonitor + Offsets.AfkMonitor.Delay:X}");
            Memory.WriteValue(afkMonitor + Offsets.AfkMonitor.Delay, AFK_DELAY);
        }

        public override void OnRaidStart()
        {
            _applied = false;
        }
    }
}
