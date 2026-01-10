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

using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.Unity;
using LoneEftDmaRadar.UI.Radar.Maps;
using LoneEftDmaRadar.UI.Skia;
using System.Collections.Frozen;

namespace LoneEftDmaRadar.Tarkov.GameWorld.Exits
{
    public class Exfil : IExitPoint, IWorldEntity, IMapEntity, IMouseoverEntity
    {
        public EStatus Status { get; private set; } = EStatus.Closed;
        public Exfil(ulong baseAddr, string exfilName, string mapId, bool IsPmc, Vector3 position)
        {
            exfilBase = baseAddr;

            if (ExfilNames.TryGetValue(mapId.ToLower(), out var mapExfils) && mapExfils.TryGetValue(exfilName, out var outName))
            {
                Name = outName;
            }

            _position = position;

        }

        public void Update(Enums.EExfiltrationStatus status)
        {
            /// Update Status
            switch (status)
            {
                case Enums.EExfiltrationStatus.NotPresent:
                    Status = EStatus.Closed;
                    break;
                case Enums.EExfiltrationStatus.UncompleteRequirements:
                    Status = EStatus.Pending;
                    break;
                case Enums.EExfiltrationStatus.Countdown:
                    Status = EStatus.Open;
                    break;
                case Enums.EExfiltrationStatus.RegularMode:
                    Status = EStatus.Open;
                    break;
                case Enums.EExfiltrationStatus.Pending:
                    Status = EStatus.Pending;
                    break;
                case Enums.EExfiltrationStatus.AwaitsManualActivation:
                    Status = EStatus.Pending;
                    break;
                case Enums.EExfiltrationStatus.Hidden:
                    Status = EStatus.Closed;
                    break;
            }
        }

        public string Name { get; }

        #region Interfaces

        public ulong exfilBase { get; init; }

        private readonly Vector3 _position;
        public ref readonly Vector3 Position => ref _position;
        public Vector2 MouseoverPosition { get; set; }

        public void Draw(SKCanvas canvas, EftMapParams mapParams, LocalPlayer localPlayer)
        {
            var heightDiff = Position.Y - localPlayer.Position.Y;
            var paint = SKPaints.PaintExfil;
            if (Status == EStatus.Open)
            {
                SKPaints.PaintExfil.Color = SKColors.Green;
            }
            else if (Status == EStatus.Pending)
            {
                SKPaints.PaintExfil.Color = SKColors.Yellow;
            }
            else // Closed
            {
                SKPaints.PaintExfil.Color = SKColors.Red;
            }
            var point = Position.ToMapPos(mapParams.Map).ToZoomedPos(mapParams);
            MouseoverPosition = new Vector2(point.X, point.Y);
            SKPaints.ShapeOutline.StrokeWidth = 2f;
            if (heightDiff > 1.85f) // exfil is above player
            {
                using var path = point.GetUpArrow(6.5f);
                canvas.DrawPath(path, SKPaints.ShapeOutline);
                canvas.DrawPath(path, paint);
            }
            else if (heightDiff < -1.85f) // exfil is below player
            {
                using var path = point.GetDownArrow(6.5f);
                canvas.DrawPath(path, SKPaints.ShapeOutline);
                canvas.DrawPath(path, paint);
            }
            else // exfil is level with player
            {
                float size = 4.75f * App.Config.UI.UIScale;
                canvas.DrawCircle(point, size, SKPaints.ShapeOutline);
                canvas.DrawCircle(point, size, paint);
            }
        }

        public void DrawMouseover(SKCanvas canvas, EftMapParams mapParams, LocalPlayer localPlayer)
        {
            List<string> lines = new();
            var exfilName = Name;
            exfilName ??= "unknown";
            lines.Add($"{exfilName} ({Status.ToString()})");
            Position.ToMapPos(mapParams.Map).ToZoomedPos(mapParams).DrawMouseoverText(canvas, string.Join("\n", lines));
        }

        #endregion

        public enum EStatus
        {
            [Description(nameof(Open))] Open,
            [Description(nameof(Pending))] Pending,
            [Description(nameof(Closed))] Closed
        }

        public static FrozenDictionary<string, FrozenDictionary<string, string>> ExfilNames { get; } = new Dictionary<string, FrozenDictionary<string, string>>(StringComparer.OrdinalIgnoreCase)
        {
            { "woods", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Factory Gate"] = "Friendship Bridge (Co-Op)",
                ["RUAF Gate"] = "RUAF Gate",
                ["ZB-016"] = "ZB-016",
                ["ZB-014"] = "ZB-014",
                ["UN Roadblock"] = "UN Roadblock",
                ["South V-Ex"] = "Bridge V-Ex",
                ["Outskirts"] = "Outskirts",
                ["un-sec"] = "Northern UN Roadblock",
                ["wood_sniper_exit"] = "Power Line Passage (Flare)",
                ["woods_secret_minefield"] = "Railway Bridge to Tarkov (Secret)",

                // SCAV
                ["Friendship Bridge (Co-Op)"] = "Friendship Bridge (Co-Op)",
                ["Outskirts Water"] = "Scav Bridge",
                ["Dead Man's Place"] = "Dead Man's Place",
                ["The Boat"] = "Boat",
                ["Scav House"] = "Scav House",
                ["East Gate"] = "Scav Bunker",
                ["Mountain Stash"] = "Mountain Stash",
                ["West Border"] = "Eastern Rocks",
                ["Old Station"] = "Old Railway Depot",
                ["RUAF Roadblock"] = "RUAF Roadblock",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "shoreline", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Shorl_V-Ex"] = "Road to North V-Ex",
                ["Road to Customs"] = "Road to Customs",
                ["Road_at_railbridge"] = "Railway Bridge",
                ["Tunnel"] = "Tunnel",
                ["Lighthouse_pass"] = "Path to Lighthouse",
                ["Smugglers_Trail_coop"] = "Smuggler's Path (Co-op)",
                ["Pier Boat"] = "Pier Boat",
                ["RedRebel_alp"] = "Climber's Trail",
                ["shoreline_secret_heartbeat"] = "Mountain Bunker (Secret)",
                // SCAV
                ["Scav Road to Customs"] = "Road to Customs",
                ["Lighthouse"] = "Lighthouse",
                ["Wrecked Road"] = "Ruined Road",
                ["South Fence Passage"] = "Old Bunker",
                ["RWing Gym Entrance"] = "East Wing Gym Entrance",
                ["Adm Basement"] = "Admin Basement",
                ["Smuggler's Path (Co-op)"] = "Smuggler's Path (Co-op)",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "rezervbase", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["EXFIL_Bunker_D2"] = "D-2",
                ["EXFIL_Bunker"] = "Bunker Hermetic Door",
                ["Alpinist"] = "Cliff Descent",
                ["EXFIL_ScavCooperation"] = "Scav Lands (Co-op)",
                ["EXFIL_vent"] = "Sewer Manhole",
                ["EXFIL_Train"] = "Armored Train",
                ["reserve_secret_minefield"] = "Exit to Woods (Secret)",
                // SCAV
                ["Bunker Hermetic Door"] = "Depot Hermetic Door",
                ["Scav Lands (Co-Op)"] = "Scav Lands (Co-Op)",
                ["Sewer Manhole"] = "Sewer Manhole",
                ["Exit1"] = "Hole in the Wall by the Mountains",
                ["Exit2"] = "Heating Pipe",
                ["Exit3"] = "??",
                ["Exit4"] = "Checkpoint Fence",
                ["Armored Train"] = "Armored Train",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "Labyrinth", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["labir_exit"] = "The Way Up",
                ["labyrinth_secret_tagilla_key"] = "Ariadne's Path (Secret)"
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "laboratory", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["lab_Elevator_Cargo"] = "Cargo Elevator",
                ["lab_Elevator_Main"] = "Main Elevator",
                ["lab_Vent"] = "Ventilation Shaft",
                ["lab_Elevator_Med"] = "Medical Block Elevator",
                ["lab_Under_Storage_Collector"] = "Sewage Conduit",
                ["lab_Parking_Gate"] = "Parking Gate",
                ["lab_Hangar_Gate"] = "Hangar Gate"
                // No Scav Exfils

            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "interchange", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["SE Exfil"] = "Emercom Checkpoint",
                ["NW Exfil"] = "Railway Exfil",
                ["PP Exfil"] = "Power Station V-Ex",
                ["Interchange Cooperation"] = "Scav Camp (Co-Op)",
                ["Hole Exfill"] = "Hole in the Fence",
                ["Saferoom Exfil"] = "Saferoom Exfil",
                ["shopping_sniper_exit"] = "Path to River (Flare)",
                // SCAV
                ["Emercom Checkpoint"] = "Emercom Checkpoint",
                ["Railway Exfil"] = "Railway Exfil",
                ["Scav Camp (Co-Op)"] = "Scav Camp (Co-Op)",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "factory4_day", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Cellars"] = "Cellars",
                ["Gate 3"] = "Gate 3",
                ["Gate 0"] = "Gate 0",
                ["Gate m"] = "Med Tent Gate",
                ["Gate_o"] = "Courtyard Gate",
                ["factory_secret_ark"] = "Smugglers' Passage (Secret)",
                // SCAV
                ["Camera Bunker Door"] = "Camera Bunker Door",
                ["Office Window"] = "Office Window",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "factory4_night", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Cellars"] = "Cellars",
                ["Gate 3"] = "Gate 3",
                ["Gate 0"] = "Gate 0",
                ["Gate m"] = "Med Tent Gate",
                ["Gate_o"] = "Courtyard Gate",
                ["factory_secret_ark"] = "Smugglers' Passage (Secret)",
                // SCAV
                ["Camera Bunker Door"] = "Camera Bunker Door",
                ["Office Window"] = "Office Window",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "bigmap", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["EXFIL_ZB013"] = "ZB-013",
                ["Dorms V-Ex"] = "Dorms V-Ex",
                ["ZB-1011"] = "ZB-1011",
                ["Crossroads"] = "Crossroads",
                ["Old Gas Station"] = "Old Gas Station",
                ["Trailer Park"] = "Trailer Park",
                ["RUAF Roadblock"] = "RUAF Roadblock",
                ["Smuggler's Boat"] = "Smuggler's Boat",
                ["ZB-1012"] = "ZB-1012",
                ["customs_secret_voron_boat"] = "Smugglers' Boat (Secret)",
                ["customs_secret_voron_bunker"] = "Smugglers' Bunker (ZB-1012) (Secret)",
                ["Custom_scav_pmc"] = "Boiler Room Basement (Co-op)",
                ["customs_sniper_exit"] = "Railroad Passage (Flare)",
                // SCAV
                ["Shack"] = "Military Base CP",
                ["Beyond Fuel Tank"] = "Passage Between Rocks",
                ["Railroad To Military Base"] = "Railroad to Military Base",
                ["Old Road Gate"] = "Old Road Gate",
                ["Sniper Roadblock"] = "Sniper Roadblock",
                ["Railroad To Port"] = "Railroad To Port",
                ["Trailer Park Workers Shack"] = "Trailer Park Workers Shack",
                ["Railroad To Tarkov"] = "Railroad To Tarkov",
                ["RUAF Roadblock_scav"] = "RUAF Roadblock",
                ["Warehouse 17"] = "Warehouse 17",
                ["Factory Shacks"] = "Factory Shacks",
                ["Warehouse 4"] = "Warehouse 4",
                ["Old Azs Gate"] = "Old Gas Station",
                ["Factory Far Corner"] = "Factory Far Corner",
                ["Administration Gate"] = "Administration Gate",
                ["Military Checkpoint"] = "Scav Checkpoint",
                ["Customs_scav_pmc"] = "Boiler Room Basement (Co-op)",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "lighthouse", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["V-Ex_light"] = "Road to Military Base V-Ex",
                ["tunnel_shared"] = "Side Tunnel (Co-Op)",
                ["Alpinist_light"] = "Mountain Pass",
                ["Shorl_free"] = "Path to Shoreline",
                ["Nothern_Checkpoint"] = "Northern Checkpoint",
                ["Coastal_South_Road"] = "Southern Road",
                ["EXFIL_Train"] = "Armored Train",
                ["lighthouse_secret_minefield"] = "Passage by the Lake (Secret)",
                // SCAV
                ["Side Tunnel (Co-Op)"] = "Side Tunnel (Co-Op)",
                ["Shorl_free_scav"] = "Path to Shoreline",
                ["Scav_Coastal_South"] = "Southern Road",
                ["Scav_Underboat_Hideout"] = "Hideout Under the Landing Stage",
                ["Scav_Hideout_at_the_grotto"] = "Scav Hideout at the Grotto",
                ["Scav_Industrial_zone"] = "Industrial Zone Gates",
                ["Armored Train"] = "Armored Train",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "tarkovstreets", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["E8_yard"] = "Courtyard",
                ["E7_car"] = "Primorsky Ave Taxi V-Ex",
                ["E1"] = "Stylobate Building Elevator",
                ["E4"] = "Crash Site",
                ["E2"] = "Sewer River",
                ["E3"] = "Damaged House",
                ["E5"] = "Collapsed Crane",
                ["E6"] = "??",
                ["E9_sniper"] = "Klimov Street",
                ["Exit_E10_coop"] = "Pinewood Basement (Co-Op)",
                ["E7"] = "Expo Checkpoint",
                ["streets_secret_onyx"] = "Smugglers' Basement (Secret)",
                // SCAV
                ["scav_e1"] = "Basement Descent",
                ["scav_e2"] = "Entrance to Catacombs",
                ["scav_e3"] = "Ventilation Shaft",
                ["scav_e4"] = "Sewer Manhole",
                ["scav_e5"] = "Near Kamchatskaya Arch",
                ["scav_e7"] = "Cardinal Apartment Complex Parking",
                ["scav_e8"] = "Klimov Shopping Mall Exfil",
                ["scav_e6"] = "Pinewood Basement (Co-Op)",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "Sandbox", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Sandbox_VExit"] = "Police Cordon V-Ex",
                ["Unity_free_exit"] = "Emercom Checkpoint",
                ["Scav_coop_exit"] = "Scav Checkpoint (Co-Op)",
                ["Nakatani_stairs_free_exit"] = "Nakatani Basement Stairs",
                ["Sniper_exit"] = "Mira Ave",
                ["groundzero_secret_adaptation"] = "Tartowers Sales Office (Secret)",
                // SCAV
                ["Scav Checkpoint (Co-Op)"] = "Scav Checkpoint (Co-Op)",
                ["Emercom Checkpoint"] = "Emercom Checkpoint",
                ["Nakatani Basement Stairs"] = "Nakatani Basement Stairs",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
            { "Sandbox_high", new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                // PMC
                ["Sandbox_VExit"] = "Police Cordon V-Ex",
                ["Unity_free_exit"] = "Emercom Checkpoint",
                ["Scav_coop_exit"] = "Scav Checkpoint (Co-Op)",
                ["Nakatani_stairs_free_exit"] = "Nakatani Basement Stairs",
                ["Sniper_exit"] = "Mira Ave",
                ["groundzero_secret_adaptation"] = "Tartowers Sales Office (Secret)",
                // SCAV
                ["Scav Checkpoint (Co-Op)"] = "Scav Checkpoint (Co-Op)",
                ["Emercom Checkpoint"] = "Emercom Checkpoint",
                ["Nakatani Basement Stairs"] = "Nakatani Basement Stairs",
            }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase) },
        }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase);
    }
}