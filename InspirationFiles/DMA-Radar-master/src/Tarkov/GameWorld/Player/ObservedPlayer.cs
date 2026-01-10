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

using System.Collections.Generic;
using LoneEftDmaRadar.Misc.Services;
using LoneEftDmaRadar.Tarkov.GameWorld.Player.Helpers;
using LoneEftDmaRadar.Tarkov.Unity.Collections;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using LoneEftDmaRadar.UI.Misc;
using LoneEftDmaRadar.UI.Radar.ViewModels;
using VmmSharpEx.Scatter;

namespace LoneEftDmaRadar.Tarkov.GameWorld.Player
{
    public class ObservedPlayer : AbstractPlayer
    {
        /// <summary>
        /// Player's Current Items.
        /// </summary>
        public PlayerEquipment Equipment { get; }
        /// <summary>
        /// Address of InventoryController field.
        /// </summary>
        public ulong InventoryControllerAddr { get; }
        /// <summary>
        /// Hands Controller field address.
        /// </summary>
        public ulong HandsControllerAddr { get; }
        /// <summary>
        /// ObservedPlayerController for non-clientplayer players.
        /// </summary>
        private ulong ObservedPlayerController { get; }
        /// <summary>
        /// ObservedHealthController for non-clientplayer players.
        /// </summary>
        private ulong ObservedHealthController { get; }

        // Backing fields for properties
        private string _name;
        private PlayerType _type;
        private string _alerts;
        private int _groupId;
        private Enums.EPlayerSide _playerSide;

        /// <summary>
        /// Player name.
        /// </summary>
        public override string Name
        {
            get => _name;
            set => _name = value;
        }
        /// <summary>
        /// Type of player unit.
        /// </summary>
        public override PlayerType Type
        {
            get => _type;
            set => _type = value;
        }
        /// <summary>
        /// Player Alerts.
        /// </summary>
        public override string Alerts
        {
            get => _alerts;
            protected set => _alerts = value;
        }
        /// <summary>
        /// Account UUID for Human Controlled Players (not available).
        /// </summary>
        public override string AccountID => "";
        /// <summary>
        /// Group that the player belongs to.
        /// </summary>
        public override int GroupID
        {
            get => _groupId;
            protected set => _groupId = value;
        }
        /// <summary>
        /// Player's Faction.
        /// </summary>
        public override Enums.EPlayerSide PlayerSide
        {
            get => _playerSide;
            protected set => _playerSide = value;
        }
        /// <summary>
        /// Player is Human-Controlled.
        /// </summary>
        public override bool IsHuman { get; }
        /// <summary>
        /// MovementContext / StateContext
        /// </summary>
        public override ulong MovementContext { get; }
        /// <summary>
        /// Corpse field address..
        /// </summary>
        public override ulong CorpseAddr { get; }
        /// <summary>
        /// Player Rotation Field Address (view angles).
        /// </summary>
        public override ulong RotationAddress { get; }
        /// <summary>
        /// Player's Current Health Status
        /// </summary>
        public Enums.ETagStatus HealthStatus { get; private set; } = Enums.ETagStatus.Healthy;
        /// <summary>
        /// Raid ID for this player session.
        /// </summary>
        public int RaidId { get; private set; }

        /// <summary>
        /// Player ID for this player session.
        /// </summary>
        public int PlayerId { get; private set; }

        internal ObservedPlayer(ulong playerBase) : base(playerBase)
        {
            var localPlayer = Memory.LocalPlayer;
            ArgumentNullException.ThrowIfNull(localPlayer, nameof(localPlayer));

            RaidId = Memory.ReadValue<int>(this + Offsets.ObservedPlayerView.RaidId);
            PlayerId = Memory.ReadValue<int>(this + Offsets.ObservedPlayerView.Id);

            ObservedPlayerController = Memory.ReadPtr(this + Offsets.ObservedPlayerView.ObservedPlayerController);
            ArgumentOutOfRangeException.ThrowIfNotEqual(this, Memory.ReadValue<ulong>(ObservedPlayerController + Offsets.ObservedPlayerController.PlayerView), nameof(ObservedPlayerController));
            InventoryControllerAddr = ObservedPlayerController + Offsets.ObservedPlayerController.InventoryController;
            HandsControllerAddr = ObservedPlayerController + Offsets.ObservedPlayerController.HandsController;
            ObservedHealthController = Memory.ReadPtr(ObservedPlayerController + Offsets.ObservedPlayerController.HealthController);
            ArgumentOutOfRangeException.ThrowIfNotEqual(this, Memory.ReadValue<ulong>(ObservedHealthController + Offsets.ObservedHealthController._player), nameof(ObservedHealthController));
            CorpseAddr = ObservedHealthController + Offsets.ObservedHealthController._playerCorpse;

            MovementContext = GetMovementContext();
            RotationAddress = ValidateRotationAddr(MovementContext + Offsets.ObservedPlayerStateContext.Rotation);
            /// Setup Transform
            var ti = Memory.ReadPtrChain(this, false, _transformInternalChain);
            SkeletonRoot = new UnityTransform(ti);
            var initialPos = SkeletonRoot.UpdatePosition();
            SetupBones();
            // Initialize cached position for fallback (in case skeleton updates fail later)
            _cachedPosition = initialPos;

            bool isAI = Memory.ReadValue<bool>(this + Offsets.ObservedPlayerView.IsAI);
            IsHuman = !isAI;
            // Get Group ID
            GroupID = -1;
            /// Determine Player Type
            PlayerSide = (Enums.EPlayerSide)Memory.ReadValue<int>(this + Offsets.ObservedPlayerView.Side);
            if (!Enum.IsDefined(PlayerSide))
                throw new ArgumentOutOfRangeException(nameof(PlayerSide));
            if (IsScav)
            {
                if (isAI)
                {
                    var voicePtr = Memory.ReadPtr(this + Offsets.ObservedPlayerView.Voice);
                    string voice = Memory.ReadUnityString(voicePtr);
                    var role = GetAIRoleInfo(voice);
                    Name = role.Name;
                    Type = role.Type;
                }
                else
                {
                    Name = $"PScav{GetPlayerId()}";
                    Type = IsTempTeammate(this) || (GroupID != -1 && GroupID == localPlayer.GroupID) ?
                        PlayerType.Teammate : PlayerType.PScav;
                }
            }
            else if (IsPmc)
            {
                Name = $"{PlayerSide.ToString().ToUpper()}{GetPlayerId()}";
                Type = IsTempTeammate(this) || (GroupID != -1 && GroupID == localPlayer.GroupID) ?
                    PlayerType.Teammate : PlayerType.PMC;
            }
            else
                throw new NotImplementedException(nameof(PlayerSide));

            Equipment = new PlayerEquipment(this);
        }

        /// <summary>
        /// Get the Player's ID.
        /// </summary>
        /// <returns>Player Id or 0 if failed.</returns>
        private int GetPlayerId()
        {
            try
            {
                return Memory.ReadValue<int>(this + Offsets.ObservedPlayerView.Id);
            }
            catch
            {
                return 0;
            }
        }


        /// <summary>
        /// Gets player's Group Number.
        /// </summary>
        private int GetGroupNumber()
        {
            try
            {
                var groupIdPtr = Memory.ReadPtr(this + Offsets.ObservedPlayerView.GroupID);
                string groupId = Memory.ReadUnityString(groupIdPtr);
                return _groups.GetOrAdd(
                    groupId,
                    _ => Interlocked.Increment(ref _lastGroupNumber));
            }
            catch { return -1; }
        }

        /// <summary>
        /// Get Movement Context Instance.
        /// </summary>
        private ulong GetMovementContext()
        {
            var movementController = Memory.ReadPtrChain(ObservedPlayerController, true, Offsets.ObservedPlayerController.MovementController, Offsets.ObservedMovementController.ObservedPlayerStateContext);
            return movementController;
        }

        private void SetupBones()
        {
            var bonesToRegister = new[]
            {
                Bones.HumanHead,
                Bones.HumanNeck,
                Bones.HumanSpine3,
                Bones.HumanSpine2,
                Bones.HumanSpine1,
                Bones.HumanPelvis,
                Bones.HumanLUpperarm,
                Bones.HumanLForearm1,
                Bones.HumanLForearm2,
                Bones.HumanLPalm,
                Bones.HumanRUpperarm,
                Bones.HumanRForearm1,
                Bones.HumanRForearm2,
                Bones.HumanRPalm,
                Bones.HumanLThigh1,
                Bones.HumanLThigh2,
                Bones.HumanLCalf,
                Bones.HumanLFoot,
                Bones.HumanRThigh1,
                Bones.HumanRThigh2,
                Bones.HumanRCalf,
                Bones.HumanRFoot
            };

            foreach (var bone in bonesToRegister)
            {
                try
                {
                    var chain = _transformInternalChain.ToArray();
                    chain[chain.Length - 2] = UnityList<byte>.ArrStartOffset + (uint)bone * 0x8;
                    
                    var ti = Memory.ReadPtrChain(this, false, chain);
                    var transform = new UnityTransform(ti);
                    PlayerBones.TryAdd(bone, transform);
                }
                catch { }
            }
            
            if (PlayerBones.Count > 0)
            {
                 _verticesCount = PlayerBones.Values.Max(x => x.Count);
                 _verticesCount = Math.Max(_verticesCount, SkeletonRoot.Count);
            }
            Skeleton = new PlayerSkeleton(SkeletonRoot, PlayerBones);
        }

        /// <summary>
        /// Refresh Player Information.
        /// </summary>
        public override void OnRegRefresh(VmmScatter scatter, ISet<ulong> registered, bool? isActiveParam = null)
        {
            if (isActiveParam is not bool isActive)
                isActive = registered.Contains(this);
            if (isActive)
            {
                UpdateHealthStatus();
            }
            base.OnRegRefresh(scatter, registered, isActive);
        }

        /// <summary>
        /// Updates the player's Type when temporary teammate status changes.
        /// </summary>
        public override void UpdateTypeForTeammate(bool isTeammate)
        {
            if (isTeammate && Type is PlayerType.PMC or PlayerType.PScav)
                Type = PlayerType.Teammate;
            else if (!isTeammate && Type is PlayerType.Teammate)
                Type = IsPmc ? PlayerType.PMC : PlayerType.PScav;
        }

        /// <summary>
        /// Get Player's Updated Health Condition
        /// Only works in Online Mode.
        /// </summary>
        private void UpdateHealthStatus()
        {
            try
            {
                var tag = (Enums.ETagStatus)Memory.ReadValue<int>(ObservedHealthController + Offsets.ObservedHealthController.HealthStatus);
                if ((tag & Enums.ETagStatus.Dying) == Enums.ETagStatus.Dying)
                    HealthStatus = Enums.ETagStatus.Dying;
                else if ((tag & Enums.ETagStatus.BadlyInjured) == Enums.ETagStatus.BadlyInjured)
                    HealthStatus = Enums.ETagStatus.BadlyInjured;
                else if ((tag & Enums.ETagStatus.Injured) == Enums.ETagStatus.Injured)
                    HealthStatus = Enums.ETagStatus.Injured;
                else
                    HealthStatus = Enums.ETagStatus.Healthy;
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"ERROR updating Health Status for '{Name}': {ex}");
            }
        }

        /// <summary>
        /// Check if this player is Santa Claus by checking equipment IDs.
        /// Santa has a specific backpack (61b9e1aaef9a1b5d6a79899a).
        /// </summary>
        public void CheckSanta()
        {
            if (!IsAI || Type != PlayerType.AIBoss)
                return;

            try
            {
                var inventorycontroller = Memory.ReadPtr(InventoryControllerAddr);
                var inventory = Memory.ReadPtr(inventorycontroller + Offsets.InventoryController.Inventory);
                var equipment = Memory.ReadPtr(inventory + Offsets.Inventory.Equipment);
                var slotsPtr = Memory.ReadPtr(equipment + Offsets.InventoryEquipment._cachedSlots);

                using var slotsArray = UnityArray<ulong>.Create(slotsPtr, true);
                if (slotsArray.Count < 1)
                    return;

                foreach (var slotPtr in slotsArray)
                {
                    var namePtr = Memory.ReadPtr(slotPtr + Offsets.Slot.ID);
                    if (namePtr == 0)
                        continue;

                    var slotName = Memory.ReadUnityString(namePtr);
                    if (slotName != "Backpack")
                        continue;

                    var containedItem = Memory.ReadPtr(slotPtr + Offsets.Slot.ContainedItem);
                    if (containedItem == 0)
                        continue;

                    var inventorytemplate = Memory.ReadPtr(containedItem + Offsets.LootItem.Template);
                    if (inventorytemplate == 0)
                        continue;

                    var mongoId = Memory.ReadValue<MongoID>(inventorytemplate + Offsets.ItemTemplate._id);
                    var itemId = mongoId.ReadString();

                    if (itemId == "61b9e1aaef9a1b5d6a79899a") // Santa's backpack!
                    {
                        if (Name != "Santa")
                        {
                            Name = "Santa";
                        }
                        return;
                    }
                }
            }
            catch
            {
                // silently skip
            }
        }

        /// <summary>
        /// Check if this player is Zryachiy by checking equipment IDs.
        /// Zryachiy has specific FaceCover (63626d904aa74b8fe30ab426) and Headwear (636270263f2495c26f00b007).
        /// </summary>
        public void CheckZryachiy()
        {
            if (!IsAI || Type != PlayerType.AIBoss)
                return;

            try
            {
                var inventorycontroller = Memory.ReadPtr(InventoryControllerAddr);
                var inventory = Memory.ReadPtr(inventorycontroller + Offsets.InventoryController.Inventory);
                var equipment = Memory.ReadPtr(inventory + Offsets.Inventory.Equipment);
                var slotsPtr = Memory.ReadPtr(equipment + Offsets.InventoryEquipment._cachedSlots);

                using var slotsArray = UnityArray<ulong>.Create(slotsPtr, true);
                if (slotsArray.Count < 1)
                    return;

                // Zryachiy equipment IDs to check
                var zryachiyItems = new HashSet<string>
                {
                    "63626d904aa74b8fe30ab426",
                    "636270263f2495c26f00b007"
                };

                bool hasZryachiyEquipment = false;

                foreach (var slotPtr in slotsArray)
                {
                    var namePtr = Memory.ReadPtr(slotPtr + Offsets.Slot.ID);
                    if (namePtr == 0)
                        continue;

                    var slotName = Memory.ReadUnityString(namePtr);

                    // Only check FaceCover and Headwear slots
                    if (slotName != "FaceCover" && slotName != "Headwear")
                        continue;

                    var containedItem = Memory.ReadPtr(slotPtr + Offsets.Slot.ContainedItem);
                    if (containedItem == 0)
                        continue;

                    var inventorytemplate = Memory.ReadPtr(containedItem + Offsets.LootItem.Template);
                    if (inventorytemplate == 0)
                        continue;

                    var mongoId = Memory.ReadValue<MongoID>(inventorytemplate + Offsets.ItemTemplate._id);
                    var itemId = mongoId.ReadString();

                    if (zryachiyItems.Contains(itemId))
                    {
                        hasZryachiyEquipment = true;
                        break;
                    }
                }

                if (hasZryachiyEquipment && Name != "Zryachiy")
                {
                    Name = "Zryachiy";
                }
            }
            catch
            {
                // silently skip
            }
        }

        private static readonly uint[] _transformInternalChain =
        [
            Offsets.ObservedPlayerView.PlayerBody,
            Offsets.PlayerBody.SkeletonRootJoint,
            Offsets.DizSkinningSkeleton._values,
            UnityList<byte>.ArrOffset,
            UnityList<byte>.ArrStartOffset + (uint)Bones.HumanBase * 0x8,
            0x10
        ];
    }
}
