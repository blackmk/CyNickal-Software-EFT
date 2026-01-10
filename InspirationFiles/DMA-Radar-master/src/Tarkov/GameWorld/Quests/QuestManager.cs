using Collections.Pooled;
using LoneEftDmaRadar.Tarkov.Unity.Collections;

namespace LoneEftDmaRadar.Tarkov.GameWorld.Quests
{
    public sealed class QuestManager
    {
        private readonly ulong _profile;
        private DateTimeOffset _last = DateTimeOffset.MinValue;

        public QuestManager(ulong profile)
        {
            _profile = profile;
        }

        private readonly ConcurrentDictionary<string, QuestEntry> _quests = new(StringComparer.OrdinalIgnoreCase); // Key = Quest ID
        /// <summary>
        /// All current quests.
        /// </summary>
        public IReadOnlyDictionary<string, QuestEntry> Quests => _quests;

        private readonly ConcurrentDictionary<string, byte> _items = new(StringComparer.OrdinalIgnoreCase); // Key = Item ID
        /// <summary>
        /// All item BSG ID's that we need to pickup.
        /// </summary>
        public IReadOnlyDictionary<string, byte> ItemConditions => _items;
        private readonly ConcurrentDictionary<string, QuestLocation> _locations = new(StringComparer.OrdinalIgnoreCase); // Key = Target ID
        /// <summary>
        /// All locations that we need to visit.
        /// </summary>
        public IReadOnlyDictionary<string, QuestLocation> LocationConditions => _locations;

        /// <summary>
        /// Map Identifier of Current Map.
        /// </summary>
        private static string MapID
        {
            get
            {
                var id = Memory.MapID;
                id ??= "MAPDEFAULT";
                return id;
            }
        }

        public void Refresh(CancellationToken ct)
        {
            try
            {
                var now = DateTimeOffset.UtcNow;
                if (now - _last < TimeSpan.FromSeconds(1))
                    return;
                using var masterQuests = new PooledSet<string>(StringComparer.OrdinalIgnoreCase);
                using var masterItems = new PooledSet<string>(StringComparer.OrdinalIgnoreCase);
                using var masterLocations = new PooledSet<string>(StringComparer.OrdinalIgnoreCase);
                var questsData = Memory.ReadPtr(_profile + Offsets.Profile.QuestsData);
                using var questsDataList = UnityList<ulong>.Create(questsData, false);
                foreach (var qDataEntry in questsDataList)
                {
                    ct.ThrowIfCancellationRequested();
                    try
                    {
                        //qDataEntry should be public class QuestStatusData : Object
                        var qStatus = Memory.ReadValue<int>(qDataEntry + Offsets.QuestsData.Status);
                        if (qStatus != 2) // started
                            continue;
                        var qId = Memory.ReadUnityString(Memory.ReadPtr(qDataEntry + Offsets.QuestsData.Id));
                        // qID should be Task ID
                        if (!TarkovDataManager.TaskData.TryGetValue(qId, out var task))
                            continue;
                        masterQuests.Add(qId);
                        _ = _quests.GetOrAdd(
                            qId,
                            id => new QuestEntry(id));
                        if (App.Config.QuestHelper.BlacklistedQuests.ContainsKey(qId))
                            continue; // Log the quest but dont get any conditions
                        using var completedHS = UnityHashSet<MongoID>.Create(Memory.ReadPtr(qDataEntry + Offsets.QuestsData.CompletedConditions), true);
                        using var completedConditions = new PooledSet<string>(StringComparer.OrdinalIgnoreCase);
                        foreach (var c in completedHS)
                        {
                            var completedCond = c.Value.ReadString();
                            completedConditions.Add(completedCond);
                        }

                        FilterConditions(task, qId, completedConditions, masterItems, masterLocations);
                    }
                    catch
                    {

                    }
                }
                // Remove stale Quests/Items/Locations
                foreach (var oldQuest in _quests)
                {
                    if (!masterQuests.Contains(oldQuest.Key))
                    {
                        _quests.TryRemove(oldQuest.Key, out _);
                    }
                }
                foreach (var oldItem in _items)
                {
                    if (!masterItems.Contains(oldItem.Key))
                    {
                        _items.TryRemove(oldItem.Key, out _);
                    }
                }
                foreach (var oldLoc in _locations.Keys)
                {
                    if (!masterLocations.Contains(oldLoc))
                    {
                        _locations.TryRemove(oldLoc, out _);
                    }
                }
                _last = now;
            }
            catch (OperationCanceledException) { throw; }
            catch (Exception ex)
            {
                Debug.WriteLine($"[QuestManager] CRITICAL ERROR: {ex}");
            }
        }


        private void FilterConditions(TarkovDataManager.TaskElement task, string questId, PooledSet<string> completedConditions, PooledSet<string> masterItems, PooledSet<string> masterLocations)
        {
            if (task is null)
                return;
            if (task.Objectives is null)
                return;
            foreach (var objective in task.Objectives)
            {
                try
                {
                    if (objective is null)
                        continue;

                    // Skip objectives that are already completed (by condition id)
                    if (!string.IsNullOrEmpty(objective.Id) && completedConditions.Contains(objective.Id))
                        continue;
                    //skip Type: buildWeapon, giveQuestItem, extract, shoot, traderLevel, giveItem
                    if (objective.Type == QuestObjectiveType.BuildWeapon
                        || objective.Type == QuestObjectiveType.GiveQuestItem
                        || objective.Type == QuestObjectiveType.Extract
                        || objective.Type == QuestObjectiveType.Shoot
                        || objective.Type == QuestObjectiveType.TraderLevel
                        || objective.Type == QuestObjectiveType.GiveItem)
                    {
                        continue;
                    }

                    // Item Pickup Objectives findItem and findQuestItem
                    if (objective.Type == QuestObjectiveType.FindItem
                        || objective.Type == QuestObjectiveType.FindQuestItem)
                    {
                        if (objective.QuestItem?.Id is not null)
                        {
                            masterItems.Add(objective.QuestItem.Id);
                            _ = _items.GetOrAdd(objective.QuestItem.Id, 0);
                        }
                    }
                    // Location Visit Objectives visitLocation
                    else if (objective.Type == QuestObjectiveType.Visit
                        || objective.Type == QuestObjectiveType.Mark
                        || objective.Type == QuestObjectiveType.PlantItem)
                    {
                        if (objective.Zones is not null && objective.Zones.Count > 0)
                        {
                            if (TarkovDataManager.TaskZones.TryGetValue(MapID, out var zonesForMap))
                            {
                                foreach (var zone in objective.Zones)
                                {
                                    if (zone?.Id is string zoneId && zonesForMap.TryGetValue(zoneId, out var pos))
                                    {
                                        // Make a stable key for this quest-objective-zone triple
                                        var locKey = $"{questId}:{objective.Id}:{zoneId}";
                                        _locations.GetOrAdd(locKey, _ => new QuestLocation(questId, objective.Id, pos));
                                        masterLocations.Add(locKey);
                                    }
                                }
                            }
                        }
                    }
                }
                catch
                {
                }
            }
        }
    }
}