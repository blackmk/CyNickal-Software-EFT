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

using Collections.Pooled;
using LoneEftDmaRadar.Misc;
using LoneEftDmaRadar.Tarkov.GameWorld.Loot;
using LoneEftDmaRadar.Tarkov.GameWorld.Player.Helpers;
using LoneEftDmaRadar.Tarkov.Unity;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using LoneEftDmaRadar.UI.Misc;
using LoneEftDmaRadar.UI.Radar.Maps;
using LoneEftDmaRadar.UI.Radar.ViewModels;
using LoneEftDmaRadar.UI.Skia;
using LoneEftDmaRadar.Web.TarkovDev.Data;
using VmmSharpEx.Scatter;
using static LoneEftDmaRadar.Tarkov.Unity.Structures.UnityTransform;

namespace LoneEftDmaRadar.Tarkov.GameWorld.Player
{
    /// <summary>
    /// Base class for Tarkov Players.
    /// Tarkov implements several distinct classes that implement a similar player interface.
    /// </summary>
    public abstract class AbstractPlayer : IWorldEntity, IMapEntity, IMouseoverEntity
    {
        #region Static Interfaces

        public static implicit operator ulong(AbstractPlayer x) => x.Base;
        protected static readonly ConcurrentDictionary<string, int> _groups = new(StringComparer.OrdinalIgnoreCase);
        protected static int _lastGroupNumber;

        /// <summary>
        /// Track the current RaidId to detect raid changes.
        /// </summary>
        private static int _currentRaidId;

        /// <summary>
        /// Track the current PlayerId to detect raid changes.
        /// </summary>
        private static int _currentPlayerId;

        /// <summary>
        /// Set of temporary marked teammates (by player Base address).
        /// Cleared on each raid end.
        /// </summary>
        protected static readonly HashSet<ulong> _tempTeammates = new();

        static AbstractPlayer()
        {
            Memory.RaidStopped += Memory_RaidStopped;
        }

        private static void Memory_RaidStopped(object sender, EventArgs e)
        {
            _groups.Clear();
            _lastGroupNumber = default;
            _currentRaidId = 0;
            _currentPlayerId = 0;
            lock (_tempTeammates)
            {
                _tempTeammates.Clear();
            }
        }

        /// <summary>
        /// Mark a player as a temporary teammate.
        /// </summary>
        public static void AddTempTeammate(AbstractPlayer player)
        {
            if (player == null) return;
            lock (_tempTeammates)
            {
                _tempTeammates.Add(player.Base);
            }
            player.UpdateTypeForTeammate(true);
        }

        /// <summary>
        /// Remove a player from temporary teammates.
        /// </summary>
        public static void RemoveTempTeammate(AbstractPlayer player)
        {
            if (player == null) return;
            lock (_tempTeammates)
            {
                _tempTeammates.Remove(player.Base);
            }
            player.UpdateTypeForTeammate(false);
        }

        /// <summary>
        /// Check if a player is marked as a temporary teammate.
        /// </summary>
        public static bool IsTempTeammate(AbstractPlayer player)
        {
            if (player == null) return false;
            lock (_tempTeammates)
            {
                return _tempTeammates.Contains(player.Base);
            }
        }

        /// <summary>
        /// Updates the player's Type when temporary teammate status changes.
        /// Override in derived classes to handle type updates.
        /// </summary>
        public virtual void UpdateTypeForTeammate(bool isTeammate)
        {
            // do nothing
        }

        /// <summary>
        /// Sets the player's GroupID after construction.
        /// </summary>
        protected void SetGroupId(int groupId)
        {
            GroupID = groupId;
        }

        /// <summary>
        /// Distance threshold for team detection (in meters).
        /// </summary>
        private const float TeammateDetectionDistance = 15.0f;

        /// <summary>
        /// Distance threshold for PRE-RAID team detection (in meters).
        /// Uses a more conservative threshold since players are closer together at spawn.
        /// </summary>
        private const float PreRaidTeammateDetectionDistance = 10.0f;

        /// <summary>
        /// Fixed GroupID assigned to local player's team.
        /// </summary>
        private const int LocalPlayerTeamGroupId = 999;

        /// <summary>
        /// Detects teams based on player proximity at raid start.
        /// Players within 20m of each other are considered teammates.
        /// Players within 20m of LocalPlayer are automatically marked as friendly.
        /// Uses cached team data if available for the same RaidId and PlayerId.
        /// Clears cache only when RaidId/PlayerId changes (new raid).
        /// </summary>
        public static void DetectTeams(LocalPlayer localPlayer, IEnumerable<AbstractPlayer> allPlayers)
        {
            if (localPlayer == null || allPlayers == null)
                return;

            if (_currentRaidId != 0 && (_currentRaidId != localPlayer.RaidId || _currentPlayerId != localPlayer.PlayerId))
            {
                DebugLogger.LogDebug($"[TeamDetection] RaidId/PlayerId changed from {_currentRaidId}/{_currentPlayerId} to {localPlayer.RaidId}/{localPlayer.PlayerId}, clearing old cache");
                RaidInfoCache.Clear();
            }
            _currentRaidId = localPlayer.RaidId;
            _currentPlayerId = localPlayer.PlayerId;

            _lastGroupNumber = 0;

            var candidates = allPlayers
                .Where(p => p is not LocalPlayer && p.IsHuman && p.IsAlive && p.IsActive)
                .ToList();

            if (candidates.Count == 0)
            {
                return;
            }

            var cachedTeams = RaidInfoCache.LoadTeams(localPlayer.RaidId, localPlayer.PlayerId);
            if (cachedTeams != null && cachedTeams.Count > 0)
            {
                ApplyCache(cachedTeams, candidates);
                int teammateCount = cachedTeams.Values.Count(x => x == LocalPlayerTeamGroupId);
                int enemyTeamCount = cachedTeams.Values.Where(x => x > 0 && x != LocalPlayerTeamGroupId).Distinct().Count();
                DebugLogger.LogDebug($"[TeamDetection] {teammateCount} teammate(s), {enemyTeamCount} enemy team(s)");
                return;
            }

            var teams = ClusterPlayers(candidates, TeammateDetectionDistance);
            var assignedTeams = AssignGroups(teams, localPlayer);

            RaidInfoCache.SaveTeams(localPlayer.RaidId, localPlayer.PlayerId, assignedTeams);

            if (assignedTeams.Count > 0)
            {
                int teammateCount = assignedTeams.Values.Count(x => x == LocalPlayerTeamGroupId);
                int enemyTeamCount = assignedTeams.Values.Where(x => x > 0 && x != LocalPlayerTeamGroupId).Distinct().Count();
                DebugLogger.LogDebug($"[TeamDetection] {teammateCount} teammate(s), {enemyTeamCount} enemy team(s)");
            }
        }

        /// <summary>
        /// Detects teams BEFORE raid starts (during pre-raid phase when hands are empty).
        /// Uses a more conservative distance threshold (10m) for more accurate detection at spawn points.
        /// Applies cached team data if available for the same RaidId and PlayerId.
        /// </summary>
        /// <param name="localPlayer">The local player.</param>
        /// <param name="allPlayers">All players in the game world.</param>
        public static void DetectTeamsPreRaid(LocalPlayer localPlayer, IEnumerable<AbstractPlayer> allPlayers)
        {
            if (localPlayer == null || allPlayers == null)
                return;

            // Skip if Scav (Scavs don't have teams)
            if (localPlayer.IsScav)
                return;

            // Initialize RaidId/PlayerId tracking if not already done
            if (_currentRaidId == 0 || _currentRaidId != localPlayer.RaidId || _currentPlayerId != localPlayer.PlayerId)
            {
                DebugLogger.LogDebug($"[TeamDetection-PreRaid] RaidId/PlayerId set to {localPlayer.RaidId}/{localPlayer.PlayerId}");
                _currentRaidId = localPlayer.RaidId;
                _currentPlayerId = localPlayer.PlayerId;
                _lastGroupNumber = 0;
            }

            var candidates = allPlayers
                .Where(p => p is not LocalPlayer && p.IsHuman && p.IsAlive && p.IsActive)
                .ToList();

            if (candidates.Count == 0)
                return; // No candidates, silently skip

            // Load or create cache
            var cachedTeams = RaidInfoCache.LoadTeams(localPlayer.RaidId, localPlayer.PlayerId) ?? new Dictionary<int, int>();

            // Find players not in cache (new players that appeared)
            var newPlayers = candidates
                .Where(p => p is ObservedPlayer obs && !cachedTeams.ContainsKey(obs.PlayerId))
                .ToList();

            // Apply cached data to existing players
            ApplyCache(cachedTeams, candidates);

            // If there are new players, detect their teams and merge with cache
            if (newPlayers.Count > 0)
            {
                var teams = ClusterPlayers(newPlayers, PreRaidTeammateDetectionDistance);
                var assignedTeams = AssignGroups(teams, localPlayer);

                // Merge new detections with existing cache
                foreach (var kvp in assignedTeams)
                {
                    cachedTeams[kvp.Key] = kvp.Value;
                }

                // Save updated cache
                RaidInfoCache.SaveTeams(localPlayer.RaidId, localPlayer.PlayerId, cachedTeams);

                // Apply the new team assignments
                ApplyCache(assignedTeams, newPlayers);
            }
        }

        /// <summary>
        /// Apply cached team data directly to players without re-running detection.
        /// </summary>
        private static void ApplyCache(Dictionary<int, int> cachedTeams, List<AbstractPlayer> candidates)
        {
            foreach (var player in candidates)
            {
                if (player is ObservedPlayer obs)
                {
                    if (cachedTeams.TryGetValue(obs.PlayerId, out int groupId))
                    {
                        bool isTeammate = (groupId == LocalPlayerTeamGroupId);

                        obs.SetGroupId(groupId);
                        if (isTeammate)
                        {
                            obs.UpdateTypeForTeammate(true);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Clusters players into teams using connected components algorithm.
        /// </summary>
        private static List<List<AbstractPlayer>> ClusterPlayers(
            List<AbstractPlayer> players, float threshold)
        {
            var visited = new HashSet<AbstractPlayer>();
            var teams = new List<List<AbstractPlayer>>();
            float thresholdSq = threshold * threshold;

            foreach (var player in players)
            {
                if (!visited.Contains(player))
                {
                    var team = new List<AbstractPlayer>();
                    FindComponent(player, players, visited, team, thresholdSq);
                    teams.Add(team);
                }
            }

            return teams;
        }

        /// <summary>
        /// BFS to find all players in the same connected component (team).
        /// </summary>
        private static void FindComponent(
            AbstractPlayer start,
            List<AbstractPlayer> allPlayers,
            HashSet<AbstractPlayer> visited,
            List<AbstractPlayer> component,
            float thresholdSq)
        {
            var queue = new Queue<AbstractPlayer>();
            queue.Enqueue(start);
            visited.Add(start);
            component.Add(start);

            while (queue.Count > 0)
            {
                var current = queue.Dequeue();

                foreach (var other in allPlayers)
                {
                    if (!visited.Contains(other))
                    {
                        var distSq = Vector3.DistanceSquared(current.Position, other.Position);

                        if (!ValidPosition(current.Position) || !ValidPosition(other.Position))
                            continue;

                        if (distSq <= thresholdSq)
                        {
                            visited.Add(other);
                            component.Add(other);
                            queue.Enqueue(other);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Validates that a position is usable for distance calc.
        /// </summary>
        private static bool ValidPosition(Vector3 pos)
        {
            return pos != Vector3.Zero &&
                   !float.IsNaN(pos.X) &&
                   !float.IsInfinity(pos.X);
        }

        /// <summary>
        /// Assigns GroupIDs to detected teams and marks teammates.
        /// </summary>
        private static Dictionary<int, int> AssignGroups(
            List<List<AbstractPlayer>> teams,
            LocalPlayer localPlayer)
        {
            int totalTeammates = 0;
            int totalEnemyTeams = 0;
            var assignedTeams = new Dictionary<int, int>();

            foreach (var team in teams)
            {
                bool isLocalPlayerTeam = team.Any(p =>
                {
                    if (!ValidPosition(p.Position))
                        return false;
                    var localPos = localPlayer.Position;
                    if (!ValidPosition(localPos))
                        return false;
                    var dist = Vector3.Distance(p.Position, localPos);
                    return dist <= TeammateDetectionDistance;
                });

                if (isLocalPlayerTeam)
                {
                    foreach (var player in team)
                    {
                        if (player is ObservedPlayer obs)
                        {
                            obs.SetGroupId(LocalPlayerTeamGroupId);
                            obs.UpdateTypeForTeammate(true);
                            assignedTeams[obs.PlayerId] = LocalPlayerTeamGroupId;
                            totalTeammates++;
                        }
                    }
                    DebugLogger.LogDebug($"[TeamDetection] Detected {team.Count} teammate(s) nearby - assigned GroupID {LocalPlayerTeamGroupId}");
                }
                else if (team.Count > 1) // Only assign GroupID to non-solo enemy teams
                {
                    int enemyGroupId = Interlocked.Increment(ref _lastGroupNumber);
                    totalEnemyTeams++;
                    DebugLogger.LogDebug($"[TeamDetection] Detected enemy team of {team.Count} player(s) - assigned GroupID {enemyGroupId}");

                    foreach (var player in team)
                    {
                        if (player is ObservedPlayer obs)
                        {
                            obs.SetGroupId(enemyGroupId);
                            assignedTeams[obs.PlayerId] = enemyGroupId;
                        }
                    }
                }
            }

            DebugLogger.LogDebug($"[TeamDetection] {totalTeammates} teammate(s), {totalEnemyTeams} enemy team(s)");
            return assignedTeams;
        }

        /// <summary>
        /// Pre-raid detection for Boss followers.
        /// Runs during pre-raid phase to cache guard data before raid starts.
        /// Handles new Boss/Guard appearances by merging with existing cache.
        /// </summary>
        public static void DetectBossFollowersPreRaid(LocalPlayer localPlayer, IEnumerable<AbstractPlayer> allPlayers)
        {
            if (localPlayer == null || allPlayers == null)
                return;

            // Load cached data (if any)
            var cachedGuards = RaidInfoCache.LoadBossFollowersData(localPlayer.RaidId, localPlayer.PlayerId) ?? new Dictionary<int, string>();

            // Apply cached data to existing players
            int appliedCount = 0;
            foreach (var player in allPlayers)
            {
                if (player is ObservedPlayer obs && obs.RaidId != 0 && cachedGuards.TryGetValue(obs.RaidId, out string role))
                {
                    if (role == "Guard" && obs.Type == PlayerType.AIScav)
                    {
                        obs.Type = PlayerType.AIGuard;
                        obs.Name = "Guard";
                        appliedCount++;
                    }
                }
            }

            const float GuardDetectionDistance = 10.0f;
            float thresholdSq = GuardDetectionDistance * GuardDetectionDistance;

            var bosses = allPlayers.Where(p => p.Type == PlayerType.AIBoss && p.IsActive && p.IsAlive).ToList();
            var potentialGuards = allPlayers.Where(p => p.Type == PlayerType.AIScav && p.IsActive && p.IsAlive).ToList();

            if (bosses.Count == 0 || potentialGuards.Count == 0)
                return;

            var newGuardData = new Dictionary<int, string>();

            foreach (var boss in bosses)
            {
                if (!ValidPosition(boss.Position))
                    continue;

                foreach (var scavenger in potentialGuards)
                {
                    if (scavenger.Type == PlayerType.AIGuard)
                        continue;

                    if (!ValidPosition(scavenger.Position))
                        continue;

                    var distSq = Vector3.DistanceSquared(boss.Position, scavenger.Position);

                    if (distSq <= thresholdSq)
                    {
                        if (scavenger is ObservedPlayer obs && obs.RaidId != 0 && !cachedGuards.ContainsKey(obs.RaidId))
                        {
                            obs.Type = PlayerType.AIGuard;
                            obs.Name = "Guard";
                            newGuardData[obs.RaidId] = "Guard";
                        }
                    }
                }
            }

            // If new guards were detected, merge with cache and save
            if (newGuardData.Count > 0)
            {
                foreach (var kvp in newGuardData)
                {
                    cachedGuards[kvp.Key] = kvp.Value;
                }
                RaidInfoCache.SaveBossFollowers(localPlayer.RaidId, localPlayer.PlayerId, cachedGuards);
            }
        }

        /// <summary>
        /// Detects Boss followers by proximity to Boss characters.
        /// Scavs within a certain distance of a Boss are marked as Guards.
        /// </summary>
        public static void DetectBossFollowers(LocalPlayer localPlayer, IEnumerable<AbstractPlayer> allPlayers)
        {
            if (localPlayer == null || allPlayers == null)
                return;

            var appliedGuards = RaidInfoCache.LoadBossFollowers(localPlayer.RaidId, localPlayer.PlayerId, allPlayers);

            if (appliedGuards.HasValue)
            {
                if (appliedGuards.Value > 0)
                {
                    DebugLogger.LogDebug($"[BossGuard] {appliedGuards.Value} guard(s)");
                }
                return;
            }

            const float GuardDetectionDistance = 10.0f;
            float thresholdSq = GuardDetectionDistance * GuardDetectionDistance;

            var bosses = allPlayers.Where(p => p.Type == PlayerType.AIBoss && p.IsActive && p.IsAlive).ToList();
            var potentialGuards = allPlayers.Where(p => p.Type == PlayerType.AIScav && p.IsActive && p.IsAlive).ToList();

            if (bosses.Count == 0 || potentialGuards.Count == 0)
                return;

            int guardsFound = 0;
            var guardDataToSave = new Dictionary<int, string>();

            foreach (var boss in bosses)
            {
                if (!ValidPosition(boss.Position))
                    continue;

                foreach (var scavenger in potentialGuards)
                {
                    if (scavenger.Type == PlayerType.AIGuard)
                        continue;

                    if (!ValidPosition(scavenger.Position))
                        continue;

                    var distSq = Vector3.DistanceSquared(boss.Position, scavenger.Position);

                    if (distSq <= thresholdSq)
                    {
                        if (scavenger is ObservedPlayer obs && obs.RaidId != 0)
                        {
                            obs.Type = PlayerType.AIGuard;
                            obs.Name = "Guard";
                            guardDataToSave[obs.RaidId] = "Guard";
                            guardsFound++;
                            DebugLogger.LogDebug($"[BossGuard] Detected guard '{obs.Name}' near boss '{boss.Name}' ({MathF.Sqrt(distSq):F1}m)");
                        }
                    }
                }
            }

            if (guardDataToSave.Count > 0)
            {
                RaidInfoCache.SaveBossFollowers(localPlayer.RaidId, localPlayer.PlayerId, guardDataToSave);
            }

            if (guardsFound > 0)
            {
                DebugLogger.LogDebug($"[BossGuard] {guardsFound} guard(s)");
            }
        }

        /// <summary>
        /// Detects Santa Claus by checking equipment IDs.
        /// Santa has a specific backpack (61b9e1aaef9a1b5d6a79899a).
        /// </summary>
        public static void DetectSanta(IEnumerable<AbstractPlayer> allPlayers)
        {
            if (allPlayers == null)
                return;

            foreach (var player in allPlayers)
            {
                if (player is ObservedPlayer obs)
                {
                    obs.CheckSanta();
                    if (obs.Name == "Santa")
                        return;
                }
            }
        }

        /// <summary>
        /// Detects Zryachiy by checking equipment IDs.
        /// Zryachiy has specific equipment: 63626d904aa74b8fe30ab426, 636270263f2495c26f00b007.
        /// </summary>
        public static void DetectZryachiy(IEnumerable<AbstractPlayer> allPlayers)
        {
            if (allPlayers == null)
                return;

            // not lighthouse map, skip
            if (Memory.Game == null || !Memory.Game.MapID.Equals("Lighthouse", StringComparison.OrdinalIgnoreCase))
                return;

            foreach (var player in allPlayers)
            {
                if (player is ObservedPlayer obs)
                {
                    obs.CheckZryachiy();
                }
            }
        }

        #endregion

        #region Cached Skia Paths

        private static readonly SKPath _playerPill = CreatePlayerPillBase();
        private static readonly SKPath _deathMarker = CreateDeathMarkerPath();
        private const float PP_LENGTH = 9f;
        private const float PP_RADIUS = 3f;
        private const float PP_HALF_HEIGHT = PP_RADIUS * 0.85f;
        private const float PP_NOSE_X = PP_LENGTH / 2f + PP_RADIUS * 0.18f;

        private static SKPath CreatePlayerPillBase()
        {
            var path = new SKPath();

            // Rounded back (left side)
            var backRect = new SKRect(-PP_LENGTH / 2f, -PP_HALF_HEIGHT, -PP_LENGTH / 2f + PP_RADIUS * 2f, PP_HALF_HEIGHT);
            path.AddArc(backRect, 90, 180);

            // Pointed nose (right side)
            float backFrontX = -PP_LENGTH / 2f + PP_RADIUS;

            float c1X = backFrontX + PP_RADIUS * 1.1f;
            float c2X = PP_NOSE_X - PP_RADIUS * 0.28f;
            float c1Y = -PP_HALF_HEIGHT * 0.55f;
            float c2Y = -PP_HALF_HEIGHT * 0.3f;

            path.CubicTo(c1X, c1Y, c2X, c2Y, PP_NOSE_X, 0f);
            path.CubicTo(c2X, -c2Y, c1X, -c1Y, backFrontX, PP_HALF_HEIGHT);

            path.Close();
            return path;
        }

        private static SKPath CreateDeathMarkerPath()
        {
            const float length = 6f;
            var path = new SKPath();

            path.MoveTo(-length, length);
            path.LineTo(length, -length);
            path.MoveTo(-length, -length);
            path.LineTo(length, length);

            return path;
        }

        #endregion

        #region Allocation

        /// <summary>
        /// Allocates a player.
        /// </summary>
        /// <param name="regPlayers">Player Dictionary collection to add the newly allocated player to.</param>
        /// <param name="playerBase">Player base memory address.</param>
        public static void Allocate(ConcurrentDictionary<ulong, AbstractPlayer> regPlayers, ulong playerBase)
        {
            try
            {
                _ = regPlayers.GetOrAdd(
                    playerBase,
                    addr => AllocateInternal(addr));
            }
            catch
            {
                // silently skip
            }
        }

        private static AbstractPlayer AllocateInternal(ulong playerBase)
        {
            AbstractPlayer player;
            var className = ObjectClass.ReadName(playerBase, 64);
            var isClientPlayer = className == "ClientPlayer" || className == "LocalPlayer";

            if (isClientPlayer)
                player = new ClientPlayer(playerBase);
            else
                player = new ObservedPlayer(playerBase);
            DebugLogger.LogDebug($"Player '{player.Name}' allocated.");
            return player;
        }

        /// <summary>
        /// Player Constructor.
        /// </summary>
        protected AbstractPlayer(ulong playerBase)
        {
            ArgumentOutOfRangeException.ThrowIfZero(playerBase, nameof(playerBase));
            Base = playerBase;
        }

        #endregion

        #region Fields / Properties
        /// <summary>
        /// Player Class Base Address
        /// </summary>
        public ulong Base { get; }

        /// <summary>
        /// True if the Player is Active (in the player list).
        /// </summary>
        public bool IsActive { get; private set; }

        /// <summary>
        /// Type of player unit.
        /// </summary>
        public virtual PlayerType Type { get; set; }

        private Vector2 _rotation;
        /// <summary>
        /// Player's Rotation in Local Game World.
        /// </summary>
        public Vector2 Rotation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _rotation;
            private set
            {
                _rotation = value;
                float mapRotation = value.X; // Cache value
                mapRotation -= 90f;
                while (mapRotation < 0f)
                    mapRotation += 360f;
                MapRotation = mapRotation;
            }
        }

        /// <summary>
        /// Player's Map Rotation (with 90 degree correction applied).
        /// </summary>
        public float MapRotation { get; private set; }

        /// <summary>
        /// Corpse field value.
        /// </summary>
        public ulong? Corpse { get; private set; }

        /// <summary>
        /// Player's Skeleton Root.
        /// </summary>
        public UnityTransform SkeletonRoot { get; protected set; }

        /// <summary>
        /// Dictionary of Player Bones.
        /// </summary>
        public ConcurrentDictionary<Bones, UnityTransform> PlayerBones { get; } = new();
        /// <summary>
        /// Lightweight wrapper for skeleton access (used by DeviceAimbot/silent aim features).
        /// </summary>
        public PlayerSkeleton Skeleton { get; protected set; }
        protected int _verticesCount;
        private bool _skeletonErrorLogged;
        protected Vector3 _cachedPosition; // Fallback position cache

        /// <summary>
        /// TRUE if critical memory reads (position/rotation) have failed.
        /// </summary>
        public bool IsError { get; set; }

        /// <summary>
        /// Timer to track how long player has been in error state.
        /// Used to trigger re-allocation if errors persist.
        /// </summary>
        public Stopwatch ErrorTimer { get; } = new Stopwatch();

        /// <summary>
        /// Reset a specific bone transform using its internal address.
        /// From Pre-1.0 fork
        /// </summary>
        /// <param name="bone">The bone to reset</param>
        private void ResetBoneTransform(Bones bone)
        {
            try
            {
                if (PlayerBones.TryGetValue(bone, out var boneTransform))
                {
                    PlayerBones[bone] = new UnityTransform(boneTransform.TransformInternal);
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"Failed to reset bone '{bone}' transform for Player '{Name ?? "Unknown"}': {ex}");
            }
        }

        /// <summary>
        /// True if player is being focused via Right-Click (UI).
        /// </summary>
        public bool IsFocused { get; set; }

        /// <summary>
        /// Dead Player's associated loot container object.
        /// </summary>
        public LootCorpse LootObject { get; set; }
        /// <summary>
        /// Alerts for this Player Object.
        /// Used by Player History UI Interop.
        /// </summary>
        public virtual string Alerts { get; protected set; }

        #endregion

        #region Virtual Properties

        /// <summary>
        /// Player name.
        /// </summary>
        public virtual string Name { get; set; }

        /// <summary>
        /// Account UUID for Human Controlled Players.
        /// </summary>
        public virtual string AccountID { get; }

        /// <summary>
        /// Group that the player belongs to.
        /// </summary>
        public virtual int GroupID { get; protected set; } = -1;

        /// <summary>
        /// Player's Faction.
        /// </summary>
        public virtual Enums.EPlayerSide PlayerSide { get; protected set; }

        /// <summary>
        /// Player is Human-Controlled.
        /// </summary>
        public virtual bool IsHuman { get; }

        /// <summary>
        /// MovementContext / StateContext
        /// </summary>
        public virtual ulong MovementContext { get; }

        /// <summary>
        /// Corpse field address..
        /// </summary>
        public virtual ulong CorpseAddr { get; }

        /// <summary>
        /// Player Rotation Field Address (view angles).
        /// </summary>
        public virtual ulong RotationAddress { get; }

        /// <summary>
        /// Hands Controller address.
        /// </summary>
        protected ulong HandsController { get; set; }

        #endregion

        #region Boolean Getters

        /// <summary>
        /// Player is AI-Controlled.
        /// </summary>
        public bool IsAI => !IsHuman;

        /// <summary>
        /// Player is a PMC Operator.
        /// </summary>
        public bool IsPmc => PlayerSide is Enums.EPlayerSide.Usec || PlayerSide is Enums.EPlayerSide.Bear;

        /// <summary>
        /// Player is a SCAV.
        /// </summary>
        public bool IsScav => PlayerSide is Enums.EPlayerSide.Savage;

        /// <summary>
        /// Player is alive (not dead).
        /// </summary>
        public bool IsAlive => Corpse is null;

        /// <summary>
        /// True if Player is Friendly to LocalPlayer.
        /// </summary>
        public bool IsFriendly =>
            this is LocalPlayer || Type is PlayerType.Teammate;

        /// <summary>
        /// True if player is Hostile to LocalPlayer.
        /// </summary>
        public bool IsHostile => !IsFriendly;

        /// <summary>
        /// Player is Alive/Active and NOT LocalPlayer.
        /// </summary>
        public bool IsNotLocalPlayerAlive =>
            this is not LocalPlayer && IsActive && IsAlive;

        /// <summary>
        /// Player is a Hostile PMC Operator.
        /// </summary>
        public bool IsHostilePmc => IsPmc && IsHostile;

        /// <summary>
        /// Player is human-controlled (Not LocalPlayer).
        /// </summary>
        public bool IsHumanOther => IsHuman && this is not LocalPlayer;

        /// <summary>
        /// Player is AI Controlled and Alive/Active.
        /// </summary>
        public bool IsAIActive => IsAI && IsActive && IsAlive;

        /// <summary>
        /// Player is AI Controlled and Alive/Active & their AI Role is default.
        /// </summary>
        public bool IsDefaultAIActive => IsAI && Name == "defaultAI" && IsActive && IsAlive;

        /// <summary>
        /// Player is human-controlled and Active/Alive.
        /// </summary>
        public bool IsHumanActive =>
            IsHuman && IsActive && IsAlive;

        /// <summary>
        /// Player is hostile and alive/active.
        /// </summary>
        public bool IsHostileActive => IsHostile && IsActive && IsAlive;

        /// <summary>
        /// Player is human-controlled & Hostile.
        /// </summary>
        public bool IsHumanHostile => IsHuman && IsHostile;

        /// <summary>
        /// Player is human-controlled, hostile, and Active/Alive.
        /// </summary>
        public bool IsHumanHostileActive => IsHumanHostile && IsActive && IsAlive;

        /// <summary>
        /// Player is friendly to LocalPlayer (including LocalPlayer) and Active/Alive.
        /// </summary>
        public bool IsFriendlyActive => IsFriendly && IsActive && IsAlive;

        /// <summary>
        /// Player has exfil'd/left the raid.
        /// </summary>
        public bool HasExfild => !IsActive && IsAlive;

        #endregion

        #region Methods

        private readonly Lock _alertsLock = new();
        /// <summary>
        /// Update the Alerts for this Player Object.
        /// </summary>
        /// <param name="alert">Alert to set.</param>
        public void UpdateAlerts(string alert)
        {
            if (alert is null)
                return;
            lock (_alertsLock)
            {
                if (Alerts is null)
                    Alerts = alert;
                else
                    Alerts = $"{alert} | {Alerts}";
            }
        }

        /// <summary>
        /// Validates the Rotation Address.
        /// </summary>
        /// <param name="rotationAddr">Rotation va</param>
        /// <returns>Validated rotation virtual address.</returns>
        protected static ulong ValidateRotationAddr(ulong rotationAddr)
        {
            var rotation = Memory.ReadValue<Vector2>(rotationAddr, false);
            if (!rotation.IsNormalOrZero() ||
                Math.Abs(rotation.X) > 360f ||
                Math.Abs(rotation.Y) > 90f)
                throw new ArgumentOutOfRangeException(nameof(rotationAddr));

            return rotationAddr;
        }

        /// <summary>
        /// Refreshes non-realtime player information. Call in the Registered Players Loop (T0).
        /// </summary>
        /// <param name="scatter"></param>
        /// <param name="registered"></param>
        /// <param name="isActiveParam"></param>
        public virtual void OnRegRefresh(VmmScatter scatter, ISet<ulong> registered, bool? isActiveParam = null)
        {
            if (isActiveParam is not bool isActive)
                isActive = registered.Contains(this);
            if (isActive)
            {
                SetAlive();
            }
            else if (IsAlive) // Not in list, but alive
            {
                scatter.PrepareReadPtr(CorpseAddr);
                scatter.Completed += (sender, x1) =>
                {
                    if (x1.ReadPtr(CorpseAddr, out var corpsePtr))
                        SetDead(corpsePtr);
                    else
                        SetExfild();
                };
            }
        }

        /// <summary>
        /// Mark player as dead.
        /// </summary>
        /// <param name="corpse">Corpse address.</param>
        public void SetDead(ulong corpse)
        {
            Corpse = corpse;
            IsActive = false;
        }

        /// <summary>
        /// Mark player as exfil'd.
        /// </summary>
        private void SetExfild()
        {
            Corpse = null;
            IsActive = false;
        }

        /// <summary>
        /// Mark player as alive.
        /// </summary>
        private void SetAlive()
        {
            Corpse = null;
            LootObject = null;
            IsActive = true;
        }

        /// <summary>
        /// Executed on each Realtime Loop.
        /// </summary>
        /// <param name="index">Scatter read index dedicated to this player.</param>
        public virtual void OnRealtimeLoop(VmmScatter scatter)
        {
            if (SkeletonRoot == null)
            {
                IsError = true;
                return;
            }

            int vertexCount = SkeletonRoot.Count;

            // Calculate actual vertex requirements including all bones
            int maxBoneRequirement = 0;
            foreach (var bone in PlayerBones.Values)
            {
                if (bone.Count > maxBoneRequirement)
                    maxBoneRequirement = bone.Count;
            }

            int actualRequired = Math.Max(vertexCount, maxBoneRequirement);

            if (actualRequired <= 0 || actualRequired > 10000)
            {
                try
                {
                    DebugLogger.LogDebug($"Invalid vertex count detected for '{Name}': {actualRequired} (skeleton: {vertexCount}, bones: {maxBoneRequirement})");

                    // Resetting all bone transforms
                    foreach (var bone in PlayerBones.Keys.ToList())
                    {
                        ResetBoneTransform(bone);
                    }

                    Skeleton = new PlayerSkeleton(SkeletonRoot, PlayerBones);
                    DebugLogger.LogDebug($"Fast skeleton recovery for Player '{Name}' - vertexCount was {actualRequired}");

                    vertexCount = SkeletonRoot.Count;
                    maxBoneRequirement = 0;
                    foreach (var bone in PlayerBones.Values)
                    {
                        if (bone.Count > maxBoneRequirement)
                            maxBoneRequirement = bone.Count;
                    }
                    actualRequired = Math.Max(vertexCount, maxBoneRequirement);
                }
                catch (Exception ex)
                {
                    DebugLogger.LogDebug($"ERROR in fast skeleton recovery for '{Name}': {ex}");
                }

                // If still bad after recovery attempt, skip this frame
                if (actualRequired <= 0 || actualRequired > 10000)
                {
                    IsError = true;
                    _verticesCount = 0;
                    return;
                }
            }

            scatter.PrepareReadValue<Vector2>(RotationAddress); // Rotation
            int requestedVertices = _verticesCount > 0 ? _verticesCount : actualRequired;
            scatter.PrepareReadArray<TrsX>(SkeletonRoot.VerticesAddr, requestedVertices); // ESP Vertices

            scatter.Completed += (sender, s) =>
            {
                bool successRot = s.ReadValue<Vector2>(RotationAddress, out var rotation) && SetRotation(rotation);
                bool successPos = false;

                if (s.ReadPooled<TrsX>(SkeletonRoot.VerticesAddr, requestedVertices) is IMemoryOwner<TrsX> vertices)
                {
                    using (vertices)
                    {
                        try
                        {
                            if (vertices.Memory.Span.Length >= requestedVertices)
                            {
                                try
                                {
                                    _ = SkeletonRoot.UpdatePosition(vertices.Memory.Span);
                                    successPos = true;
                                }
                                catch
                                {
                                    successPos = false;
                                    return;
                                }

                                foreach (var bonePair in PlayerBones)
                                {
                                    try
                                    {
                                        if (bonePair.Value.Count <= vertices.Memory.Span.Length)
                                        {
                                            bonePair.Value.UpdatePosition(vertices.Memory.Span);
                                        }
                                        else
                                        {
                                            ResetBoneTransform(bonePair.Key);
                                        }
                                    }
                                    catch
                                    {
                                        ResetBoneTransform(bonePair.Key);
                                    }
                                }

                                _cachedPosition = SkeletonRoot.Position;

                                if (_skeletonErrorLogged)
                                {
                                    DebugLogger.LogDebug($"Skeleton update successful for Player '{Name}'");
                                    _skeletonErrorLogged = false;
                                }
                            }
                            else
                            {
                                _verticesCount = 0;
                                successPos = false;
                            }
                        }
                        catch
                        {
                            successPos = false;
                        }
                    }
                }
                else
                {
                    successPos = false;
                }

                bool hasError = !successRot || !successPos;
                if (hasError && !IsError)
                {
                    ErrorTimer.Restart();
                }
                else if (!hasError && IsError)
                {
                    ErrorTimer.Stop();
                    ErrorTimer.Reset();
                }

                IsError = hasError;
            };
        }

        /// <summary>
        /// Override this method to validate custom transforms.
        /// </summary>
        public virtual void OnValidateTransforms()
        {
        }

        /// <summary>
        /// Executed on each Transform Validation Loop.
        /// </summary>
        /// <param name="round1">Index (round 1)</param>
        /// <param name="round2">Index (round 2)</param>
        public void OnValidateTransforms(VmmScatter round1, VmmScatter round2)
        {
            round1.PrepareReadPtr(SkeletonRoot.TransformInternal + UnitySDK.UnityOffsets.TransformAccess_HierarchyOffset); // Bone Hierarchy
            round1.Completed += (sender, x1) =>
            {
                if (x1.ReadPtr(SkeletonRoot.TransformInternal + UnitySDK.UnityOffsets.TransformAccess_HierarchyOffset, out var tra))
                {
                    round2.PrepareReadPtr(tra + UnitySDK.UnityOffsets.Hierarchy_VerticesOffset); // Vertices Ptr
                    round2.Completed += (sender, x2) =>
                    {
                        if (x2.ReadPtr(tra + UnitySDK.UnityOffsets.Hierarchy_VerticesOffset, out var verticesPtr))
                        {
                            if (SkeletonRoot.VerticesAddr != verticesPtr) // check if any addr changed
                            {
                                DebugLogger.LogDebug($"WARNING - SkeletonRoot Transform has changed for Player '{Name}'");
                                var transform = new UnityTransform(SkeletonRoot.TransformInternal);
                                SkeletonRoot = transform;
                                _verticesCount = 0; // force fresh vertex count on next read

                                // IMPORTANT: also rebuild all bone transforms and skeleton wrapper
                                try
                                {
                                    foreach (var bone in PlayerBones.Keys.ToList())
                                    {
                                        ResetBoneTransform(bone);
                                    }

                                    Skeleton = new PlayerSkeleton(SkeletonRoot, PlayerBones);
                                    DebugLogger.LogDebug($"Skeleton rebuilt for Player '{Name}'");
                                }
                                catch (Exception ex)
                                {
                                    DebugLogger.LogDebug($"ERROR rebuilding skeleton for '{Name}': {ex}");
                                }
                            }
                        }
                    };
                }
            };
        }

        /// <summary>
        /// Set player rotation (Direction/Pitch)
        /// </summary>
        protected virtual bool SetRotation(Vector2 rotation)
        {
            try
            {
                rotation.ThrowIfAbnormalAndNotZero(nameof(rotation));
                rotation.X = rotation.X.NormalizeAngle();
                ArgumentOutOfRangeException.ThrowIfLessThan(rotation.X, 0f);
                ArgumentOutOfRangeException.ThrowIfGreaterThan(rotation.X, 360f);
                ArgumentOutOfRangeException.ThrowIfLessThan(rotation.Y, -90f);
                ArgumentOutOfRangeException.ThrowIfGreaterThan(rotation.Y, 90f);
                Rotation = rotation;
                return true;
            }
            catch
            {
                return false;
            }
        }

        #endregion

        #region AI Player Types

        public readonly struct AIRole
        {
            public readonly string Name { get; init; }
            public readonly PlayerType Type { get; init; }
        }

        /// <summary>
        /// Lookup AI Info based on Voice Line.
        /// </summary>
        /// <param name="voiceLine"></param>
        /// <returns></returns>
        public static AIRole GetAIRoleInfo(string voiceLine)
        {
            switch (voiceLine)
            {
                case "BossSanitar":
                    return new AIRole
                    {
                        Name = "Sanitar",
                        Type = PlayerType.AIBoss
                    };
                case "BossBully":
                    return new AIRole
                    {
                        Name = "Reshala",
                        Type = PlayerType.AIBoss
                    };
                case "BossGluhar":
                    return new AIRole
                    {
                        Name = "Gluhar",
                        Type = PlayerType.AIBoss
                    };
                case "SectantPriest":
                    return new AIRole
                    {
                        Name = "Priest",
                        Type = PlayerType.AIBoss
                    };
                case "SectantWarrior":
                    return new AIRole
                    {
                        Name = "Cultist",
                        Type = PlayerType.AIRaider
                    };
                case "BossKilla":
                    return new AIRole
                    {
                        Name = "Killa",
                        Type = PlayerType.AIBoss
                    };
                case "BossTagilla":
                    return new AIRole
                    {
                        Name = "Tagilla",
                        Type = PlayerType.AIBoss
                    };
                case "Boss_Partizan":
                    return new AIRole
                    {
                        Name = "Partisan",
                        Type = PlayerType.AIBoss
                    };
                case "BossBigPipe":
                    return new AIRole
                    {
                        Name = "Big Pipe",
                        Type = PlayerType.AIBoss
                    };
                case "BossBirdEye":
                    return new AIRole
                    {
                        Name = "Birdeye",
                        Type = PlayerType.AIBoss
                    };
                case "BossKnight":
                    return new AIRole
                    {
                        Name = "Knight",
                        Type = PlayerType.AIBoss
                    };
                case "Arena_Guard_1":
                    return new AIRole
                    {
                        Name = "Arena Guard",
                        Type = PlayerType.AIScav
                    };
                case "Arena_Guard_2":
                    return new AIRole
                    {
                        Name = "Arena Guard",
                        Type = PlayerType.AIScav
                    };
                case "Boss_Kaban":
                    return new AIRole
                    {
                        Name = "Kaban",
                        Type = PlayerType.AIBoss
                    };
                case "Boss_Kollontay":
                    return new AIRole
                    {
                        Name = "Kollontay",
                        Type = PlayerType.AIBoss
                    };
                case "Boss_Sturman":
                    return new AIRole
                    {
                        Name = "Shturman",
                        Type = PlayerType.AIBoss
                    };
                case "Zombie_Generic":
                    return new AIRole
                    {
                        Name = "Zombie",
                        Type = PlayerType.AIScav
                    };
                case "BossZombieTagilla":
                    return new AIRole
                    {
                        Name = "Zombie Tagilla",
                        Type = PlayerType.AIBoss
                    };
                case "Zombie_Fast":
                    return new AIRole
                    {
                        Name = "Zombie",
                        Type = PlayerType.AIScav
                    };
                case "Zombie_Medium":
                    return new AIRole
                    {
                        Name = "Zombie",
                        Type = PlayerType.AIScav
                    };
                default:
                    break;
            }
            if (voiceLine.Contains("scav", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Scav",
                    Type = PlayerType.AIScav
                };
            if (voiceLine.Contains("boss", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Boss",
                    Type = PlayerType.AIBoss
                };
            if (voiceLine.Contains("usec", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Usec",
                    Type = PlayerType.AIRaider
                };
            if (voiceLine.Contains("bear", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Bear",
                    Type = PlayerType.AIRaider
                };
            if (voiceLine.Contains("black_division", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "BD",
                    Type = PlayerType.AIRaider
                };
            if (voiceLine.Contains("vsrf", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Vsrf",
                    Type = PlayerType.AIRaider
                };
            if (voiceLine.Contains("civilian", StringComparison.OrdinalIgnoreCase))
                return new AIRole
                {
                    Name = "Civ",
                    Type = PlayerType.AIScav
                };
            DebugLogger.LogDebug($"Unknown Voice Line: {voiceLine}");
            return new AIRole
            {
                Name = "AI",
                Type = PlayerType.AIScav
            };
        }

        #endregion

        #region Interfaces

        public void Draw(SKCanvas canvas, EftMapParams mapParams, LocalPlayer localPlayer)
        {
            DrawReference(canvas, mapParams, localPlayer, localPlayer.Position, false);
        }

        /// <summary>
        /// Draw this Entity on the Radar with custom reference position.
        /// </summary>
        public void DrawReference(SKCanvas canvas, EftMapParams mapParams, LocalPlayer localPlayer, Vector3 referencePosition, bool isFollowTarget = false)
        {
            try
            {
                var point = Position.ToMapPos(mapParams.Map).ToZoomedPos(mapParams);
                MouseoverPosition = new Vector2(point.X, point.Y);
                if (!IsAlive) // Player Dead -- Draw 'X' death marker and move on
                {
                    DrawDeathMarker(canvas, point);
                }
                else
                {
                    DrawPlayerPill(canvas, localPlayer, point);
                    if (this == localPlayer)
                        return;
                    var height = Position.Y - localPlayer.ReferenceHeight;
                    var refPos = referencePosition;
                    var dist = Vector3.Distance(refPos, Position);
                    var roundedHeight = (int)Math.Round(height);
                    var roundedDist = (int)Math.Round(dist);

                    // Draw height indicator on the left side
                    if (roundedHeight != 0)
                    {
                        string heightIndicator = roundedHeight > 0 ? $"▲{roundedHeight}" : $"▼{Math.Abs(roundedHeight)}";
                        DrawHeightIndicator(canvas, point, heightIndicator);
                    }

                    // Draw name and distance on the right side
                    using var lines = new PooledList<string>();
                    string name = IsError ? "ERROR" : Name;
                    string health = null;
                    if (this is ObservedPlayer observed)
                    {
                        health = observed.HealthStatus is Enums.ETagStatus.Healthy
                            ? null
                            : $" ({observed.HealthStatus})"; // Only display abnormal health status
                    }
                    lines.Add($"{name}{health}");

                    if (!isFollowTarget)
                    {
                        lines.Add($"{roundedDist}M");
                    }

                    DrawPlayerText(canvas, point, lines);
                }
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"WARNING! Player Draw Error: {ex}");
            }
        }

    
        public virtual ref readonly Vector3 Position
        {
            get
            {
                var skeletonPos = SkeletonRoot.Position;
                if (skeletonPos != Vector3.Zero && !float.IsNaN(skeletonPos.X) && !float.IsInfinity(skeletonPos.X))
                {
                    _cachedPosition = skeletonPos;
                    return ref SkeletonRoot.Position;
                }
                return ref _cachedPosition;
            }
        }
        public Vector2 MouseoverPosition { get; set; }

        /// <summary>
        /// Draws a Player Pill on this location.
        /// </summary>
        private void DrawPlayerPill(SKCanvas canvas, LocalPlayer localPlayer, SKPoint point)
        {
            var paints = GetPaints();
            if (this != localPlayer && RadarViewModel.MouseoverGroup is int grp && grp == GroupID)
                paints.Item1 = SKPaints.PaintMouseoverGroup;

            float scale = 1.65f * App.Config.UI.UIScale;

            canvas.Save();
            canvas.Translate(point.X, point.Y);
            canvas.Scale(scale, scale);
            canvas.RotateDegrees(MapRotation);

            SKPaints.ShapeOutline.StrokeWidth = paints.Item1.StrokeWidth * 1.3f;
            // Draw the pill
            canvas.DrawPath(_playerPill, SKPaints.ShapeOutline); // outline
            canvas.DrawPath(_playerPill, paints.Item1);

            var aimlineLength = this == localPlayer || (IsFriendly && App.Config.UI.TeammateAimlines) ?
                App.Config.UI.AimLineLength : 0;
            if (!IsFriendly &&
                !(IsAI && !App.Config.UI.AIAimlines) &&
                this.IsFacingTarget(localPlayer, App.Config.UI.MaxDistance)) // Hostile Player, check if aiming at a friendly (High Alert)
                aimlineLength = 9999;

            if (aimlineLength > 0)
            {
                // Draw line from nose tip forward
                canvas.DrawLine(PP_NOSE_X, 0, PP_NOSE_X + aimlineLength, 0, SKPaints.ShapeOutline); // outline
                canvas.DrawLine(PP_NOSE_X, 0, PP_NOSE_X + aimlineLength, 0, paints.Item1);
            }

            canvas.Restore();
        }

        /// <summary>
        /// Draws a Death Marker on this location.
        /// </summary>
        private static void DrawDeathMarker(SKCanvas canvas, SKPoint point)
        {
            float scale = App.Config.UI.UIScale;

            canvas.Save();
            canvas.Translate(point.X, point.Y);
            canvas.Scale(scale, scale);
            canvas.DrawPath(_deathMarker, SKPaints.PaintDeathMarker);
            canvas.Restore();
        }

        /// <summary>
        /// Draws Player Text on this location.
        /// </summary>
        private void DrawPlayerText(SKCanvas canvas, SKPoint point, IList<string> lines)
        {
            var paints = GetPaints();
            if (RadarViewModel.MouseoverGroup is int grp && grp == GroupID)
                paints.Item2 = SKPaints.TextMouseoverGroup;
            point.Offset(10.5f * App.Config.UI.UIScale, 0);
            foreach (var line in lines)
            {
                if (string.IsNullOrEmpty(line?.Trim()))
                    continue;


                canvas.DrawText(line, point, SKTextAlign.Left, SKFonts.UIRegular, SKPaints.TextOutline); // Draw outline
                canvas.DrawText(line, point, SKTextAlign.Left, SKFonts.UIRegular, paints.Item2); // draw line text

                point.Offset(0, 12f * App.Config.UI.UIScale); // Compact
            }
        }

        /// <summary>
        /// Draws Height Indicator on the left side of player pill.
        /// </summary>
        private void DrawHeightIndicator(SKCanvas canvas, SKPoint point, string heightIndicator)
        {
            var paints = GetPaints();
            if (RadarViewModel.MouseoverGroup is int grp && grp == GroupID)
                paints.Item2 = SKPaints.TextMouseoverGroup;
            var leftPoint = new SKPoint(point.X - 10.5f * App.Config.UI.UIScale, point.Y + 5f * App.Config.UI.UIScale);

            canvas.DrawText(heightIndicator, leftPoint, SKTextAlign.Right, SKFonts.UIRegular, SKPaints.TextOutline); // Draw outline
            canvas.DrawText(heightIndicator, leftPoint, SKTextAlign.Right, SKFonts.UIRegular, paints.Item2); // draw text
        }


        private ValueTuple<SKPaint, SKPaint> GetPaints()
        {
            if (this is LocalPlayer)
                return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintLocalPlayer, SKPaints.TextLocalPlayer);

            if (Type == PlayerType.Teammate)
                return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintTeammate, SKPaints.TextTeammate);

            if (IsFocused)
                return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintFocused, SKPaints.TextFocused);

            switch (Type)
            {
                case PlayerType.PMC:
                    return PlayerSide switch
                    {
                        Enums.EPlayerSide.Bear => new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintPMCBear, SKPaints.TextPMCBear),
                        Enums.EPlayerSide.Usec => new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintPMCUsec, SKPaints.TextPMCUsec),
                        _ => new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintPMC, SKPaints.TextPMC)
                    };
                case PlayerType.AIScav:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintScav, SKPaints.TextScav);
                case PlayerType.AIGuard:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintGuard, SKPaints.TextGuard);
                case PlayerType.AIRaider:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintRaider, SKPaints.TextRaider);
                case PlayerType.AIBoss:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintBoss, SKPaints.TextBoss);
                case PlayerType.PScav:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintPScav, SKPaints.TextPScav);
                default:
                    return new ValueTuple<SKPaint, SKPaint>(SKPaints.PaintPMC, SKPaints.TextPMC);
            }
        }

        public void DrawMouseover(SKCanvas canvas, EftMapParams mapParams, LocalPlayer localPlayer)
        {
            if (this == localPlayer)
                return;
            using var lines = new PooledList<string>();
            var name = Name;
            string health = null;
            if (this is ObservedPlayer observed)
                health = observed.HealthStatus is Enums.ETagStatus.Healthy
                    ? null
                    : $" ({observed.HealthStatus.ToString()})"; // Only display abnormal health status
            string alert = Alerts?.Trim();
            if (!string.IsNullOrEmpty(alert)) // Special Players,etc.
                lines.Add(alert);
            if (IsHostileActive) // Enemy Players, display information
            {
                lines.Add($"{name}{health}");
                var faction = PlayerSide.ToString().ToUpper();
                string g = null;
                if (GroupID != -1)
                    g = $" G:{GroupID} ";
                lines.Add($"{faction}{g}");
            }
            else if (!IsAlive)
            {
                lines.Add($"{Type.ToString()}:{name}");
                string g = null;
                if (GroupID != -1)
                    g = $"G:{GroupID} ";
                if (g is not null) lines.Add(g);
            }
            else if (IsAIActive)
            {
                lines.Add(name);
            }

            if (this is ObservedPlayer obs2 && obs2.Equipment.Items is IReadOnlyDictionary<string, TarkovMarketItem> equipment)
            {
                // This is outside of the previous conditionals to always show equipment even if they're dead,etc.
                lines.Add($"Value: {Utilities.FormatNumberKM(obs2.Equipment.Value)}");
                foreach (var item in equipment.OrderBy(e => e.Key))
                {
                    lines.Add($"{item.Key.Substring(0, 5)}: {item.Value.ShortName}");
                }
            }

            Position.ToMapPos(mapParams.Map).ToZoomedPos(mapParams).DrawMouseoverText(canvas, lines.Span);
        }

        #endregion

        #region High Alert

        /// <summary>
        /// True if Current Player is facing <paramref name="target"/>.
        /// </summary>
        public bool IsFacingTarget(AbstractPlayer target, float? maxDist = null)
        {
            Vector3 delta = target.Position - this.Position;

            if (maxDist is float m)
            {
                float maxDistSq = m * m;
                float distSq = Vector3.Dot(delta, delta);
                if (distSq > maxDistSq) return false;
            }

            float distance = delta.Length();
            if (distance <= 1e-6f)
                return true;

            Vector3 fwd = RotationToDirection(this.Rotation);

            float cosAngle = Vector3.Dot(fwd, delta) / distance;

            const float A = 31.3573f;
            const float B = 3.51726f;
            const float C = 0.626957f;
            const float D = 15.6948f;

            float x = MathF.Abs(C - D * distance);
            float angleDeg = A - B * MathF.Log(MathF.Max(x, 1e-6f));
            if (angleDeg < 1f) angleDeg = 1f;
            if (angleDeg > 179f) angleDeg = 179f;

            float cosThreshold = MathF.Cos(angleDeg * (MathF.PI / 180f));
            return cosAngle >= cosThreshold;

            static Vector3 RotationToDirection(Vector2 rotation)
            {
                float yaw = rotation.X * (MathF.PI / 180f);
                float pitch = rotation.Y * (MathF.PI / 180f);

                float cp = MathF.Cos(pitch);
                float sp = MathF.Sin(pitch);
                float sy = MathF.Sin(yaw);
                float cy = MathF.Cos(yaw);

                var dir = new Vector3(
                    cp * sy,
                   -sp,
                    cp * cy
                );

                float lenSq = Vector3.Dot(dir, dir);
                if (lenSq > 0f && MathF.Abs(lenSq - 1f) > 1e-4f)
                {
                    float invLen = 1f / MathF.Sqrt(lenSq);
                    dir *= invLen;
                }
                return dir;
            }
        }

        /// <summary>
        /// Get Bone Position (if available).
        /// </summary>
        /// <param name="bone">Bone Index.</param>
        /// <returns>World Position of Bone, or SkeletonRoot position as fallback.</returns>
        public Vector3 GetBonePos(Bones bone)
        {
            try
            {
                if (PlayerBones.TryGetValue(bone, out var boneTransform))
                {
                    var pos = boneTransform.Position;
                    // Validate the position is reasonable (not zero, not NaN/Infinity)
                    if (pos != Vector3.Zero && !float.IsNaN(pos.X) && !float.IsInfinity(pos.X))
                        return pos;
                }
            }
            catch { }

            // Fallback to skeleton root position instead of zero
            // This prevents players from "teleporting" to the origin
            var rootPos = SkeletonRoot?.Position ?? Vector3.Zero;
            if (rootPos != Vector3.Zero && !float.IsNaN(rootPos.X) && !float.IsInfinity(rootPos.X))
                return rootPos;

            return Position; // Ultimate fallback to cached position
        }

        #endregion
    }

    /// <summary>
    /// Simple wrapper exposing skeleton root and bone transforms for aim helpers.
    /// </summary>
    public sealed class PlayerSkeleton
    {
        public PlayerSkeleton(UnityTransform root, ConcurrentDictionary<Bones, UnityTransform> bones)
        {
            Root = root;
            BoneTransforms = bones;
        }

        public UnityTransform Root { get; }
        public ConcurrentDictionary<Bones, UnityTransform> BoneTransforms { get; }
    }
}
