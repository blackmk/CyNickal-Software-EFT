/*
 * Lone EFT DMA Radar
 * MIT License - Copyright (c) 2025 Lone DMA
 */

using System;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using LoneEftDmaRadar.Tarkov;
using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.Unity.Collections;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using LoneEftDmaRadar.Web.TarkovDev.Data;
using VmmSharpEx.Extensions;
using VmmSharpEx.Scatter;

namespace LoneEftDmaRadar.UI.Misc
{
    public sealed class FirearmManager
    {
        private readonly LocalPlayer _localPlayer;
        private CachedHandsInfo _hands;

        /// <summary>
        /// Returns the Hands Controller Address and if the held item is a weapon.
        /// </summary>
        public Tuple<ulong, bool> HandsController => new(_hands, _hands?.IsWeapon ?? false);

        /// <summary>
        /// Cached Hands Information.
        /// </summary>
        public CachedHandsInfo CurrentHands => _hands;

        /// <summary>
        /// Magazine (if any) contained in this firearm.
        /// </summary>
        public MagazineManager Magazine { get; private set; }

        /// <summary>
        /// Current Firearm Fireport Transform.
        /// </summary>
        public UnityTransform FireportTransform { get; private set; }

        /// <summary>
        /// Last known Fireport Position.
        /// </summary>
        public Vector3? FireportPosition { get; private set; }

        /// <summary>
        /// Last known Fireport Rotation.
        /// </summary>
        public Quaternion? FireportRotation { get; private set; }

        public FirearmManager(LocalPlayer localPlayer)
        {
            _localPlayer = localPlayer;
            Magazine = new(localPlayer);
        }

        /// <summary>
        /// Realtime Loop for FirearmManager chained from LocalPlayer.
        /// </summary>
        public void OnRealtimeLoop(VmmScatter scatter)
        {
            if (FireportTransform is UnityTransform fireport)
            {
                // uint size = (uint)(fireport.Count * Unsafe.SizeOf<UnityTransform.TrsX>());
                // scatter.PrepareRead(fireport.VerticesAddr, size);
                scatter.PrepareReadArray<UnityTransform.TrsX>(fireport.VerticesAddr, fireport.Count);

                scatter.Completed += (s, e) =>
                {
                    try
                    {
                        if (e.ReadPooled<UnityTransform.TrsX>(fireport.VerticesAddr, fireport.Count) is IMemoryOwner<UnityTransform.TrsX> vertices)
                        {
                            using (vertices)
                            {
                                UpdateFireport(vertices.Memory.Span);
                            }
                        }
                    }
                    catch
                    {
                        ResetFireport();
                    }
                };
            }
            else if (_hands?.IsWeapon == true)
            {
                // If fireport is null but we have a weapon, we might need to initialize it.
                // However, initialization usually requires reading pointers which is better done in the main Update loop 
                // or a separate slow-scatter loop to avoid blocking operations here if possible.
                // But for now, we rely on Update() to initialize FireportTransform.
            }
        }

        /// <summary>
        /// Update cached fireport position/rotation (called from Scatter Loop).
        /// </summary>
        private void UpdateFireport(Span<UnityTransform.TrsX> vertices)
        {
            try
            {
                if (FireportTransform != null)
                {
                    FireportPosition = FireportTransform.UpdatePosition(vertices);
                    FireportRotation = FireportTransform.GetRotation(vertices);
                }
            }
            catch
            {
                ResetFireport();
            }
        }

        /// <summary>
        /// Update Hands/Firearm/Magazine information for LocalPlayer.
        /// Called from LocalPlayer.UpdateFirearmManager()
        /// </summary>
        public void Update()
        {
            try
            {
                var hands = _localPlayer.HandsController;
                if (!hands.IsValidUserVA())
                    return;

                if (hands != _hands || (_hands != null && _hands.ItemAddr != Memory.ReadPtr(hands + Offsets.ItemHandsController.Item)))
                {
                    _hands = null;
                    ResetFireport();
                    Magazine = new(_localPlayer);
                    _hands = GetHandsInfo(hands);
                }

                if (_hands?.IsWeapon == true)
                {
                    // Update Magazine
                    try
                    {
                         Magazine.Update(_hands);
                    }
                     catch (Exception ex)
                    {
                        DebugLogger.LogDebug($"[FirearmManager] ERROR Updating Magazine: {ex}");
                    }

                    // Validate Fireport
                    if (FireportTransform is UnityTransform fireportTransform)
                    {
                        try
                        {
                            var v = Memory.ReadPtrChain(hands, false, Offsets.FirearmController.To_FirePortVertices);
                            if (fireportTransform.VerticesAddr != v)
                                ResetFireport();
                        }
                        catch
                        {
                            ResetFireport();
                        }
                    }

                    if (FireportTransform is null)
                    {
                        try
                        {
                            var t = Memory.ReadPtrChain(hands, false, Offsets.FirearmController.To_FirePortTransformInternal);
                            FireportTransform = new(t, false);

                            // Initial update to validate distance
                            var pos = FireportTransform.UpdatePosition();
                            FireportRotation = FireportTransform.GetRotation();
                            
                            if (Vector3.Distance(pos, _localPlayer.Position) > 100f)
                            {
                                ResetFireport();
                                return;
                            }
                            FireportPosition = pos;
                            FireportRotation = FireportRotation;
                        }
                        catch
                        {
                            ResetFireport();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[FirearmManager] ERROR: {ex}");
            }
        }

        /// <summary>
        /// Reset the Fireport Data.
        /// </summary>
        private void ResetFireport()
        {
            FireportTransform = null;
            FireportPosition = null;
            FireportRotation = null;
        }

        /// <summary>
        /// Get updated hands information.
        /// </summary>
        private static CachedHandsInfo GetHandsInfo(ulong handsController)
        {
            try
            {
                var itemBase = Memory.ReadPtr(handsController + Offsets.ItemHandsController.Item, false);
                // Basic validation
                if (itemBase == 0) return new(handsController);

                var itemTemp = Memory.ReadPtr(itemBase + Offsets.LootItem.Template, false);
                if (itemTemp == 0) return new(handsController);

                var itemIdPtr = Memory.ReadValue<MongoID>(itemTemp + Offsets.ItemTemplate._id, false);
                var itemId = itemIdPtr.ReadString(64, false);
                
                if (string.IsNullOrEmpty(itemId)) return new(handsController);
                
                if (!TarkovDataManager.AllItems.TryGetValue(itemId, out var heldItem))
                    return new(handsController);
                
                return new(handsController, heldItem, itemBase, itemId);
            }
            catch
            {
                // Return basic info without item data on read failure (prevents crash loops)
                return new(handsController);
            }
        }

        #region Magazine Info

        /// <summary>
        /// Helper class to track a Player's Magazine Ammo Count.
        /// </summary>
        public sealed class MagazineManager
        {
            private readonly LocalPlayer _localPlayer;
            private string _fireType;
            private string _ammo;

            /// <summary>
            /// True if the MagazineManager is in a valid state for data output.
            /// </summary>
            public bool IsValid => MaxCount > 0;
            /// <summary>
            /// Current ammo count in Magazine.
            /// </summary>
            public int Count { get; private set; }
            /// <summary>
            /// Maximum ammo count in Magazine.
            /// </summary>
            public int MaxCount { get; private set; }
            /// <summary>
            /// Weapon Fire Mode & Ammo Type in a formatted string.
            /// </summary>
            public string WeaponInfo
            {
                get
                {
                    string result = "";
                    string ft = _fireType;
                    string ammo = _ammo;
                    if (ft is not null)
                        result += $"{ft}: ";
                    if (ammo is not null)
                        result += ammo;
                    if (string.IsNullOrEmpty(result))
                        return null;
                    return result.Trim().TrimEnd(':');
                }
            }

            /// <summary>
            /// Constructor.
            /// </summary>
            /// <param name="player">Player to track magazine usage for.</param>
            public MagazineManager(LocalPlayer localPlayer)
            {
                _localPlayer = localPlayer;
            }

            /// <summary>
            /// Update Magazine Information for this instance.
            /// </summary>
            public void Update(CachedHandsInfo hands)
            {
                string ammoInChamber = null;
                string fireType = null;
                int maxCount = 0;
                int currentCount = 0;
                var fireModePtr = Memory.ReadValue<ulong>(hands.ItemAddr + Offsets.LootItemWeapon.FireMode);
                var chambersPtr = Memory.ReadValue<ulong>(hands.ItemAddr + Offsets.LootItemWeapon.Chambers);
                var magSlotPtr = Memory.ReadValue<ulong>(hands.ItemAddr + Offsets.LootItemWeapon._magSlotCache);
                if (fireModePtr != 0x0)
                {
                    var fireMode = (EFireMode)Memory.ReadValue<byte>(fireModePtr + Offsets.FireModeComponent.FireMode);
                    if (fireMode >= EFireMode.Auto && fireMode <= EFireMode.SemiAuto)
                        fireType = fireMode.GetDescription();
                }
                if (chambersPtr != 0x0) // Single chamber, or for some shotguns, multiple chambers
                {
                    using var chambers = UnityArray<Chamber>.Create(chambersPtr);
                    currentCount += chambers.Count(x => x.HasBullet());
                    ammoInChamber = GetLoadedAmmoName(chambers.FirstOrDefault(x => x.HasBullet()));
                    maxCount += chambers.Count;
                }
                if (magSlotPtr != 0x0)
                {
                    var magItem = Memory.ReadValue<ulong>(magSlotPtr + Offsets.Slot.ContainedItem);
                    if (magItem != 0x0)
                    {
                        var magChambersPtr = Memory.ReadPtr(magItem + Offsets.LootItemMod.Slots);
                        using var magChambers = UnityArray<Chamber>.Create(magChambersPtr);
                        if (magChambers.Count > 0) // Revolvers, etc.
                        {
                            maxCount += magChambers.Count;
                            currentCount += magChambers.Count(x => x.HasBullet());
                            ammoInChamber = GetLoadedAmmoName(magChambers.FirstOrDefault(x => x.HasBullet()));
                        }
                        else // Regular magazines
                        {
                            var cartridges = Memory.ReadPtr(magItem + Offsets.MagazineItem.Cartridges);
                            maxCount += Memory.ReadValue<int>(cartridges + Offsets.StackSlot.MaxCount);
                            var magStackPtr = Memory.ReadPtr(cartridges + Offsets.StackSlot._items);
                            using var magStack = UnityList<ulong>.Create(magStackPtr);
                            foreach (var stack in magStack) // Each ammo type will be a separate stack
                            {
                                if (stack != 0x0)
                                    currentCount += Memory.ReadValue<int>(stack + Offsets.MagazineClass.StackObjectsCount, false);
                            }
                        }
                    }
                }
                _ammo = ammoInChamber;
                _fireType = fireType;
                Count = currentCount;
                MaxCount = maxCount;
            }

            /// <summary>
            /// Gets the name of the ammo round currently loaded in this chamber, otherwise NULL.
            /// </summary>
            /// <param name="chamber">Chamber to check.</param>
            /// <returns>Short name of ammo in chamber, or null if no round loaded.</returns>
            private static string GetLoadedAmmoName(Chamber chamber)
            {
                if (chamber != 0x0)
                {
                    var bulletItem = Memory.ReadValue<ulong>(chamber + Offsets.Slot.ContainedItem);
                    if (bulletItem != 0x0)
                    {
                        var bulletTemp = Memory.ReadPtr(bulletItem + Offsets.LootItem.Template);
                        var idPtr = Memory.ReadValue<MongoID>(bulletTemp + Offsets.ItemTemplate._id);
                        string id = idPtr.ReadString();
                        if (TarkovDataManager.AllItems.TryGetValue(id, out var bullet))
                            return bullet?.ShortName;
                    }
                }
                return null;
            }

            /// <summary>
            /// Returns the Ammo Template from a Weapon (First loaded round).
            /// </summary>
            /// <param name="lootItemBase">EFT.InventoryLogic.Weapon instance</param>
            /// <returns>Ammo Template Ptr</returns>
            public static ulong GetAmmoTemplateFromWeapon(ulong lootItemBase)
            {
                var chambersPtr = Memory.ReadValue<ulong>(lootItemBase + Offsets.LootItemWeapon.Chambers);
                ulong firstRound;
                UnityArray<Chamber> chambers = null;
                UnityArray<Chamber> magChambers = null;
                UnityList<ulong> magStack = null;
                try
                {
                    if (chambersPtr != 0x0 && (chambers = UnityArray<Chamber>.Create(chambersPtr)).Count > 0) // Single chamber, or for some shotguns, multiple chambers
                        firstRound = Memory.ReadPtr(chambers.First(x => x.HasBullet(true)) + Offsets.Slot.ContainedItem);
                    else
                    {
                        var magSlot = Memory.ReadPtr(lootItemBase + Offsets.LootItemWeapon._magSlotCache);
                        var magItemPtr = Memory.ReadPtr(magSlot + Offsets.Slot.ContainedItem);
                        var magChambersPtr = Memory.ReadPtr(magItemPtr + Offsets.LootItemMod.Slots);
                        magChambers = UnityArray<Chamber>.Create(magChambersPtr);
                        if (magChambers.Count > 0) // Revolvers, etc.
                            firstRound = Memory.ReadPtr(magChambers.First(x => x.HasBullet(true)) + Offsets.Slot.ContainedItem);
                        else // Regular magazines
                        {
                            var cartridges = Memory.ReadPtr(magItemPtr + Offsets.MagazineItem.Cartridges);
                            var magStackPtr = Memory.ReadPtr(cartridges + Offsets.StackSlot._items);
                            magStack = UnityList<ulong>.Create(magStackPtr);
                            firstRound = magStack[0];
                        }
                    }
                    return Memory.ReadPtr(firstRound + Offsets.LootItem.Template);
                }
                finally
                {
                    chambers?.Dispose();
                    magChambers?.Dispose();
                    magStack?.Dispose();
                }
            }

            /// <summary>
            /// Wrapper defining a Chamber Structure.
            /// </summary>
            [StructLayout(LayoutKind.Sequential, Pack = 1)]
            private readonly struct Chamber
            {
                public static implicit operator ulong(Chamber x) => x._base;
                private readonly ulong _base;

                public readonly bool HasBullet(bool useCache = false)
                {
                    if (_base == 0x0)
                        return false;
                    return Memory.ReadValue<ulong>(_base + Offsets.Slot.ContainedItem, useCache) != 0x0;
                }
            }

            private enum EFireMode : byte
            {
                // Token: 0x0400B0EE RID: 45294
                [Description(nameof(Auto))]
                Auto = 0,
                // Token: 0x0400B0EF RID: 45295
                [Description(nameof(Single))]
                Single = 1,
                // Token: 0x0400B0F0 RID: 45296
                [Description(nameof(DbTap))]
                DbTap = 2,
                // Token: 0x0400B0F1 RID: 45297
                [Description(nameof(Burst))]
                Burst = 3,
                // Token: 0x0400B0F2 RID: 45298
                [Description(nameof(DbAction))]
                DbAction = 4,
                // Token: 0x0400B0F3 RID: 45299
                [Description(nameof(SemiAuto))]
                SemiAuto = 5
            }
        }

        #endregion

        #region Hands Cache

        public sealed class CachedHandsInfo
        {
            public static implicit operator ulong(CachedHandsInfo x) => x?._hands ?? 0x0;

            private readonly ulong _hands;
            private readonly TarkovMarketItem _item;

            /// <summary>
            /// Address of currently held item (if any).
            /// </summary>
            public ulong ItemAddr { get; }

            /// <summary>
            /// BSG Item ID (24 character string).
            /// </summary>
            public string ItemId { get; }

            /// <summary>
            /// True if the Item being currently held (if any) is a weapon, otherwise False.
            /// </summary>
            public bool IsWeapon => _item?.IsWeapon ?? false;

            public CachedHandsInfo(ulong handsController)
            {
                _hands = handsController;
            }

            public CachedHandsInfo(ulong handsController, TarkovMarketItem item, ulong itemAddr, string itemId)
            {
                _hands = handsController;
                _item = item;
                ItemAddr = itemAddr;
                ItemId = itemId;
            }
        }

        #endregion
    }

    public static class EnumExtension
    {
        public static string GetDescription(this Enum value)
        {
            var field = value.GetType().GetField(value.ToString());
            if (field == null) return value.ToString();
            var attribute = Attribute.GetCustomAttribute(field, typeof(DescriptionAttribute)) as DescriptionAttribute;
            return attribute == null ? value.ToString() : attribute.Description;
        }
    }
}
