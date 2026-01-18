using System.Linq;
using static LoneEftDmaRadar.Tarkov.Unity.UnitySDK;

namespace SDK
{
    public readonly partial struct Offsets
    {
        public readonly partial struct GameWorld
		{
			public const uint BtrController = 0x20; // EFT.Vehicle.BtrController
			public const uint LocationId = 0xC8; // string
			public const uint LootList = 0x190; // System.Collections.Generic.List<IKillable>
			public const uint RegisteredPlayers = 0x1B0; // System.Collections.Generic.List<IPlayer>
			public const uint MainPlayer = 0x208; // EFT.Player
			public const uint SynchronizableObjectLogicProcessor = 0x240; // EFT.SynchronizableObjects.SynchronizableObjectLogicProcessor
			public const uint Grenades = 0x280; // DictionaryListHydra<int, Throwable>
            public const uint ExfiltrationController = 0x50; // EFT.Interactive.ExfiltrationController
        }

        public readonly partial struct ExfiltrationController
        {
            public const uint SecretExfilitrationController = 0x18; // EFT.Interactive.SecretExfiltrations.SecretExfilitranionController
            public const uint ExfiltrationPoints = 0x20; // EFT.Interactive.ExfiltrationPoint[]
            public const uint ScavExfiltrationPoints = 0x28; // EFT.Interactive.ScavExfiltrationPoint[]
            public const uint SecretExfiltrationPoints = 0x30; // EFT.Interactive.SecretExfiltrations.SecretExfiltrationPoint[]
        }

        public readonly partial struct ExfiltrationPoint
        {
            public const uint _status = 0x58; // EFT.Interactive.EExfiltrationStatus
            public const uint Settings = 0x98; // EFT.Interactive.ExitTriggerSettings
            public const uint EligibleEntryPoints = 0xC0; // string[]
            public const uint EligibleIds = 0xF8; // System.Collections.Generic.List<string>
        }

        public readonly partial struct ExitTriggerSettings
        {
            public const uint Id = 0x10; // string
            public const uint Name = 0x18; // string
            public const uint EntryPoints = 0x40; // string
        }

        public readonly partial struct EFTHardSettings
        {
			public const uint DecelerationSpeed = 0xc0; // Single
			public const uint LOOT_RAYCAST_DISTANCE = 0x188; // Single
			public const uint DOOR_RAYCAST_DISTANCE = 0x18c; // Single
			public const uint STOP_AIMING_AT = 0x214; // Single
			public const uint MOUSE_LOOK_HORIZONTAL_LIMIT = 0x340; // UnityEngine.Vector2
			public const uint MOUSE_LOOK_VERTICAL_LIMIT = 0x348; // UnityEngine.Vector2
        }

        public readonly partial struct SynchronizableObject
        {
            public const uint Type = 0x68; // System.Int32
        }

        public readonly partial struct SynchronizableObjectLogicProcessor
        {
            public const uint SynchronizableObjects = 0x18; // System.Collections.Generic.List<SynchronizableObject>
        }

        public readonly partial struct TripwireSynchronizableObject
        {
            public const uint _tripwireState = 0xE4; // System.Int32
            public const uint ToPosition = 0x158; // UnityEngine.Vector3
        }

        public readonly partial struct BtrController
        {
            public const uint BtrView = 0x50; // EFT.Vehicle.BTRView
        }

        public readonly partial struct BTRView
        {
            public const uint turret = 0x60; // EFT.Vehicle.BTRTurretView
            public const uint _previousPosition = 0xB4;
        }

        public readonly partial struct BTRTurretView
        {
            public const uint AttachedBot = 0x60; // System.ValueTuple<ObservedPlayerView, Boolean>
        }

        public readonly partial struct Throwable
        {
            public const uint _isDestroyed  = 0x4D; // Boolean
        }

        public readonly partial struct Player
        {
            public const uint MovementContext = 0x60; // EFT.MovementContext
            public const uint _playerBody = 0x190; // EFT.PlayerBody
            public const uint Physical = 0x918; // -.\uE399 <Physical> Physical
            public const uint Corpse = 0x680; // EFT.Interactive.Corpse
            public const uint Location = 0x870; // String
            public const uint RaidId = 0x8D8; // Int32
            public const uint PlayerId = 0x8F8; // Int32
            public const uint Profile = 0x900; // EFT.Profile
            public const uint ProceduralWeaponAnimation = 0x338; // EFT.Animations.ProceduralWeaponAnimation
            public const uint _inventoryController = 0x978; // EFT.PlayerInventoryController update
            public const uint _handsController = 0x980; // EFT.PlayerHands update
            public const uint _playerLookRaycastTransform = 0xA08; // UnityEngine.Transform
        }

        public readonly partial struct ObservedPlayerView
        {
            public const uint RaidId = 0x20; // Int32
            public const uint ObservedPlayerController = 0x28; // EFT.NextObservedPlayer.ObservedPlayerController
			public const uint Voice = 0x40; // string
            public const uint WorldTime = 0x68; // single
            public const uint Id = 0x7C; // int32_t
            public const uint GroupID = 0x80; // string
			public const uint Side = 0x94; // EFT.EPlayerSide
			public const uint IsAI = 0xA0; // bool
            public const uint AccountId = 0xC0; // string
			public const uint PlayerBody = 0xD8; // EFT.PlayerBody
        }

        public readonly partial struct ObservedPlayerController
        {
            public const uint InventoryController = 0x10; // EFT.NextObservedPlayer.ObservedPlayerInventoryController
            public const uint PlayerView = 0x18; // EFT.NextObservedPlayer.ObservedPlayerView
            public const uint MovementController = 0xD8; // EFT.NextObservedPlayer.ObservedPlayerMovementController
            public const uint HealthController = 0xE8; // ObservedPlayerHealthController
            public const uint HandsController = 0x120; // EFT.PlayerHandsController update
        }

        public readonly partial struct ObservedPlayerHandsController
        {
            public const uint _item = 0x58; // EFT.InventoryLogic.Item
        }

        public readonly partial struct ObservedPlayerStateContext
        {
            public const uint Rotation = 0x20; // UnityEngine.Vector2
        }

          public readonly partial struct ObservedHealthController
        {
            public const uint HealthStatus = 0x10; // ETagStatus
            public const uint _player = 0x18; // EFT.NextObservedPlayer.ObservedPlayerView
            public const uint _playerCorpse = 0x20; // EFT.Interactive.ObservedCorpse
        }

        public readonly partial struct Profile // EFT, class: Profile
        {
            public const uint Id = 0x10; // String
            public const uint AccountId = 0x18; // String
            public const uint Info = 0x48; // -.\uE9AD
            public const uint QuestsData = 0x98; // object
        }

        public readonly partial struct QuestsData
        {
            public const uint Id = 0x10; // string
            public const uint Status = 0x1C; // object
            public const uint CompletedConditions = 0x28; // object
        }

        public readonly partial struct PlayerInfo // EFT, class: ProfileInfo
        {
            public const uint EntryPoint = 0x28; // String
            public const uint GroupId = 0x50; // String
            public const uint Side = 0x48; // [HUMAN] Int32
            public const uint RegistrationDate = 0x4C; // Int32
        }

        public readonly partial struct MovementContext // EFT, class: MovementContext
        {
            public const uint Player = 0x48; // EFT.Player
            public const uint _rotation = 0xC8; // UnityEngine.Vector2
            public const uint PlantState = 0x78; // EFT.BaseMovementState <PlantState> PlantState
            public const uint CurrentState = 0x1F0; // EFT.BaseMovementState <CurrentState>k__BackingField
            public const uint _states = 0x480; // System.Collections.Generic.Dictionary<Byte, BaseMovementState> <_states> _states
            public const uint _movementStates = 0x4b0; // -.IPlayerStateContainerBehaviour[] <_movementStates> _movementStates
            public const uint _tilt = 0xb4; // Single <_tilt> _tilt
            public const uint _physicalCondition = 0x198; // System.Int32 <_physicalCondition> _physicalCondition
            public const uint _speedLimitIsDirty = 0x1b9; // Boolean <_speedLimitIsDirty> _speedLimitIsDirty
            public const uint StateSpeedLimit = 0x1bc; // Single <<StateSpeedLimit>k__BackingField> <StateSpeedLimit>k__BackingField
            public const uint StateSprintSpeedLimit = 0x1c0; // Single <<StateSprintSpeedLimit>k__BackingField> <StateSprintSpeedLimit>k__BackingField
            public const uint _lookDirection = 0x3b8; // UnityEngine.Vector3  <_lookDirection> _lookDirection
        }

        public readonly partial struct MovementState //Class: MovementState
        {
            public const uint StickToGround = 0x54; // Boolean <StickToGround> StickToGround
            public const uint PlantTime = 0x58; // Single <PlantTime> PlantTime
            public const uint Name = 0x11; // System.Byte <Name> Name
            public const uint AnimatorStateHash = 0x20; // Int32 <AnimatorStateName> AnimatorStateName
        }

        public readonly partial struct PlayerStateContainer //Class: PlayerStateContainer
        {
            public const uint Name = 0x19; // System.Byte
            public const uint StateFullNameHash = 0x40; // Int32 <StateFullNameHash> StateFullNameHash
        }

        public readonly partial struct InteractiveLootItem // EFT.Interactive, class: LootItem
        {
            public const uint Item = 0xF0; // EFT.InventoryLogic.Item
        }

        public readonly partial struct DizSkinningSkeleton // Diz.Skinning, class: Skeleton
        {
            public const uint _values = 0x30; // System.Collections.Generic.List<Transform>
        }

        public readonly partial struct LootableContainer // EFT.Interactive, class: LootableContainer
        {
            public const uint ItemOwner = 0x168; // -.\uEFB4
            public const uint InteractingPlayer = 0x150; // System.Object <InteractingPlayer>k__BackingField
        }

        public readonly partial struct LootableContainerItemOwner // EFT.InventoryLogic, class: ItemController
        {
            public const uint RootItem = 0xD0; // EFT.InventoryLogic.Item
        }

        public readonly partial struct LootItem
        {
            public const uint Template = 0x60; // EFT.InventoryLogic.ItemTemplate
            public const uint Version = 0x7C; // Int32
        }

        public readonly partial struct ItemTemplate // EFT.InventoryLogic, class: ItemTemplate
        {
            public const uint ShortName = 0x18; // String
            public const uint _id = 0xE0; // EFT.MongoID
            public const uint QuestItem = 0x34; // Boolean
        }

        public readonly partial struct PlayerBody // EFT, class: PlayerBody
        {
            public const uint SkeletonRootJoint = 0x30;
        }

        public readonly partial struct FirearmController
        {
            public const uint WeaponAnimation = 0x198; // EFT.Animations.ProceduralWeaponAnimation
            public const uint Fireport = 0x150; // EFT.BifacialTransform <Fireport> Fireport
            public const uint TotalCenterOfImpact = 0x2a0; // Single
            public static readonly uint[] To_FirePortTransformInternal = new uint[] { Fireport, 0x10, 0x10 };
            public static readonly uint[] To_FirePortVertices = To_FirePortTransformInternal.Concat(new uint[] { UnityOffsets.TransformInternal_TransformAccessOffset, UnityOffsets.Hierarchy_VerticesOffset }).ToArray();
        }

        public readonly partial struct ClientFirearmController // EFT, class: ClientFirearmController : FirearmController
        {
            public const uint FirearmPacket = 0x1F0; // System.Object
            public const uint _firedShots = 0x420; // GenericInst
            public const uint _shotsWaitingApproval = 0x428; // GenericInst
            public const uint _shotsFragmentsBuffer = 0x430; // UnknownType
            public const uint LastShotId = 0x438; // System.Byte - increments per shot
            public const uint LastShotType = 0x43C; // System.Object
        }


        public readonly partial struct ProceduralWeaponAnimation // EFT.Animations, class: ProceduralWeaponAnimation
        {
            public const uint HandsContainer = 0x20; // EFT.Animations.PlayerSpring <HandsContainer> HandsContainer
            public const uint Breath = 0x38; // EFT.Animations.BreathEffector <Breath> Breath
            public const uint MotionReact = 0x48; // -.MotionEffector <MotionReact> MotionReact
            public const uint Shootingg = 0x58; // -.ShotEffector <Shootingg> Shootingg
            public const uint _optics = 0x180; // System.Collections.Generic.List<ProceduralWeaponAnimation.SightNBone>
            public const uint Mask = 0x30; // System.Int32 <Mask> Mask
            public const uint _isAiming = 0x145; // Boolean
            public const uint _pointOfView = 0x160;
            public const uint _aimingSpeed = 0x164; // Single <_aimingSpeed> _aimingSpeed
            public const uint _fovCompensatoryDistance = 0x194; // Single <_fovCompensatoryDistance> _fovCompensatoryDistance
            public const uint _compensatoryScale = 0x1c4; // Single <_compensatoryScale> _compensatoryScale
            public const uint _shotDirection = 0x1c8; // UnityEngine.Vector3 <_shotDirection> _shotDirection
            public const uint CameraSmoothOut = 0x20c; // Single <CameraSmoothOut> CameraSmoothOut
            public const uint PositionZeroSum = 0x31c; // UnityEngine.Vector3 <PositionZeroSum> PositionZeroSum
            public const uint _swayStrength = 0x390;
            public const uint ShotNeedsFovAdjustments = 0x433; // Boolean <<ShotNeedsFovAdjustments>k__BackingField> <ShotNeedsFovAdjustments>k__BackingField
        }

        public readonly partial struct SightNBone
        {
            public const uint Mod = 0x10; // EFT.InventoryLogic.SightComponent
        }

        public readonly partial struct SightComponent
        {
            public const uint _template = 0x20; // EFT.InventoryLogic.ISightComponentTemplate
            public const uint ScopesSelectedModes = 0x30; // System.Int32[]
            public const uint SelectedScope = 0x38; // System.Int32
            public const uint ScopeZoomValue = 0x3C; // System.Single
        }

        public readonly partial struct SightInterface
        {
            public const uint Zooms = 0x1B8; // System.Single[][]
        }

        public readonly partial struct Physical //Class: .PhysicalBase
        {
            public const uint Stamina = 0x68; // -.\uE398 <Stamina> Stamina
            public const uint HandsStamina = 0x70; // -.\uE398 <HandsStamina> HandsStamina
            public const uint Oxygen = 0x78; // -.\uE398 <Oxygen> Oxygen
            public const uint Overweight = 0x1c; // Single <Overweight> Overweight
            public const uint WalkOverweight = 0x20; // Single <WalkOverweight> WalkOverweight
            public const uint WalkSpeedLimit = 0x24; // Single <WalkSpeedLimit> WalkSpeedLimit
            public const uint Inertia = 0x28; // Single <Inertia> Inertia
            public const uint WalkOverweightLimits = 0xa4; // UnityEngine.Vector2 <WalkOverweightLimits> WalkOverweightLimits
            public const uint BaseOverweightLimits = 0xac; // UnityEngine.Vector2 <BaseOverweightLimits> BaseOverweightLimits
            public const uint SprintOverweightLimits = 0xc0; // UnityEngine.Vector2 <SprintOverweightLimits> SprintOverweightLimits
            public const uint SprintWeightFactor = 0x104; // Single
            public const uint SprintAcceleration = 0x114; // Single <SprintAcceleration> SprintAcceleration
            public const uint PreSprintAcceleration = 0x118; // Single <PreSprintAcceleration> PreSprintAcceleration
            public const uint IsOverweightA = 0x11C; // Boolean
            public const uint IsOverweightB = 0x11D; // Boolean
        }

        public readonly partial struct PhysicalValue //Class: .Stamina
        {
            public const uint Current = 0x10; // Single
        }

        public readonly partial struct BreathEffector //Class: EFT.Animations.BreathEffector
        {
            public const uint Physical = 0x10; // System.Object
            public const uint IsAiming = 0x18; // Boolean
            public const uint _handsRotationSpring = 0x20; // System.Object
            public const uint _shotEffector = 0x28; // System.Object
            public const uint Intensity = 0x30; // Single <Intensity> Intensity
            public const uint _shakeIntensity = 0x34; // Single
            public const uint XRandom = 0x38; // System.Object - Random generator for X sway
            public const uint YRandom = 0x40; // System.Object - Random generator for Y sway
            public const uint _breathIntensity = 0x48; // Single
            public const uint _breathFrequency = 0x4C; // Single
            public const uint TremorXRandom = 0x50; // System.Object
            public const uint TremorYRandom = 0x58; // System.Object
            public const uint TremorZRandom = 0x60; // System.Object
            public const uint HipXRandom = 0x68; // System.Object
            public const uint HipZRandom = 0x70; // System.Object
            public const uint HipPenalty = 0x78; // Single
            public const uint Curves = 0x80; // UnknownType
            public const uint _processors = 0x88; // UnknownType
            public const uint BreathParams = 0x90; // System.Object
            public const uint TremorOn = 0x98; // Boolean
            public const uint Fracture = 0x99; // Boolean
            public const uint _cameraSensetivity = 0x9C; // Single
            public const uint StiffUntill = 0xA0; // Single
            public const uint EnergyLowerLimit = 0xA4; // Single
            public const uint EnergyFractureLimit = 0xA8; // Single
            public const uint AmplitudeGainPerShot = 0xAC; // Single
            public const uint Delay = 0xB0; // Single
            public const uint Overweight = 0xF8; // Single
            public const uint OxygenLevel = 0x100; // GenericInst
            public const uint StaminaLevel = 0x108; // GenericInst
        }

        public readonly partial struct ShotEffector //Class: .ShotEffector
        {
            public const uint NewShotRecoil = 0x20; // EFT.Animations.NewRecoil.NewRecoilShotEffect <NewShotRecoil> NewShotRecoil
        }

        public readonly partial struct NewShotRecoil //Class: EFT.Animations.NewRecoil.NewRecoilShotEffect
        {
            public const uint IntensitySeparateFactors = 0x94; // UnityEngine.Vector3 <IntensitySeparateFactors> IntensitySeparateFactors
        }

        public readonly partial struct ItemHandsController //Class: .ItemHandsController
        {
            public const uint Item = 0x70; // EFT.InventoryLogic.Item
        }

        public readonly partial struct LootItemWeapon //Class: EFT.InventoryLogic.Weapon
        {
            public const uint FireMode = 0xa0; // EFT.InventoryLogic.FireModeComponent <FireMode> FireMode
            public const uint Chambers = 0xb0; // EFT.InventoryLogic.Slot[] <<Chambers>k__BackingField> <Chambers>k__BackingField
            public const uint _magSlotCache = 0xc8; // EFT.InventoryLogic.Slot <_magSlotCache> _magSlotCache
        }

        public readonly partial struct FireModeComponent //Class: EFT.InventoryLogic.FireModeComponent
        {
            public const uint Item = 0x10; 
            public const uint _template = 0x18;
            public const uint OnChanged = 0x20;
            public const uint FireMode = 0x28;
        }

        public readonly partial struct LootItemMagazine //Class: EFT.InventoryLogic.MagazineTemplate
        {
            public const uint Cartridges = 0x1a8; // EFT.InventoryLogic.StackSlot <Cartridges> Cartridges (on Template)
            public const uint LoadUnloadModifier = 0x1b0; // Single <LoadUnloadModifier> LoadUnloadModifier
        }

        // Magazine Item (not template) offsets - used when reading from magSlot.ContainedItem
        public readonly partial struct MagazineItem //Class: EFT.InventoryLogic.MagazineClass (Item instance)
        {
            public const uint Cartridges = 0xA8; // EFT.InventoryLogic.StackSlot - Cartridges on the Magazine Item itself
        }

        public readonly partial struct StackSlot //Class: EFT.InventoryLogic.StackSlot
        {
            public const uint MaxCount = 0x10; // Int32 - Max cartridge count
            public const uint _items = 0x18; // System.Collections.Generic.List<Item> - List of ammo stacks
        }

        public readonly partial struct Slot //Class: EFT.InventoryLogic.Slot
        {
            public const uint ContainedItem = 0x48; // EFT.InventoryLogic.Item
            public const uint ID = 0x58; // String
            public const uint Required = 0x18; // Boolean
        }

        public readonly partial struct LootItemMod //Class: EFT.InventoryLogic.Mod
        {
            public const uint Grids = 0x78; // -.\uEE74[]
            public const uint Slots = 0x80; // EFT.InventoryLogic.Slot[]
        }

        public readonly partial struct ModTemplate //Class: EFT.InventoryLogic.ModTemplate
        {
            public const uint Velocity = 0x188; // Single <Velocity> Velocity
        }

        public readonly partial struct AmmoTemplate //Class: EFT.InventoryLogic.AmmoTemplate
        {
            public const uint InitialSpeed = 0x1a4; // Single <InitialSpeed> InitialSpeed
            public const uint BallisticCoeficient = 0x1b8; // Single <BallisticCoeficient> BallisticCoeficient
            public const uint BulletMassGram = 0x25c; // Single <BulletMassGram> BulletMassGram
            public const uint BulletDiameterMilimeters = 0x260; // Single <BulletDiameterMilimeters> BulletDiameterMilimeters
        }

        public readonly partial struct WeaponTemplate //Class: EFT.InventoryLogic.WeaponTemplate
        {
            public const uint Velocity = 0x254; // Single <Velocity> Velocity
        }

        public readonly partial struct ObservedMovementController //Class: EFT.NextObservedPlayer.ObservedPlayerMovementModel
        {
            public const uint ObservedPlayerStateContext = 0x98;
            public const uint Rotation = 0x1c; // UnityEngine.Vector2 <HeadRotation> HeadRotation
            public const uint Velocity = 0x30; // UnityEngine.Vector3 <Velocity> Velocity
        }

        public readonly partial struct InventoryController
        {
            public const uint Inventory = 0x100; // EFT.InventoryLogic.Inventory
        }

        public readonly partial struct Inventory
        {
            public const uint Equipment = 0x18; // EFT.InventoryLogic.InventoryEquipment
        }

        public readonly partial struct InventoryEquipment
        {
            public const uint _cachedSlots = 0x90; // EFT.InventoryLogic.Slot[]
        }

        public readonly partial struct MagazineClass //Class: EFT.InventoryLogic.Item
        {
            public const uint StackObjectsCount = 0x24; // int32_t
        }

        public readonly partial struct TarkovApplication // EFT.TarkovApplication
        {
            public const uint MenuOperation = 0x128; // System.Object
        }

        public readonly partial struct MenuOperation // EFT.MainMenuShowOperation
        {
            public const uint AfkMonitor = 0x38; // System.Object
        }

        public readonly partial struct AfkMonitor
        {
            public const uint Delay = 0x10; // _afkTimeout
        }
    }

    public readonly partial struct Enums
    {
        public enum EPlayerSide
        {
            Usec = 1,
            Bear = 2,
            Savage = 4,
        }

        public enum EPlayerState : byte
        {
            None = 0,
            Idle = 1,
            ProneIdle = 2,
            ProneMove = 3,
            Run = 4,
            Sprint = 5,
            Jump = 6,
            FallDown = 7,
            Transition = 8,
            BreachDoor = 9,
            Loot = 10,
            Pickup = 11,
            Open = 12,
            Close = 13,
            Unlock = 14,
            Sidestep = 15,
            DoorInteraction = 16,
            Approach = 17,
            Prone2Stand = 18,
            Transit2Prone = 19,
            Plant = 20,
            Stationary = 21,
            Roll = 22,
            JumpLanding = 23,
            ClimbOver = 24,
            ClimbUp = 25,
            VaultingFallDown = 26,
            VaultingLanding = 27,
            BlindFire = 28,
            IdleWeaponMounting = 29,
            IdleZombieState = 30,
            MoveZombieState = 31,
            TurnZombieState = 32,
            StartMoveZombieState = 33,
            EndMoveZombieState = 34,
            DoorInteractionZombieState = 35,
        }

        [Flags]
        public enum ETagStatus
        {
            Unaware = 1,
            Aware = 2,
            Combat = 4,
            Solo = 8,
            Coop = 16,
            Bear = 32,
            Usec = 64,
            Scav = 128,
            TargetSolo = 256,
            TargetMultiple = 512,
            Healthy = 1024,
            Injured = 2048,
            BadlyInjured = 4096,
            Dying = 8192,
            Birdeye = 16384,
            Knight = 32768,
            BigPipe = 65536,
            BlackDivision = 131072,
            VSRF = 262144
        }

        [Flags]
        public enum EMemberCategory
        {
            Default = 0,
            Developer = 1,
            UniqueId = 2,
            Trader = 4,
            Group = 8,
            System = 16,
            ChatModerator = 32,
            ChatModeratorWithPermanentBan = 64,
            UnitTest = 128,
            Sherpa = 256,
            Emissary = 512,
            Unheard = 1024,
        }

        public enum EExfiltrationStatus
        {
            NotPresent = 1,
            UncompleteRequirements = 2,
            Countdown = 3,
            RegularMode = 4,
            Pending = 5,
            AwaitsManualActivation = 6,
            Hidden = 7,
        }

        public enum SynchronizableObjectType
        {
            AirDrop = 0,
            AirPlane = 1,
            Tripwire = 2,
        }

        public enum ETripwireState
        {
            None = 0,
            Wait = 1,
            Active = 2,
            Exploding = 3,
            Exploded = 4,
            Inert = 5,
        }

        public enum EQuestStatus
        {
            Locked = 0,
            AvailableForStart = 1,
            Started = 2,
            AvailableForFinish = 3,
            Success = 4,
            Fail = 5,
            FailRestartable = 6,
            MarkedAsFailed = 7,
            Expired = 8,
            AvailableAfter = 9,
        }
    }
}
