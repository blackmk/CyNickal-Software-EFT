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

using LoneEftDmaRadar.Tarkov.GameWorld.Loot;
using MessagePack;

namespace LoneEftDmaRadar.Web.WebRadar.Data
{
    /// <summary>
    /// Type of loot for web radar.
    /// </summary>
    public enum WebLootType : byte
    {
        Regular = 0,
        Valuable = 1,
        Important = 2,
        QuestItem = 3,
        QuestHelper = 4,
        Corpse = 5,
        Container = 6,
        Airdrop = 7
    }

    [MessagePackObject]
    public readonly struct WebRadarLoot
    {
        /// <summary>
        /// Loot Name (Short Name).
        /// </summary>
        [Key(0)]
        public readonly string Name { get; init; }

        /// <summary>
        /// Loot Type.
        /// </summary>
        [Key(1)]
        public readonly WebLootType Type { get; init; }

        /// <summary>
        /// Price in roubles.
        /// </summary>
        [Key(2)]
        public readonly int Price { get; init; }

        /// <summary>
        /// Unity World Position.
        /// </summary>
        [Key(3)]
        public readonly Vector3 Position { get; init; }

        /// <summary>
        /// Create a WebRadarLoot from a LootItem object.
        /// </summary>
        public static WebRadarLoot Create(LootItem item)
        {
            WebLootType type;
            
            if (item is LootCorpse)
                type = WebLootType.Corpse;
            else if (item is LootAirdrop)
                type = WebLootType.Airdrop;
            else if (item is StaticLootContainer)
                type = WebLootType.Container;
            else if (item.IsQuestItem)
                type = WebLootType.QuestItem;
            else if (item.IsQuestHelperItem)
                type = WebLootType.QuestHelper;
            else if (item.Important)
                type = WebLootType.Important;
            else if (item.IsValuableLoot)
                type = WebLootType.Valuable;
            else
                type = WebLootType.Regular;

            return new WebRadarLoot
            {
                Name = item.ShortName ?? item.Name ?? "Unknown",
                Type = type,
                Price = item.Price,
                Position = item.Position
            };
        }
    }
}
