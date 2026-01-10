using LoneEftDmaRadar.Misc;
using LoneEftDmaRadar.Tarkov.GameWorld;
using LoneEftDmaRadar.Tarkov.GameWorld.Exits;
using LoneEftDmaRadar.Tarkov.GameWorld.Explosives;
using LoneEftDmaRadar.Tarkov.GameWorld.Loot;
using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.GameWorld.Player.Helpers;
using LoneEftDmaRadar.Tarkov.GameWorld.Quests;
using LoneEftDmaRadar.Tarkov.Unity.Structures;
using System.Drawing;
using System.Linq;
using System.Collections.Concurrent;
using System.Windows.Input;
using System.Windows.Threading;
using LoneEftDmaRadar.UI.Skia;
using LoneEftDmaRadar.UI.Misc;
using SharpDX;
using SharpDX.Mathematics.Interop;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Forms.Integration;
using WinForms = System.Windows.Forms;
using SkiaSharp;
using DxColor = SharpDX.Mathematics.Interop.RawColorBGRA;
using LoneEftDmaRadar.Tarkov.GameWorld.Camera;
using LoneEftDmaRadar.UI.Radar;
using LoneEftDmaRadar.UI.Radar.Maps;
using LoneEftDmaRadar.UI.Radar.ViewModels;

namespace LoneEftDmaRadar.UI.ESP
{
    public partial class ESPWindow : Window
    {
        #region Fields/Properties

        private const int MINIRADAR_SIZE = 256;
        private string _lastMapId;
        private MiniRadarParams _miniRadarParams;

        private struct MiniRadarParams
        {
            public float Scale;
            public float OffsetX;
            public float OffsetY;
            public float ScreenX;
            public float ScreenY;
            public float DrawSize;
            public bool IsValid;
            // Self-lock mode: player position in map coords
            public float PlayerWorldX;
            public float PlayerWorldY;
            public bool SelfLockEnabled;
            public float ZoomLevel;
            // Last rendered position (for texture update throttling)
            public float LastRenderedX;
            public float LastRenderedY;
            public float LastRenderedZoom;
            public float LastRenderedHeight; // Player height for SVG layer filtering
            // Actual center of the rendered texture (may differ from player pos when at map edges)
            public float TextureCenterX;
            public float TextureCenterY;
        }

        public static bool ShowESP { get; set; } = true;
        private bool _dxInitFailed;

        private readonly System.Diagnostics.Stopwatch _fpsSw = new();
        private int _fpsCounter;
        private int _fps;
        private long _lastFrameTicks;
        private Timer _highFrequencyTimer;
        private int _renderPending;

        // Render surface
        private ImGuiOverlayControl _dxOverlay;
        private WindowsFormsHost _dxHost;
        private bool _isClosing;
        private DispatcherTimer _resolutionTimer;

        // Cached Fonts/Paints
        private readonly SKPaint _skeletonPaint;
        private readonly SKPaint _boxPaint;
        private readonly SKPaint _crosshairPaint;
        private static readonly SKColor[] _espGroupPalette = new SKColor[]
        {
            SKColors.MediumSlateBlue,
            SKColors.MediumSpringGreen,
            SKColors.CadetBlue,
            SKColors.MediumOrchid,
            SKColors.PaleVioletRed,
            SKColors.SteelBlue,
            SKColors.DarkSeaGreen,
            SKColors.Chocolate
        };
        private static readonly ConcurrentDictionary<int, SKPaint> _espGroupPaints = new();

        private bool _isFullscreen;
        
        // Cached aimline values to prevent flicker during async reads
        private Vector3? _cachedFireportPos;
        private Quaternion? _cachedFireportRot;

        /// <summary>
        /// LocalPlayer (who is running Radar) 'Player' object.
        /// </summary>
        private static LocalPlayer LocalPlayer => Memory.LocalPlayer;

        /// <summary>
        /// All Players in Local Game World (including dead/exfil'd) 'Player' collection.
        /// </summary>
        private static IReadOnlyCollection<AbstractPlayer> AllPlayers => Memory.Players;

        private static IReadOnlyCollection<IExitPoint> Exits => Memory.Exits;

        private static IReadOnlyCollection<IExplosiveItem> Explosives => Memory.Explosives;

        private static bool InRaid => Memory.InRaid;

        // Bone Connections for Skeleton
        private static readonly (Bones From, Bones To)[] _boneConnections = new[]
        {
            (Bones.HumanHead, Bones.HumanNeck),
            (Bones.HumanNeck, Bones.HumanSpine3),
            (Bones.HumanSpine3, Bones.HumanSpine2),
            (Bones.HumanSpine2, Bones.HumanSpine1),
            (Bones.HumanSpine1, Bones.HumanPelvis),
            
            // Left Arm
            (Bones.HumanNeck, Bones.HumanLUpperarm), // Shoulder approx
            (Bones.HumanLUpperarm, Bones.HumanLForearm1),
            (Bones.HumanLForearm1, Bones.HumanLForearm2),
            (Bones.HumanLForearm2, Bones.HumanLPalm),
            
            // Right Arm
            (Bones.HumanNeck, Bones.HumanRUpperarm), // Shoulder approx
            (Bones.HumanRUpperarm, Bones.HumanRForearm1),
            (Bones.HumanRForearm1, Bones.HumanRForearm2),
            (Bones.HumanRForearm2, Bones.HumanRPalm),
            
            // Left Leg
            (Bones.HumanPelvis, Bones.HumanLThigh1),
            (Bones.HumanLThigh1, Bones.HumanLThigh2),
            (Bones.HumanLThigh2, Bones.HumanLCalf),
            (Bones.HumanLCalf, Bones.HumanLFoot),
            
            // Right Leg
            (Bones.HumanPelvis, Bones.HumanRThigh1),
            (Bones.HumanRThigh1, Bones.HumanRThigh2),
            (Bones.HumanRThigh2, Bones.HumanRCalf),
            (Bones.HumanRCalf, Bones.HumanRFoot),
        };

        #endregion

        public ESPWindow()
        {
            InitializeComponent();
            InitializeRenderSurface();
            
            // Initial sizes
            this.Width = 400;
            this.Height = 300;
            this.WindowStartupLocation = WindowStartupLocation.CenterScreen;

            // Cache paints/fonts
            _skeletonPaint = new SKPaint
            {
                Color = SKColors.White,
                StrokeWidth = 1.5f,
                IsAntialias = true,
                Style = SKPaintStyle.Stroke
            };

            _boxPaint = new SKPaint
            {
                Color = SKColors.White,
                StrokeWidth = 1.0f,
                IsAntialias = false, // Crisper boxes
                Style = SKPaintStyle.Stroke
            };

            _crosshairPaint = new SKPaint
            {
                Color = SKColors.White,
                StrokeWidth = 1.5f,
                IsAntialias = true,
                Style = SKPaintStyle.Stroke
            };

            _fpsSw.Start();
            _lastFrameTicks = System.Diagnostics.Stopwatch.GetTimestamp();

            // Use dedicated timer for independent FPS from WPF
            _highFrequencyTimer = new System.Threading.Timer(
                callback: HighFrequencyRenderCallback,
                state: null,
                dueTime: 0,
                period: 1); // 1ms period for max ~1000 FPS capability, actual FPS controlled by EspMaxFPS

            _resolutionTimer = new DispatcherTimer(DispatcherPriority.Background, this.Dispatcher);
            _resolutionTimer.Interval = TimeSpan.FromSeconds(1);
            _resolutionTimer.Tick += (s, e) => ApplyResolutionOverrideIfNeeded();
            _resolutionTimer.Start();
        }

        private void InitializeRenderSurface()
        {
            RenderRoot.Children.Clear();

            _dxOverlay = new ImGuiOverlayControl
            {
                Dock = WinForms.DockStyle.Fill
            };

            ApplyDxFontConfig();
            _dxOverlay.RenderFrame = RenderSurface;
            _dxOverlay.DeviceInitFailed += Overlay_DeviceInitFailed;
            _dxOverlay.MouseDown += GlControl_MouseDown;
            _dxOverlay.DoubleClick += GlControl_DoubleClick;
            _dxOverlay.KeyDown += GlControl_KeyDown;

            _dxHost = new WindowsFormsHost
            {
                Child = _dxOverlay
            };

            RenderRoot.Children.Add(_dxHost);
        }

        private void HighFrequencyRenderCallback(object state)
        {
            try
            {
                if (_isClosing || _dxOverlay == null)
                    return;

                long currentTicks = System.Diagnostics.Stopwatch.GetTimestamp();
                int maxFPS = App.Config.UI.EspMaxFPS;
                
                // Calculate time since last frame
                double elapsedMs = (currentTicks - _lastFrameTicks) * 1000.0 / System.Diagnostics.Stopwatch.Frequency;
                
                // FPS limiting: Check if we've waited long enough
                if (maxFPS > 0)
                {
                    double targetMs = 1000.0 / maxFPS;
                    if (elapsedMs < targetMs)
                    {
                        // Not ready yet - wait for next timer tick
                        return;
                    }
                }

                // Only render if no render is currently pending
                if (System.Threading.Interlocked.CompareExchange(ref _renderPending, 1, 0) == 0)
                {
                    _lastFrameTicks = currentTicks;
                    
                    try
                    {
                        _dxOverlay?.Render();
                    }
                    finally
                    {
                        System.Threading.Interlocked.Exchange(ref _renderPending, 0);
                    }
                }
            }
            catch { /* Ignore errors during shutdown */ }
        }

        #region Rendering Methods

        /// <summary>
        /// Record the Rendering FPS.
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
        private void SetFPS()
        {
            if (_fpsSw.ElapsedMilliseconds >= 1000)
            {
                _fps = System.Threading.Interlocked.Exchange(ref _fpsCounter, 0);
                _fpsSw.Restart();
            }
            else
            {
                _fpsCounter++;
            }
        }

        private bool _lastInRaidState = false;

        /// <summary>
        /// Main ESP Render Event.
        /// </summary>
        private void RenderSurface(ImGuiRenderContext ctx)
        {
            if (_dxInitFailed)
                return;

            float screenWidth = ctx.Width;
            float screenHeight = ctx.Height;

            SetFPS();

            // Clear with black background (transparent for fuser)
            ctx.Clear(new DxColor(0, 0, 0, 255));

                // Detect raid state changes and reset camera/state when leaving raid
                if (_lastInRaidState && !InRaid)
                {
                    CameraManager.Reset();
                    DebugLogger.LogInfo("ESP: Detected raid end - reset all state");
                }
                _lastInRaidState = InRaid;

                if (!InRaid)
                    return;

                var localPlayer = LocalPlayer;
                var allPlayers = AllPlayers;
                
                if (localPlayer is not null && allPlayers is not null)
                {
                    if (!ShowESP)
                    {
                        DrawNotShown(ctx, screenWidth, screenHeight);
                    }
                    else
                    {
                        // ApplyResolutionOverrideIfNeeded(); // performance fix: don't invoke to UI thread every frame

                        // Render Loot (background layer)
                        if (App.Config.Loot.Enabled && App.Config.UI.EspLoot)
                        {
                            DrawLoot(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        if (App.Config.Loot.Enabled && App.Config.UI.EspContainers)
                        {
                            DrawStaticContainers(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        // Render Exfils
                        if (Exits is not null && App.Config.UI.EspExfils)
                        {
                            foreach (var exit in Exits)
                            {
                                if (exit is Exfil exfil && (exfil.Status == Exfil.EStatus.Open || exfil.Status == Exfil.EStatus.Pending))
                                {
                                     if (CameraManager.WorldToScreenWithScale(exfil.Position, out var screen, out float scale, true, true))
                                     {
                                         var dotColor = exfil.Status == Exfil.EStatus.Pending
                                             ? ToColor(SKPaints.PaintExfilPending)
                                             : ToColor(SKPaints.PaintExfilOpen);
                                         var textColor = GetExfilColorForRender();

                                         float radius = Math.Clamp(4f * App.Config.UI.UIScale * scale, 2f, 15f);
                                         ctx.DrawCircle(ToRaw(screen), radius, dotColor, true);

                                         DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                                         ctx.DrawText(exfil.Name, screen.X + radius + 6, screen.Y + 4, textColor, textSize);
                                     }
                                }
                            }
                        }

                        // Render Quest Helper Zones
                        if (App.Config.QuestHelper.Enabled)
                        {
                            DrawQuestHelperZones(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        if (Explosives is not null && App.Config.UI.EspTripwires)
                        {
                            DrawTripwires(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        if (Explosives is not null && App.Config.UI.EspGrenades)
                        {
                            DrawGrenades(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        // Render players
                        foreach (var player in allPlayers)
                        {
                            DrawPlayerESP(ctx, player, localPlayer, screenWidth, screenHeight);
                        }

                        DrawDeviceAimbotTargetLine(ctx, screenWidth, screenHeight);

                        if (App.Config.Device.Enabled)
                        {
                            DrawDeviceAimbotFovCircle(ctx, screenWidth, screenHeight);
                        }

                        if (App.Config.UI.EspCrosshair)
                        {
                            DrawCrosshair(ctx, screenWidth, screenHeight);
                        }

                        DrawDeviceAimbotDebugOverlay(ctx, screenWidth, screenHeight);
                        DrawMemoryInspectorOverlay(ctx, screenWidth, screenHeight);

                        if (App.Config.UI.MiniRadar.Enabled)
                        {
                            DrawMiniRadar(ctx, localPlayer, allPlayers, screenWidth, screenHeight);
                        }

                        if (App.Config.UI.EspLocalAimline)
                        {
                            DrawLocalPlayerAimline(ctx, screenWidth, screenHeight, localPlayer);
                        }
                        
                        if (App.Config.UI.EspLocalAmmo)
                        {
                            DrawLocalPlayerAmmo(ctx, screenWidth, screenHeight, localPlayer);
                        }

                        DrawFPS(ctx, screenWidth, screenHeight);
                    }
                }

        }

        private void DrawLocalPlayerAimline(ImGuiRenderContext ctx, float width, float height, LocalPlayer localPlayer)
        {
            var fm = localPlayer.FirearmManager;
            
            // Update cached values if current values are valid
            if (fm?.FireportPosition is { } newPos && fm.FireportRotation is { } newRot)
            {
                _cachedFireportPos = newPos;
                _cachedFireportRot = newRot;
            }
            
            // Use cached values if available
            if (_cachedFireportPos is not { } start || _cachedFireportRot is not { } rot)
                return; // No cached data available
            
            // Clear cache if weapon is no longer valid
            if (fm?.CurrentHands?.IsWeapon != true)
            {
                _cachedFireportPos = null;
                _cachedFireportRot = null;
                return;
            }
            
            // Use Down (-Y) as the shooting direction for the fireport transform
            var shootDir = rot.Down();
            var end = start + (shootDir * 50f);

            // Try to project the fireport position to screen
            if (!CameraManager.WorldToScreenWithScale(start, out var sStart, out _, true, true))
                return; // Start not visible

            // Try to project the end point
            if (CameraManager.WorldToScreenWithScale(end, out var sEnd, out _, false, false))
            {
                ctx.DrawLine(ToRaw(sStart), ToRaw(sEnd), new DxColor(255, 0, 0, 200), 2f);
            }
            else
            {
                // End point off-screen, extend from start in the direction
                var nearEnd = start + (shootDir * 5f);
                if (CameraManager.WorldToScreenWithScale(nearEnd, out var sNearEnd, out _, false, false))
                {
                    float dx = sNearEnd.X - sStart.X;
                    float dy = sNearEnd.Y - sStart.Y;
                    float len = MathF.Sqrt(dx * dx + dy * dy);
                    if (len > 0.5f)
                    {
                        float scale = 2000f / len;
                        var extendedEnd = new SKPoint(sStart.X + dx * scale, sStart.Y + dy * scale);
                        ctx.DrawLine(ToRaw(sStart), ToRaw(extendedEnd), new DxColor(255, 0, 0, 200), 2f);
                    }
                }
            }
        }

        private void DrawLocalPlayerAmmo(ImGuiRenderContext ctx, float width, float height, LocalPlayer localPlayer)
        {
            var mag = localPlayer.FirearmManager?.Magazine;
            if (mag == null || !mag.IsValid)
                return; // Data not ready yet - skip this frame

            var text = $"{mag.Count}/{mag.MaxCount}";
            if (!string.IsNullOrEmpty(mag.WeaponInfo))
                text += $" [{mag.WeaponInfo}]";

            // Draw near bottom center/right
            float x = width - 200f;
            float y = height - 100f;
            
            ctx.DrawText(text, x, y, new DxColor(0, 255, 255, 255), DxTextSize.Large);
        }

        private void DrawLoot(ImGuiRenderContext ctx, float screenWidth, float screenHeight, LocalPlayer localPlayer)
        {
            var lootItems = Memory.Game?.Loot?.FilteredLoot;
            if (lootItems is null) return;

            var camPos = localPlayer?.Position ?? Vector3.Zero;

            foreach (var item in lootItems)
            {
                // Filter based on ESP settings
                bool isCorpse = item is LootCorpse;
                bool isQuestHelper = item.IsQuestHelperItem; // Use quest helper (active quests) instead of static quest items
                bool isQuest = item.IsQuestItem;
                if (isQuest && !App.Config.UI.EspQuestLoot)
                    continue;
                if (isCorpse && !App.Config.UI.EspCorpses)
                    continue;

                bool isContainer = item is StaticLootContainer;
                if (isContainer)
                    continue;

                bool isFood = item.IsFood;
                bool isMeds = item.IsMeds;
                bool isBackpack = item.IsBackpack;

                // Skip if it's one of these types and the setting is disabled
                if (isFood && !App.Config.UI.EspFood)
                    continue;
                if (isMeds && !App.Config.UI.EspMeds)
                    continue;
                if (isBackpack && !App.Config.UI.EspBackpacks)
                    continue;

                // Check distance to loot
                float distance = Vector3.Distance(camPos, item.Position);
                if (App.Config.UI.EspLootMaxDistance > 0 && distance > App.Config.UI.EspLootMaxDistance)
                    continue;

                if (CameraManager.WorldToScreenWithScale(item.Position, out var screen, out float scale, true, true))
                {
                     // Calculate cone filter based on screen position
                     bool coneEnabled = App.Config.UI.EspLootConeEnabled && App.Config.UI.EspLootConeAngle > 0f;
                     bool inCone = true;

                     if (coneEnabled)
                     {
                         // Calculate angle from screen center
                         float centerX = screenWidth / 2f;
                         float centerY = screenHeight / 2f;
                         float dx = screen.X - centerX;
                         float dy = screen.Y - centerY;

                         // Calculate angular distance from center (in screen space)
                         // Using FOV to convert screen distance to angle
                         float fov = App.Config.UI.FOV;
                         float screenAngleX = MathF.Abs(dx / centerX) * (fov / 2f);
                         float screenAngleY = MathF.Abs(dy / centerY) * (fov / 2f);
                         float screenAngle = MathF.Sqrt(screenAngleX * screenAngleX + screenAngleY * screenAngleY);

                         inCone = screenAngle <= App.Config.UI.EspLootConeAngle;
                     }

                     // Determine colors based on item type (default to user-selected loot color).
                     DxColor circleColor = GetLootColorForRender();
                     DxColor textColor = circleColor;

                     if (isQuest)
                     {
                        circleColor = ToColor(SKPaints.PaintQuestItem);
                        textColor = circleColor;
                     }
                     if (isQuestHelper)
                     {
                         circleColor = ToColor(SKPaints.PaintQuestHelperItem);
                         textColor = circleColor;
                     }
                     else if (item.Important)
                     {
                         circleColor = ToColor(SKPaints.PaintFilteredLoot);
                         textColor = circleColor;
                     }
                     else if (item.IsValuableLoot)
                     {
                         circleColor = ToColor(SKPaints.PaintImportantLoot);
                         textColor = circleColor;
                     }
                     else if (isBackpack)
                     {
                         circleColor = ToColor(SKPaints.PaintBackpacks);
                         textColor = circleColor;
                     }
                     else if (isMeds)
                     {
                         circleColor = ToColor(SKPaints.PaintMeds);
                         textColor = circleColor;
                     }
                     else if (isFood)
                     {
                         circleColor = ToColor(SKPaints.PaintFood);
                         textColor = circleColor;
                     }
                     else if (isCorpse)
                     {
                         circleColor = ToColor(SKPaints.PaintCorpse);
                         textColor = circleColor;
                     }
                     else if (item is LootAirdrop)
                     {
                         circleColor = ToColor(SKPaints.PaintAirdrop);
                         textColor = circleColor;
                     }

                     float radius = Math.Clamp(3f * App.Config.UI.UIScale * scale, 2f, 15f);
                     ctx.DrawCircle(ToRaw(screen), radius, circleColor, true);

                     if (item.Important || inCone)
                     {
                         string text;
                         if (isCorpse && item is LootCorpse corpse)
                         {
                             var corpseName = corpse.Player?.Name;
                             text = string.IsNullOrWhiteSpace(corpseName) ? corpse.Name : corpseName;
                         }
                         else if (item is LootAirdrop)
                         {
                             text = "Airdrop";
                         }
                         else
                         {
                             var shortName = string.IsNullOrWhiteSpace(item.ShortName) ? item.Name : item.ShortName;
                             text = shortName;
                             if (App.Config.UI.EspLootPrice)
                             {
                                 text = item.Important
                                     ? shortName
                                     : $"{shortName} ({LoneEftDmaRadar.Misc.Utilities.FormatNumberKM(item.Price)})";
                             }
                         }
                         DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                         ctx.DrawText(text, screen.X + radius + 4, screen.Y + 4, textColor, textSize);
                    }
                }
            }
        }

        private void DrawQuestHelperZones(ImGuiRenderContext ctx, float screenWidth, float screenHeight, LocalPlayer localPlayer)
        {
            var questLocations = Memory.QuestManager?.LocationConditions?.Values;
            if (questLocations is null) return;

            var camPos = localPlayer?.Position ?? Vector3.Zero;
            var zoneColor = ToColor(SKPaints.PaintQuestZone);
            
            foreach (var loc in questLocations)
            {
                float distance = Vector3.Distance(camPos, loc.Position);
                
                // Skip if too far (use same distance as loot for consistency)
                if (App.Config.UI.EspLootMaxDistance > 0 && distance > App.Config.UI.EspLootMaxDistance)
                    continue;

                if (CameraManager.WorldToScreenWithScale(loc.Position, out var screen, out float scale, true, true))
                {
                    // Draw a distinctive marker for quest zones (diamond shape simulated with rotated square)
                    float size = Math.Clamp(6f * App.Config.UI.UIScale * scale, 3f, 20f);
                    
                    // Draw diamond shape (using lines)
                    var center = ToRaw(screen);
                    var top = new SharpDX.Mathematics.Interop.RawVector2(center.X, center.Y - size);
                    var right = new SharpDX.Mathematics.Interop.RawVector2(center.X + size, center.Y);
                    var bottom = new SharpDX.Mathematics.Interop.RawVector2(center.X, center.Y + size);
                    var left = new SharpDX.Mathematics.Interop.RawVector2(center.X - size, center.Y);
                    
                    ctx.DrawLine(top, right, zoneColor, 2f);
                    ctx.DrawLine(right, bottom, zoneColor, 2f);
                    ctx.DrawLine(bottom, left, zoneColor, 2f);
                    ctx.DrawLine(left, top, zoneColor, 2f);
                    
                    // Draw quest name + type
                    DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                    string label = $"{loc.Name}";
                    if (loc.Type != Tarkov.GameWorld.Quests.QuestObjectiveType.Unknown)
                    {
                        label += $" ({loc.Type})";
                    }
                    ctx.DrawText(label, screen.X + size + 4, screen.Y + 4, zoneColor, textSize);
                }
            }
        }

        private void DrawStaticContainers(ImGuiRenderContext ctx, float screenWidth, float screenHeight, LocalPlayer localPlayer)
        {
            if (!App.Config.Containers.Enabled)
                return;

            var containers = Memory.Game?.Loot?.StaticContainers;
            if (containers is null)
                return;

            bool selectAll = App.Config.Containers.SelectAll;
            var selected = App.Config.Containers.Selected;
            bool hideSearched = App.Config.Containers.HideSearched;
            float maxDistance = App.Config.Containers.EspDrawDistance;
            var color = GetContainerColorForRender();

            foreach (var container in containers)
            {
                var id = container.ID ?? "UNKNOWN";
                if (!(selectAll || selected.ContainsKey(id)))
                    continue;

                if (hideSearched && container.Searched)
                    continue;

                float distance = Vector3.Distance(localPlayer.Position, container.Position);
                if (maxDistance > 0 && distance > maxDistance)
                    continue;

                if (!CameraManager.WorldToScreenWithScale(container.Position, out var screen, out float scale, true, true))
                    continue;

                float radius = Math.Clamp(3f * App.Config.UI.UIScale * scale, 2f, 15f);
                ctx.DrawCircle(ToRaw(screen), radius, color, true);

                DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                ctx.DrawText(container.Name ?? "Container", screen.X + radius + 4, screen.Y + 4, color, textSize);
            }
        }

        private void DrawTripwires(ImGuiRenderContext ctx, float screenWidth, float screenHeight, LocalPlayer localPlayer)
        {
            if (Explosives is null)
                return;

            var camPos = localPlayer?.Position ?? Vector3.Zero;
            float maxDistance = App.Config.UI.EspTripwireMaxDistance;

            foreach (var explosive in Explosives)
            {
                if (explosive is null || explosive is not Tripwire tripwire || !tripwire.IsActive)
                    continue;

                try
                {
                    if (tripwire.Position == Vector3.Zero)
                        continue;

                    // Check distance to tripwire
                    float distance = Vector3.Distance(camPos, tripwire.Position);
                    if (maxDistance > 0 && distance > maxDistance)
                        continue;

                    if (!CameraManager.WorldToScreenWithScale(tripwire.Position, out var screen, out float scale, true, true))
                        continue;

                    var color = GetTripwireColorForRender();
                    float radius = Math.Clamp(5f * App.Config.UI.UIScale * scale, 3f, 20f);
                    ctx.DrawCircle(ToRaw(screen), radius, color, true);

                    DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                    ctx.DrawText("Tripwire", screen.X + radius + 6, screen.Y, color, textSize);
                }
                catch
                {
                    // Silently skip invalid tripwires to prevent ESP from breaking
                    continue;
                }
            }
        }

        private void DrawGrenades(ImGuiRenderContext ctx, float screenWidth, float screenHeight, LocalPlayer localPlayer)
        {
            if (Explosives is null)
                return;

            var camPos = localPlayer?.Position ?? Vector3.Zero;
            float maxDistance = App.Config.UI.EspGrenadeMaxDistance;
            var grenadeColor = GetGrenadeColorForRender();

            foreach (var explosive in Explosives)
            {
                if (explosive is null || explosive is not Grenade grenade)
                    continue;

                try
                {
                    if (grenade.Position == Vector3.Zero)
                        continue;

                    float distance = Vector3.Distance(camPos, grenade.Position);
                    if (maxDistance > 0 && distance > maxDistance)
                        continue;

                    if (!CameraManager.WorldToScreenWithScale(grenade.Position, out var screen, out float scale, true, true))
                        continue;

                    // Draw blast radius circle
                    if (App.Config.UI.EspGrenadeBlastRadius)
                    {
                        float blastRadiusWorld = 5f; // blast radius data should be read from memory or some other sources, for now it's hardcoded
                        const int segments = 32;
                        var blastColor = ToColor(new SKColor(
                            grenadeColor.R, grenadeColor.G, grenadeColor.B, 255));

                        var circlePoints = new List<SKPoint>();
                        for (int i = 0; i <= segments; i++)
                        {
                            float angle = (i / (float)segments) * MathF.PI * 2;
                            var offset = new Vector3(
                                MathF.Cos(angle) * blastRadiusWorld,
                                0,
                                MathF.Sin(angle) * blastRadiusWorld);
                            var worldPos = grenade.Position + offset;

                            if (CameraManager.WorldToScreenWithScale(worldPos, out var screenPos, out _, true, true))
                            {
                                circlePoints.Add(screenPos);
                            }
                        }

                        // Draw line segments between points
                        for (int i = 0; i < circlePoints.Count - 1; i++)
                        {
                            ctx.DrawLine(ToRaw(circlePoints[i]), ToRaw(circlePoints[i + 1]), blastColor, 2f);
                        }
                    }

                    // Draw trail
                    if (App.Config.UI.EspGrenadeTrail && grenade.PositionHistory.Count > 1)
                    {
                        var trailColor = ToColor(new SKColor(
                            grenadeColor.R, grenadeColor.G, grenadeColor.B, 255));

                        var screenPoints = new List<SKPoint>();
                        foreach (var pos in grenade.PositionHistory)
                        {
                            if (pos == Vector3.Zero)
                                continue;
                            if (CameraManager.WorldToScreenWithScale(pos, out var posScreen, out _, true, true))
                            {
                                screenPoints.Add(posScreen);
                            }
                        }

                        // Draw trail segments with increasing thickness
                        for (int i = 0; i < screenPoints.Count - 1; i++)
                        {
                            float progress = (float)i / (screenPoints.Count - 1);
                            float thickness = 0.5f + (progress * 3.5f); // 0.5f to 4f

                            ctx.DrawLine(ToRaw(screenPoints[i]), ToRaw(screenPoints[i + 1]), trailColor, thickness);
                        }
                    }

                    float radius = Math.Clamp(5f * App.Config.UI.UIScale * scale, 3f, 20f);
                    ctx.DrawCircle(ToRaw(screen), radius, grenadeColor, true);

                    DxTextSize textSize = scale > 1.5f ? DxTextSize.Medium : DxTextSize.Small;
                    ctx.DrawText("Grenade", screen.X + radius + 6, screen.Y, grenadeColor, textSize);
                }
                catch
                {
                    // Silently skip invalid grenades to prevent ESP from breaking
                    continue;
                }
            }
        }

        /// <summary>
        /// Renders player on ESP
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
        private void DrawPlayerESP(ImGuiRenderContext ctx, AbstractPlayer player, LocalPlayer localPlayer, float screenWidth, float screenHeight)
        {
            if (player is null || player == localPlayer || !player.IsAlive || !player.IsActive)
                return;

            // Validate player position is valid (not zero or NaN/Infinity)
            var playerPos = player.Position;
            if (playerPos == Vector3.Zero || 
                float.IsNaN(playerPos.X) || float.IsNaN(playerPos.Y) || float.IsNaN(playerPos.Z) ||
                float.IsInfinity(playerPos.X) || float.IsInfinity(playerPos.Y) || float.IsInfinity(playerPos.Z))
                return;

            // Check if this is AI or player
            bool isAI = player.Type is PlayerType.AIScav or PlayerType.AIRaider or PlayerType.AIBoss or PlayerType.AIGuard or PlayerType.PScav;

            // Optimization: Skip players/AI that are too far before W2S
            float distance = Vector3.Distance(localPlayer.Position, player.Position);
            float maxDistance = isAI ? App.Config.UI.EspAIMaxDistance : App.Config.UI.EspPlayerMaxDistance;

            // If maxDistance is 0, it means unlimited, otherwise check distance
            if (maxDistance > 0 && distance > maxDistance)
                return;

            // Fallback to old MaxDistance if the new settings aren't configured
            // if (maxDistance == 0 && distance > App.Config.UI.MaxDistance)
            //    return;

            // Get Color
            var color = GetPlayerColorForRender(player);
            bool isDeviceAimbotLocked = Memory.DeviceAimbot?.LockedTarget == player;
            if (isDeviceAimbotLocked)
            {
                color = ToColor(new SKColor(0, 200, 255, 220));
            }

            bool drawSkeleton = isAI ? App.Config.UI.EspAISkeletons : App.Config.UI.EspPlayerSkeletons;
            bool drawBox = isAI ? App.Config.UI.EspAIBoxes : App.Config.UI.EspPlayerBoxes;
            bool drawName = isAI ? App.Config.UI.EspAINames : App.Config.UI.EspPlayerNames;
            bool drawHealth = isAI ? App.Config.UI.EspAIHealth : App.Config.UI.EspPlayerHealth;
            bool drawDistance = isAI ? App.Config.UI.EspAIDistance : App.Config.UI.EspPlayerDistance;
            bool drawGroupId = isAI ? App.Config.UI.EspAIGroupIds : App.Config.UI.EspGroupIds;
            bool drawWeapon = isAI ? App.Config.UI.EspAIWeapon : App.Config.UI.EspPlayerWeapon;
            bool drawLabel = drawName || drawDistance || drawHealth || drawGroupId || drawWeapon;

            // Draw Skeleton (only if not in error state to avoid frozen bones)
            if (drawSkeleton && !player.IsError)
            {
                DrawSkeleton(ctx, player, screenWidth, screenHeight, color, _skeletonPaint.StrokeWidth);
            }
            
            RectangleF bbox = default;
            bool hasBox = false;
            if (drawBox || drawLabel)
            {
                hasBox = TryGetBoundingBox(player, screenWidth, screenHeight, out bbox);
            }

            // Draw Box
            if (drawBox && hasBox)
            {
                DrawBoundingBox(ctx, bbox, color, _boxPaint.StrokeWidth);
            }

            // Draw head marker
            bool drawHeadCircle = isAI ? App.Config.UI.EspHeadCircleAI : App.Config.UI.EspHeadCirclePlayers;
            if (drawHeadCircle)
            {
                var head = player.GetBonePos(Bones.HumanHead);
                var headTop = head;
                headTop.Y += 0.18f; // small offset to estimate head height

                if (TryProject(head, screenWidth, screenHeight, out var headScreen) &&
                    TryProject(headTop, screenWidth, screenHeight, out var headTopScreen))
                {
                    float radius;
                    if (hasBox)
                    {
                        // scale with on-screen box to stay proportional to the model
                        radius = MathF.Min(bbox.Width, bbox.Height) * 0.1f;
                    }
                    else
                    {
                        // fallback: use projected head height
                        var dy = MathF.Abs(headTopScreen.Y - headScreen.Y);
                        radius = dy * 0.65f;
                    }

                    radius = Math.Clamp(radius, 2f, 12f);
                    ctx.DrawCircle(ToRaw(headScreen), radius, color, filled: false);
                }
            }

            if (drawLabel)
            {
                DrawPlayerLabel(ctx, player, distance, color, hasBox ? bbox : (RectangleF?)null, screenWidth, screenHeight, drawName, drawDistance, drawHealth, drawGroupId, drawWeapon);
            }
        }

        private void DrawMiniRadar(ImGuiRenderContext ctx, LocalPlayer localPlayer, IEnumerable<AbstractPlayer> allPlayers, float screenWidth, float screenHeight)
        {
             try
             {
                 var cfg = App.Config.UI.MiniRadar;

                 if (!cfg.Enabled) return;

                 // Ensure Map Manager is synced with Game Memory
                 var gameMapId = Memory.Game?.MapID;
                 if (!string.IsNullOrEmpty(gameMapId) &&
                     !string.Equals(gameMapId, EftMapManager.Map?.ID, StringComparison.OrdinalIgnoreCase))
                 {
                     DebugLogger.LogInfo($"[MiniRadar] Map Mismatch detected! Game: '{gameMapId}' vs Manager: '{EftMapManager.Map?.ID}'. Loading correct map...");
                     EftMapManager.LoadMap(gameMapId);
                 }

                 var map = EftMapManager.Map;

                 if (map is null) return;

                 // Check if map changed or size changed (null-safe comparison)
                 bool mapChanged = !string.Equals(_lastMapId, map.ID, StringComparison.Ordinal);
                 bool sizeChanged = _lastMiniRadarSize != cfg.Size;
                 
                 // Get current player position for self-lock mode
                 float currentPlayerX = 0, currentPlayerY = 0;
                 if (cfg.SelfLock && localPlayer != null)
                 {
                     var playerMapPos = localPlayer.Position.ToMapPos(map.Config);
                     currentPlayerX = playerMapPos.X;
                     currentPlayerY = playerMapPos.Y;
                 }
                 
                 // Check if we need to update texture in self-lock mode
                 bool selfLockNeedsUpdate = false;
                 bool selfLockModeChanged = cfg.SelfLock != _lastSelfLock;
                 
                 // Get player height for SVG layer filtering
                 float currentPlayerHeight = localPlayer?.Position.Y ?? 0f;
                 
                 // Check if player height changed significantly (for floor/level changes)
                 // Use 2 units threshold to avoid excessive updates from small vertical movements
                 bool heightChanged = MathF.Abs(_miniRadarParams.LastRenderedHeight - currentPlayerHeight) > 2f;
                 
                 if (cfg.SelfLock && localPlayer != null)
                 {
                     // Update if player moved more than 5 map units (balance smoothness vs perf)
                     float dx = currentPlayerX - _miniRadarParams.LastRenderedX;
                     float dy = currentPlayerY - _miniRadarParams.LastRenderedY;
                     float distMoved = MathF.Sqrt(dx * dx + dy * dy);
                     
                     // Also update if zoom changed
                     bool zoomChanged = MathF.Abs(_miniRadarParams.LastRenderedZoom - cfg.ZoomLevel) > 0.01f;
                     
                     selfLockNeedsUpdate = distMoved > 5f || zoomChanged || !_miniRadarParams.IsValid;
                 }
                 
                 // Force update when self-lock mode changes or height changes (for floor/level SVG layers)
                 if (mapChanged || sizeChanged || selfLockNeedsUpdate || selfLockModeChanged || heightChanged)
                 {
                     _lastSelfLock = cfg.SelfLock;
                     
                     if (cfg.SelfLock && localPlayer != null)
                     {
                         UpdateMiniRadarTextureCentered(map, cfg.Size, currentPlayerX, currentPlayerY, cfg.ZoomLevel, currentPlayerHeight);
                     }
                     else
                     {
                         UpdateMiniRadarTexture(map, cfg.Size, currentPlayerHeight);
                     }
                 }

                 if (!_miniRadarParams.IsValid)
                     return;

                 // Define screen position
                 // If ScreenX is < 0, use auto-position (Top Right)
                 float radarScreenX = cfg.ScreenX < 0 
                     ? screenWidth - cfg.Size - 20f 
                     : cfg.ScreenX;
                 
                 float radarScreenY = cfg.ScreenY;
                 
                 // Update internal screen params (Size comes from config now)
                 _miniRadarParams.DrawSize = cfg.Size;
                 _miniRadarParams.ScreenX = radarScreenX;
                 _miniRadarParams.ScreenY = radarScreenY;
                 _miniRadarParams.SelfLockEnabled = cfg.SelfLock;
                 _miniRadarParams.ZoomLevel = cfg.ZoomLevel;
                 
                 // If self-lock is enabled, track local player position for centering
                 if (cfg.SelfLock && localPlayer != null)
                 {
                     _miniRadarParams.PlayerWorldX = currentPlayerX;
                     _miniRadarParams.PlayerWorldY = currentPlayerY;
                 }

                 // Draw Background Texture
                 // Draw Map Texture (No background, no border)
                 var destRect = new RectangleF(radarScreenX, radarScreenY, cfg.Size, cfg.Size);
                 ctx.DrawMapTexture(destRect, 1.0f);

                 // Draw Exits (if enabled)
                 if (Exits is not null && cfg.ShowExits && App.Config.UI.EspExfils)
                 {
                     DrawMiniRadarExits(ctx, map);
                 }

                // Draw Loot (if enabled)
                if (App.Config.Loot.Enabled && cfg.ShowLoot)
                {
                    DrawMiniRadarLoot(ctx, map);
                }
                
                // Draw Containers (if enabled)
                if (App.Config.Loot.Enabled && cfg.ShowContainers)
                {
                    DrawMiniRadarContainers(ctx, map);
                }

                // Draw Explosives (if enabled)
                if (Explosives is not null && cfg.ShowGrenades && (App.Config.UI.EspTripwires || App.Config.UI.EspGrenades))
                {
                    DrawMiniRadarExplosives(ctx, map);
                }

                 // Draw Local Player
                 if (localPlayer is not null)
                 {
                     DrawMiniRadarDot(ctx, localPlayer.Position, localPlayer.Rotation, map, SKColors.Cyan, 4f, true);
                 }

                 // Draw Other Players
                 if (allPlayers != null && cfg.ShowPlayers)
                 {
                     foreach (var player in allPlayers)
                     {
                         if (player == localPlayer || !player.IsAlive || !player.IsActive) continue;
                         
                         var color = GetPlayerColor(player).Color;
                         
                         DrawMiniRadarDot(ctx, player.Position, player.Rotation, map, color, 3f, true);
                     }
                 }
                 
                 // Draw Border Outline
                 ctx.DrawRect(destRect, new DxColor(100, 100, 100, 255), 1.0f);
             }
             catch
             {
                 // Ignore drawing errors
             }
        }

        private void DrawMiniRadarExplosives(ImGuiRenderContext ctx, IEftMap map)
        {
            if (Explosives is null) return;

            bool showTrip = App.Config.UI.EspTripwires;
            bool showNade = App.Config.UI.EspGrenades;
            var tripColor = ColorFromHex(App.Config.UI.EspColorTripwire);
            var nadeColor = ColorFromHex(App.Config.UI.EspColorGrenade);

            foreach (var explosive in Explosives)
            {
                if (explosive is null || explosive.Position == Vector3.Zero) continue;
                
                if (explosive is Tripwire trip && trip.IsActive && showTrip)
                {
                    DrawMiniRadarDot(ctx, trip.Position, map, tripColor, 2f);
                }
                else if (explosive is Grenade && showNade)
                {
                    DrawMiniRadarDot(ctx, explosive.Position, map, nadeColor, 2f);
                }
            }
        }

        private void DrawMiniRadarContainers(ImGuiRenderContext ctx, IEftMap map)
        {
             if (!App.Config.Containers.Enabled) return;
             var containers = Memory.Game?.Loot?.StaticContainers;
             if (containers is null) return;
             
             bool selectAll = App.Config.Containers.SelectAll;
             var selected = App.Config.Containers.Selected;
             bool hideSearched = App.Config.Containers.HideSearched;
             var color = ColorFromHex(App.Config.UI.EspColorContainers);
             
             foreach (var c in containers)
             {
                  if (c.Position == Vector3.Zero) continue;
                  var id = c.ID ?? "UNKNOWN";
                  if (!(selectAll || selected.ContainsKey(id))) continue;
                  if (hideSearched && c.Searched) continue;
                  
                  DrawMiniRadarDot(ctx, c.Position, map, color, 1.5f);
             }
        }

        private void DrawMiniRadarLoot(ImGuiRenderContext ctx, IEftMap map)
        {
            var lootItems = Memory.Game?.Loot?.FilteredLoot;
            if (lootItems is null) return;

            foreach (var item in lootItems)
            {
                // Basic filtering consistent with DrawLoot
                 bool isCorpse = item is LootCorpse;
                 bool isQuest = item.IsQuestItem;
                 bool isQuestHelper = item.IsQuestHelperItem;
                 if (isQuest && !App.Config.UI.EspQuestLoot) continue;
                 if (isCorpse && !App.Config.UI.EspCorpses) continue;

                 var color = isQuest ? SKColors.YellowGreen :
                             isQuestHelper ? SKPaints.PaintQuestHelperItem.Color :
                             isCorpse ? SKColors.Gray :
                             item.Important ? SKColors.Turquoise : SKColors.White;

                 DrawMiniRadarDot(ctx, item.Position, map, color, 1.5f);
            }
        }

        private void DrawMiniRadarExits(ImGuiRenderContext ctx, IEftMap map)
        {
            if (Exits is null) return;

            foreach (var exit in Exits)
            {
                if (exit is Exfil exfil && (exfil.Status == Exfil.EStatus.Open || exfil.Status == Exfil.EStatus.Pending))
                {
                    var color = exfil.Status == Exfil.EStatus.Pending ? SKColors.Yellow : SKColors.LimeGreen;
                    DrawMiniRadarDot(ctx, exfil.Position, map, color, 2.5f);
                }
            }
        }

        private void DrawMiniRadarDot(ImGuiRenderContext ctx, Vector3 worldPos, IEftMap map, SKColor color, float size)
        {
            var unused = Vector2.Zero; // Dummy rotation
            DrawMiniRadarDot(ctx, worldPos, unused, map, color, size, false);
        }

        private void DrawMiniRadarDot(ImGuiRenderContext ctx, Vector3 worldPos, Vector2 rotation, IEftMap map, SKColor color, float size, bool drawLookDir)
        {
             // Transform world pos to map coordinates
             var mapPos = worldPos.ToMapPos(map.Config);
             
             float screenX, screenY;
             
             if (_miniRadarParams.SelfLockEnabled)
             {
                 // Self-lock mode: positions are relative to the TEXTURE center, not player position
                 // This is important when player is at map corners - the texture is clamped to map bounds
                 // so the actual center of what's rendered may differ from the player position
                 float halfSize = _miniRadarParams.DrawSize / 2f;
                 
                 // Calculate relative position from the actual texture center (not player position)
                 float relX = mapPos.X - _miniRadarParams.TextureCenterX;
                 float relY = mapPos.Y - _miniRadarParams.TextureCenterY;
                 
                 // The Scale already includes the zoom factor from UpdateMiniRadarTextureCentered
                 // So we just apply the scale directly
                 screenX = _miniRadarParams.ScreenX + halfSize + (relX * _miniRadarParams.Scale);
                 screenY = _miniRadarParams.ScreenY + halfSize + (relY * _miniRadarParams.Scale);
             }
             else
             {
                 // Standard mode: use pre-calculated params from UpdateMiniRadarTexture
                 float miniX = mapPos.X * _miniRadarParams.Scale + _miniRadarParams.OffsetX;
                 float miniY = mapPos.Y * _miniRadarParams.Scale + _miniRadarParams.OffsetY;
                 
                 screenX = _miniRadarParams.ScreenX + miniX;
                 screenY = _miniRadarParams.ScreenY + miniY;
             }

             // Clip to radar bounds
             if (screenX < _miniRadarParams.ScreenX || screenX > _miniRadarParams.ScreenX + _miniRadarParams.DrawSize ||
                 screenY < _miniRadarParams.ScreenY || screenY > _miniRadarParams.ScreenY + _miniRadarParams.DrawSize)
                 return;

             if (drawLookDir)
             {
                 DrawMiniRadarLookDirection(ctx, screenX, screenY, rotation, color);
             }

             ctx.DrawCircle(new SharpDX.Mathematics.Interop.RawVector2(screenX, screenY), size, ToColor(color), true);
        }

        private void DrawMiniRadarLookDirection(ImGuiRenderContext ctx, float screenX, float screenY, Vector2 rotation, SKColor color)
        {
             float rX = rotation.X; // Yaw
             float rad = (rX - 90) * (MathF.PI / 180f);
             float cos = MathF.Cos(rad);
             float sin = MathF.Sin(rad);
             
             float len = 10f; // Look length
             
             float endX = screenX + cos * len;
             float endY = screenY + sin * len;

             ctx.DrawLine(new RawVector2(screenX, screenY), new RawVector2(endX, endY), ToColor(color), 1f);
        }

        private int _lastMiniRadarSize = -1;
        private bool _lastSelfLock = false;
        private DateTime _lastMiniRadarErrorTime = DateTime.MinValue;

        private void UpdateMiniRadarTexture(IEftMap map, int size, float playerHeight)
        {
             try 
             {
                 // Get Map Bounds
                 var bounds = map.GetBounds();
                 if (bounds.IsEmpty) 
                 {
                     if ((DateTime.Now - _lastMiniRadarErrorTime).TotalSeconds > 5)
                     {
                         DebugLogger.LogDebug($"[MiniRadar] Map bounds empty for '{map.ID}'. Retrying...");
                         _lastMiniRadarErrorTime = DateTime.Now;
                     }
                     return;
                 }

                 // We render to a higher resolution texture to ensure map lines are preserved during scaling
                 // Then we let DX9 overlay handle the downsampling to screen size
                 const int TEXTURE_SIZE = 512; // Moderate res for quality/perf balance
                 
                 float mapW = bounds.Width;
                 float mapH = bounds.Height;
                 
                 if (mapW <= 0 || mapH <= 0) return;

                 // The scale used for RENDERING the map to the texture
                 // We want to fit the map into TEXTURE_SIZE (512)
                 float renderScale = Math.Min((float)TEXTURE_SIZE / mapW, (float)TEXTURE_SIZE / mapH);

                 // Determine offsets to center the map in the TEXTURE
                 float renderOffsetX = (TEXTURE_SIZE - (mapW * renderScale)) / 2f;
                 float renderOffsetY = (TEXTURE_SIZE - (mapH * renderScale)) / 2f;

                 float screenScale = renderScale * ((float)size / TEXTURE_SIZE);
                 float screenOffsetX = renderOffsetX * ((float)size / TEXTURE_SIZE);
                 float screenOffsetY = renderOffsetY * ((float)size / TEXTURE_SIZE);
                 
                 // Render to Bitmap
                 using var bitmap = new SKBitmap(TEXTURE_SIZE, TEXTURE_SIZE, SKColorType.Bgra8888, SKAlphaType.Premul);
                 using var canvas = new SKCanvas(bitmap);
                 
                 // Use Transparent background so the underlying FilledRect color shows through
                 canvas.Clear(SKColors.Transparent);

                 // Render map into the 512x512 canvas with height-based layer filtering
                 try
                 {
                     map.RenderThumbnail(canvas, TEXTURE_SIZE, TEXTURE_SIZE, playerHeight);
                 }
                 catch (Exception ex)
                 {
                     DebugLogger.LogError($"[ESPWindow] Failed to render mini-radar thumbnail: {ex.Message}");
                     return;
                 }
                 
                 // Note: Debug Red X removed to clean up view.

                 var bytes = bitmap.Bytes; 
                 // Request texture update
                 _dxOverlay.RequestMapTextureUpdate(TEXTURE_SIZE, TEXTURE_SIZE, bytes);
                 
                 // Set ID/Params only after success to ensure we retry on failure
                 // This ensures that dots are only drawn if the map is valid and loaded
                 _miniRadarParams = new MiniRadarParams
                 {
                     Scale = screenScale,
                     OffsetX = screenOffsetX,
                     OffsetY = screenOffsetY,
                     DrawSize = size,
                     IsValid = true,
                     LastRenderedHeight = playerHeight
                 };
                 
                 _lastMapId = map.ID;
                 _lastMiniRadarSize = size;
                 DebugLogger.LogInfo($"[MiniRadar] Texture updated for '{map.ID}' @ {size}px (Scale: {screenScale:F3})");
             }
             catch (Exception ex)
             {
                 if ((DateTime.Now - _lastMiniRadarErrorTime).TotalSeconds > 5)
                 {
                     DebugLogger.LogDebug($"[MiniRadar] Update failed: {ex.Message}");
                     _lastMiniRadarErrorTime = DateTime.Now;
                 }
             }
        }

        private void UpdateMiniRadarTextureCentered(IEftMap map, int size, float centerX, float centerY, float zoom, float playerHeight)
        {
             try 
             {
                 // Get Map Bounds
                 var bounds = map.GetBounds();
                 if (bounds.IsEmpty) 
                 {
                     if ((DateTime.Now - _lastMiniRadarErrorTime).TotalSeconds > 5)
                     {
                         DebugLogger.LogDebug($"[MiniRadar] Map bounds empty for '{map.ID}'. Retrying...");
                         _lastMiniRadarErrorTime = DateTime.Now;
                     }
                     return;
                 }

                 const int TEXTURE_SIZE = 512;
                 
                 float mapW = bounds.Width;
                 float mapH = bounds.Height;
                 
                 if (mapW <= 0 || mapH <= 0) return;

                 // Render to Bitmap
                 using var bitmap = new SKBitmap(TEXTURE_SIZE, TEXTURE_SIZE, SKColorType.Bgra8888, SKAlphaType.Premul);
                 using var canvas = new SKCanvas(bitmap);
                 
                 canvas.Clear(SKColors.Transparent);

                 // Render map centered on player with zoom and height-based layer filtering
                 try
                 {
                     map.RenderThumbnailCentered(canvas, TEXTURE_SIZE, TEXTURE_SIZE, centerX, centerY, zoom, playerHeight);
                 }
                 catch (Exception ex)
                 {
                     DebugLogger.LogError($"[ESPWindow] Failed to render centered mini-radar: {ex.Message}");
                     return;
                 }

                 var bytes = bitmap.Bytes; 
                 _dxOverlay.RequestMapTextureUpdate(TEXTURE_SIZE, TEXTURE_SIZE, bytes);
                 
                 // For self-lock mode, scale is based on zoom and the dot positions are calculated differently
                 // The texture shows a zoomed portion, so dots need to be positioned relative to texture center
                 float visibleW = mapW / zoom;
                 float visibleH = mapH / zoom;
                 float renderScale = Math.Min((float)TEXTURE_SIZE / visibleW, (float)TEXTURE_SIZE / visibleH);
                 float screenScale = renderScale * ((float)size / TEXTURE_SIZE);
                 
                 // Calculate the actual texture center after boundary clamping
                 // This must match the logic in RenderThumbnailCentered
                 float srcLeft = centerX - (visibleW / 2f);
                 float srcTop = centerY - (visibleH / 2f);
                 
                 // Clamp to map bounds (same as RenderThumbnailCentered)
                 srcLeft = Math.Max(0, Math.Min(srcLeft, mapW - visibleW));
                 srcTop = Math.Max(0, Math.Min(srcTop, mapH - visibleH));
                 
                 // The actual center of what's being rendered
                 float textureCenterX = srcLeft + (visibleW / 2f);
                 float textureCenterY = srcTop + (visibleH / 2f);
                 
                 _miniRadarParams = new MiniRadarParams
                 {
                     Scale = screenScale,
                     OffsetX = 0, // Not used in self-lock mode
                     OffsetY = 0,
                     DrawSize = size,
                     IsValid = true,
                     PlayerWorldX = centerX,
                     PlayerWorldY = centerY,
                     SelfLockEnabled = true,
                     ZoomLevel = zoom,
                     LastRenderedX = centerX,
                     LastRenderedY = centerY,
                     LastRenderedZoom = zoom,
                     LastRenderedHeight = playerHeight,
                     TextureCenterX = textureCenterX,
                     TextureCenterY = textureCenterY
                 };
                 
                 _lastMapId = map.ID;
                 _lastMiniRadarSize = size;
             }
             catch (Exception ex)
             {
                 if ((DateTime.Now - _lastMiniRadarErrorTime).TotalSeconds > 5)
                 {
                     DebugLogger.LogDebug($"[MiniRadar] Centered update failed: {ex.Message}");
                     _lastMiniRadarErrorTime = DateTime.Now;
                 }
             }
        }

        private void DrawSkeleton(ImGuiRenderContext ctx, AbstractPlayer player, float w, float h, DxColor color, float thickness)
        {
            foreach (var (from, to) in _boneConnections)
            {
                var p1 = player.GetBonePos(from);
                var p2 = player.GetBonePos(to);

                // Skip if either bone position is invalid (zero or NaN)
                if (p1 == Vector3.Zero || p2 == Vector3.Zero)
                    continue;

                if (TryProject(p1, w, h, out var s1) && TryProject(p2, w, h, out var s2))
                {
                    ctx.DrawLine(ToRaw(s1), ToRaw(s2), color, thickness);
                }
            }
        }

            private bool TryGetBoundingBox(AbstractPlayer player, float w, float h, out RectangleF rect)
        {
            rect = default;
            
            // Validate player position before calculating bounding box
            var playerPos = player.Position;
            if (playerPos == Vector3.Zero || 
                float.IsNaN(playerPos.X) || float.IsInfinity(playerPos.X))
                return false;
            
            var projectedPoints = new List<SKPoint>();
            var mins = new Vector3((float)-0.4, 0, (float)-0.4);
            var maxs = new Vector3((float)0.4, (float)1.75, (float)0.4);

            mins = playerPos + mins;
            maxs = playerPos + maxs;

            var points = new List<Vector3> {
                new Vector3(mins.X, mins.Y, mins.Z),
                new Vector3(mins.X, maxs.Y, mins.Z),
                new Vector3(maxs.X, maxs.Y, mins.Z),
                new Vector3(maxs.X, mins.Y, mins.Z),
                new Vector3(maxs.X, maxs.Y, maxs.Z),
                new Vector3(mins.X, maxs.Y, maxs.Z),
                new Vector3(mins.X, mins.Y, maxs.Z),
                new Vector3(maxs.X, mins.Y, maxs.Z)
            };

            foreach (var position in points)
            {
                if (TryProject(position, w, h, out var s))
                    projectedPoints.Add(s);
            }

            if (projectedPoints.Count < 2)
                return false;

            float minX = float.MaxValue, minY = float.MaxValue;
            float maxX = float.MinValue, maxY = float.MinValue;

            foreach (var point in projectedPoints)
            {
                if (point.X < minX) minX = point.X;
                if (point.X > maxX) maxX = point.X;
                if (point.Y < minY) minY = point.Y;
                if (point.Y > maxY) maxY = point.Y;
            }

            float boxWidth = maxX - minX;
            float boxHeight = maxY - minY;

            if (boxWidth < 1f || boxHeight < 1f || boxWidth > w * 2f || boxHeight > h * 2f)
                return false;

            minX = Math.Clamp(minX, -50f, w + 50f);
            maxX = Math.Clamp(maxX, -50f, w + 50f);
            minY = Math.Clamp(minY, -50f, h + 50f);
            maxY = Math.Clamp(maxY, -50f, h + 50f);

            float padding = 2f;
            rect = new RectangleF(minX - padding, minY - padding, (maxX - minX) + padding * 2f, (maxY - minY) + padding * 2f);
            return true;
        }

        private void DrawBoundingBox(ImGuiRenderContext ctx, RectangleF rect, DxColor color, float thickness)
        {
            ctx.DrawRect(rect, color, thickness);
        }

        /// <summary>
        /// Determines player color based on type
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
        private static SKPaint GetPlayerColor(AbstractPlayer player)
        {
            if (player is LocalPlayer)
                return SKPaints.PaintAimviewWidgetLocalPlayer;

            if (player.Type == PlayerType.Teammate)
                return SKPaints.PaintAimviewWidgetTeammate;

            if (player.IsFocused)
                return SKPaints.PaintAimviewWidgetFocused;

            if (player.Type == PlayerType.PMC)
            {
                if (App.Config.UI.EspGroupColors && player.GroupID >= 0 && !(player is LocalPlayer))
                {
                    return _espGroupPaints.GetOrAdd(player.GroupID, id =>
                    {
                        var color = _espGroupPalette[Math.Abs(id) % _espGroupPalette.Length];
                        return new SKPaint
                        {
                            Color = color,
                            StrokeWidth = SKPaints.PaintAimviewWidgetPMC.StrokeWidth,
                            Style = SKPaints.PaintAimviewWidgetPMC.Style,
                            IsAntialias = SKPaints.PaintAimviewWidgetPMC.IsAntialias
                        };
                    });
                }

                if (App.Config.UI.EspFactionColors)
                {
                    if (player.PlayerSide == Enums.EPlayerSide.Bear)
                        return SKPaints.PaintPMCBear;
                    if (player.PlayerSide == Enums.EPlayerSide.Usec)
                        return SKPaints.PaintPMCUsec;
                }

                return SKPaints.PaintPMC;
            }

            return player.Type switch
            {
                PlayerType.AIScav => SKPaints.PaintAimviewWidgetScav,
                PlayerType.AIRaider => SKPaints.PaintAimviewWidgetRaider,
                PlayerType.AIGuard => SKPaints.PaintAimviewWidgetRaider,
                PlayerType.AIBoss => SKPaints.PaintAimviewWidgetBoss,
                PlayerType.PScav => SKPaints.PaintAimviewWidgetPScav,
                _ => SKPaints.PaintAimviewWidgetPMC
            };
        }

        /// <summary>
        /// Draws player label (name/distance) relative to the bounding box or head fallback.
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
        private void DrawPlayerLabel(ImGuiRenderContext ctx, AbstractPlayer player, float distance, DxColor color, RectangleF? bbox, float screenWidth, float screenHeight, bool showName, bool showDistance, bool showHealth, bool showGroup, bool showWeapon)
        {
            if (!showName && !showDistance && !showHealth && !showGroup && !showWeapon)
                return;

            var name = showName ? GetPlayerDisplayName(player) ?? "Unknown" : null;
            var distanceText = showDistance ? $"{distance:F0}m" : null;

            string healthText = null;
            if (showHealth && player is ObservedPlayer observed && observed.HealthStatus is not Enums.ETagStatus.Healthy)
                healthText = observed.HealthStatus.ToString();

            string factionText = null;
            if (App.Config.UI.EspPlayerFaction && player.IsPmc)
                factionText = player.PlayerSide.ToString();

            string groupText = null;
            if (showGroup && player.GroupID != -1 && player.IsPmc && !player.IsAI)
                groupText = $"G:{player.GroupID}";

            string weaponText = null;
            if (showWeapon && player is ObservedPlayer obsWeapon)
            {
                var wName = obsWeapon.Equipment?.InHands?.ShortName;
                if (!string.IsNullOrWhiteSpace(wName))
                    weaponText = wName;
            }

            string primaryText = name; // Name first
            
            if (!string.IsNullOrWhiteSpace(healthText))
                primaryText = string.IsNullOrWhiteSpace(primaryText) ? healthText : $"{primaryText} ({healthText})";
            if (!string.IsNullOrWhiteSpace(distanceText))
                primaryText = string.IsNullOrWhiteSpace(primaryText) ? distanceText : $"{primaryText} ({distanceText})";
            if (!string.IsNullOrWhiteSpace(groupText))
                primaryText = string.IsNullOrWhiteSpace(primaryText) ? groupText : $"{primaryText} [{groupText}]";
            if (!string.IsNullOrWhiteSpace(factionText))
                primaryText = string.IsNullOrWhiteSpace(primaryText) ? factionText : $"{primaryText} [{factionText}]";

            if (string.IsNullOrWhiteSpace(primaryText) && string.IsNullOrWhiteSpace(weaponText))
                return;

            // Measure texts
            var primaryBounds = !string.IsNullOrWhiteSpace(primaryText) 
                ? ctx.MeasureText(primaryText, DxTextSize.Medium) 
                : new SharpDX.Mathematics.Interop.RawRectangle(0,0,0,0);
            
            int primaryHeight = Math.Max(0, primaryBounds.Bottom - primaryBounds.Top);

            int weaponHeight = 0;
            if (!string.IsNullOrWhiteSpace(weaponText))
            {
                var wBounds = ctx.MeasureText(weaponText, DxTextSize.Small);
                weaponHeight = Math.Max(0, wBounds.Bottom - wBounds.Top);
            }

            int totalHeight = primaryHeight + weaponHeight;
            int textPadding = 6;

            float drawX;
            float startY;

            var labelPos = player.IsAI ? App.Config.UI.EspLabelPositionAI : App.Config.UI.EspLabelPosition;

            if (bbox.HasValue)
            {
                var box = bbox.Value;
                drawX = box.Left + (box.Width / 2f);
                startY = labelPos == EspLabelPosition.Top
                    ? box.Top - totalHeight - textPadding
                    : box.Bottom + textPadding;
            }
            else if (TryProject(player.GetBonePos(Bones.HumanHead), screenWidth, screenHeight, out var headScreen))
            {
                drawX = headScreen.X;
                startY = labelPos == EspLabelPosition.Top
                    ? headScreen.Y - totalHeight - textPadding
                    : headScreen.Y + textPadding;
            }
            else
            {
                return;
            }

            // Draw Primary
            if (!string.IsNullOrWhiteSpace(primaryText))
            {
                ctx.DrawText(primaryText, drawX, startY, color, DxTextSize.Medium, centerX: true);
            }

            // Draw Weapon
            if (!string.IsNullOrWhiteSpace(weaponText))
            {
                ctx.DrawText(weaponText, drawX, startY + primaryHeight, color, DxTextSize.Small, centerX: true);
            }
        }

        /// <summary>
        /// Draw 'ESP Hidden' notification.
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
        private void DrawNotShown(ImGuiRenderContext ctx, float width, float height)
        {
            ctx.DrawText("ESP Hidden", width / 2f, height / 2f, new DxColor(255, 255, 255, 255), DxTextSize.Large, centerX: true, centerY: true);
        }

        private void DrawCrosshair(ImGuiRenderContext ctx, float width, float height)
        {
            float centerX = width / 2f;
            float centerY = height / 2f;
            float length = MathF.Max(2f, App.Config.UI.EspCrosshairLength);

            var color = GetCrosshairColor();
            ctx.DrawLine(new RawVector2(centerX - length, centerY), new RawVector2(centerX + length, centerY), color, _crosshairPaint.StrokeWidth);
            ctx.DrawLine(new RawVector2(centerX, centerY - length), new RawVector2(centerX, centerY + length), color, _crosshairPaint.StrokeWidth);
        }

        private void DrawDeviceAimbotTargetLine(ImGuiRenderContext ctx, float width, float height)
        {
            var DeviceAimbot = Memory.DeviceAimbot;
            if (DeviceAimbot?.LockedTarget is not { } target)
                return;

            var headPos = target.GetBonePos(Bones.HumanHead);
            if (!WorldToScreen2(headPos, out var screen, width, height))
                return;

            var center = new RawVector2(width / 2f, height / 2f);
            bool engaged = DeviceAimbot.IsEngaged;
            var skColor = engaged ? new SKColor(0, 200, 255, 200) : new SKColor(255, 210, 0, 180);
            ctx.DrawLine(center, ToRaw(screen), ToColor(skColor), 2f);
        }

        private void DrawDeviceAimbotFovCircle(ImGuiRenderContext ctx, float width, float height)
        {
            var cfg = App.Config.Device;
            if (!cfg.ShowFovCircle || cfg.FOV <= 0)
                return;

            float limit = Math.Min(width, height);
            if (limit < 6f) return;

            float radius = Math.Clamp(cfg.FOV, 5f, limit);
            bool engaged = Memory.DeviceAimbot?.IsEngaged == true;

            // Parse color from config using SKColor.Parse (supports #AARRGGBB and #RRGGBB formats)
            var colorStr = engaged ? cfg.FovCircleColorEngaged : cfg.FovCircleColorIdle;
            var skColor = SKColor.TryParse(colorStr, out var parsed)
                ? parsed
                : new SKColor(255, 255, 255, 180); // Fallback to semi-transparent white

            ctx.DrawCircle(new RawVector2(width / 2f, height / 2f), radius, ToColor(skColor), filled: false);
        }

        private void DrawDeviceAimbotDebugOverlay(ImGuiRenderContext ctx, float width, float height)
        {
            if (!App.Config.Device.ShowDebug)
                return;

            var snapshot = Memory.DeviceAimbot?.GetDebugSnapshot();

            var lines = snapshot == null
                ? new[] { "Device Aimbot: no data" }
                : new[]
                {
                    "=== Device Aimbot ===",
                    $"Status: {snapshot.Status}",
                    $"Key: {(snapshot.KeyEngaged ? "ENGAGED" : "Idle")} | Enabled: {snapshot.Enabled} | Device: {(snapshot.DeviceConnected ? "Connected" : "Disconnected")}",
                    $"InRaid: {snapshot.InRaid} | FOV: {snapshot.ConfigFov:F0}px | MaxDist: {snapshot.ConfigMaxDistance:F0}m | Mode: {snapshot.TargetingMode}",
                    $"Candidates t:{snapshot.CandidateTotal} type:{snapshot.CandidateTypeOk} dist:{snapshot.CandidateInDistance} skel:{snapshot.CandidateWithSkeleton} w2s:{snapshot.CandidateW2S} final:{snapshot.CandidateCount}",
                    $"Target: {(snapshot.LockedTargetName ?? "None")} [{snapshot.LockedTargetType?.ToString() ?? "-"}] valid={snapshot.TargetValid}",
                    snapshot.LockedTargetDistance.HasValue ? $"  Dist {snapshot.LockedTargetDistance.Value:F1}m | FOV { (float.IsNaN(snapshot.LockedTargetFov) ? "n/a" : snapshot.LockedTargetFov.ToString("F1")) } | Bone {snapshot.TargetBone}" : string.Empty,
                    $"Fireport: {(snapshot.HasFireport ? snapshot.FireportPosition?.ToString() : "None")}",
                    $"Ballistics: {(snapshot.BallisticsValid ? $"OK (Speed {(snapshot.BulletSpeed.HasValue ? snapshot.BulletSpeed.Value.ToString("F1") : "?")} m/s, Predict {(snapshot.PredictionEnabled ? "ON" : "OFF")})" : "Invalid/None")}"
                }.Where(l => !string.IsNullOrEmpty(l)).ToArray();

            float x = 10f;
            float y = 40f;
            float lineStep = 16f;
            var color = ToColor(SKColors.White);

            foreach (var line in lines)
            {
                ctx.DrawText(line, x, y, color, DxTextSize.Small);
                y += lineStep;
            }
        }

        /// <summary>
        /// Draw Memory Inspector values on ESP overlay.
        /// </summary>
        private void DrawMemoryInspectorOverlay(ImGuiRenderContext ctx, float width, float height)
        {
            var data = MemoryInspectorViewModel.EspOverlayData;
            if (data == null || !data.Enabled || data.Fields.Count == 0)
                return;

            // Position on right side of screen
            float x = width - 320f;
            float y = 40f;
            float lineStep = 14f;
            var headerColor = ToColor(new SKColor(0, 255, 200, 255)); // Cyan
            var fieldColor = ToColor(new SKColor(200, 200, 200, 255)); // Gray
            var valueColor = ToColor(new SKColor(100, 255, 100, 255)); // Green

            // Draw header
            ctx.DrawText($"=== {data.StructName} ===", x, y, headerColor, DxTextSize.Small);
            y += lineStep;
            ctx.DrawText($"Base: 0x{data.BaseAddress:X}", x, y, fieldColor, DxTextSize.Small);
            y += lineStep + 4;

            // Draw fields (limited to prevent too many lines)
            int maxFields = Math.Min(data.Fields.Count, 25);
            for (int i = 0; i < maxFields; i++)
            {
                var field = data.Fields[i];
                var offsetText = $"0x{field.Offset:X2}";
                var line = $"{field.Name}: {field.Value}";
                
                // Truncate long values
                if (line.Length > 45)
                    line = line.Substring(0, 42) + "...";

                ctx.DrawText(line, x, y, valueColor, DxTextSize.Small);
                y += lineStep;
            }

            if (data.Fields.Count > maxFields)
            {
                ctx.DrawText($"... +{data.Fields.Count - maxFields} more", x, y, fieldColor, DxTextSize.Small);
            }
        }

        private void DrawFPS(ImGuiRenderContext ctx, float width, float height)
        {
            var fpsText = $"FPS: {_fps}";
            ctx.DrawText(fpsText, 10, 10, new DxColor(255, 255, 255, 255), DxTextSize.Small);
        }

        private static RawVector2 ToRaw(SKPoint point) => new(point.X, point.Y);

        private static DxColor ToColor(SKPaint paint) => ToColor(paint.Color);

        private static DxColor ToColor(SKColor color) => new(color.Blue, color.Green, color.Red, color.Alpha);

        #endregion

        private DxColor GetPlayerColorForRender(AbstractPlayer player)
        {
            var cfg = App.Config.UI;
            var basePaint = GetPlayerColor(player);

            // Preserve special colouring (local, focused, teammates).
            if (player is LocalPlayer || player.IsFocused || player.Type is PlayerType.Teammate)
            {
                return ToColor(basePaint);
            }

            // Respect group/faction colours when enabled.
            if (!player.IsAI)
            {
                if (cfg.EspGroupColors && player.GroupID >= 0)
                    return ToColor(basePaint);
                if (cfg.EspFactionColors && player.IsPmc)
                {
                    var factionColor = player.PlayerSide switch
                    {
                        Enums.EPlayerSide.Bear => ColorFromHex(cfg.EspColorFactionBear),
                        Enums.EPlayerSide.Usec => ColorFromHex(cfg.EspColorFactionUsec),
                        _ => ColorFromHex(cfg.EspColorPlayers)
                    };
                    return ToColor(factionColor);
                }
            }

            if (player.IsAI)
            {
                var aiHex = player.Type switch
                {
                    PlayerType.AIBoss => cfg.EspColorBosses,
                    PlayerType.AIRaider => cfg.EspColorRaiders,
                    PlayerType.AIGuard => cfg.EspColorRaiders,
                    _ => cfg.EspColorAI
                };

                return ToColor(ColorFromHex(aiHex));
            }

            // Handle Player Scavs specifically.
            if (player.Type == PlayerType.PScav)
            {
                return ToColor(ColorFromHex(cfg.EspColorPlayerScavs));
            }

            // Fallback to user-configured player colours.
            return ToColor(ColorFromHex(cfg.EspColorPlayers));
        }

        private DxColor GetLootColorForRender() => ToColor(ColorFromHex(App.Config.UI.EspColorLoot));
        private DxColor GetExfilColorForRender() => ToColor(ColorFromHex(App.Config.UI.EspColorExfil));
        private DxColor GetTripwireColorForRender() => ToColor(ColorFromHex(App.Config.UI.EspColorTripwire));
        private DxColor GetGrenadeColorForRender() => ToColor(ColorFromHex(App.Config.UI.EspColorGrenade));
        private DxColor GetContainerColorForRender() => ToColor(ColorFromHex(App.Config.UI.EspColorContainers));
        private DxColor GetCrosshairColor() => ToColor(ColorFromHex(App.Config.UI.EspColorCrosshair));

        private static SKColor ColorFromHex(string hex)
        {
            if (string.IsNullOrWhiteSpace(hex))
                return SKColors.White;
            try { return SKColor.Parse(hex); }
            catch { return SKColors.White; }
        }

        private void ApplyDxFontConfig()
        {
            var ui = App.Config.UI;
            _dxOverlay?.SetFontConfig(
                ui.EspFontFamily,
                ui.EspFontWeight,
                ui.EspFontSizeSmall,
                ui.EspFontSizeMedium,
                ui.EspFontSizeLarge);
        }

        #region DX Init Handling

        private void Overlay_DeviceInitFailed(Exception ex)
        {
            _dxInitFailed = true;
            DebugLogger.LogDebug($"ESP DX init failed: {ex}");

            Dispatcher.BeginInvoke(new Action(() =>
            {
                RenderRoot.Children.Clear();
                RenderRoot.Children.Add(new TextBlock
                {
                    Text = "DX overlay init failed. See log for details.",
                    Foreground = System.Windows.Media.Brushes.White,
                    Background = System.Windows.Media.Brushes.Black,
                    Margin = new Thickness(12)
                });
            }), DispatcherPriority.Send);
        }

        #endregion

        #region WorldToScreen Conversion

        /// <summary>
        /// Resets ESP state when a raid ends (ensures clean slate next raid).
        /// </summary>
        public void OnRaidStopped()
        {
            _lastInRaidState = false;
            _espGroupPaints.Clear();
            _lastMapId = null; // Reset map ID to force update next raid
            _miniRadarParams = default; // Clear render params
            CameraManager.Reset();
            RefreshESP();
            DebugLogger.LogInfo("ESP: RaidStopped -> state reset");
        }

        /// <summary>
        /// Gets the display name for a player, checking whitelist for custom names first.
        /// </summary>
        /// <param name="player">The player to get the display name for</param>
        /// <returns>Custom name from whitelist if available, otherwise the player's original name</returns>
        private static string GetPlayerDisplayName(AbstractPlayer player)
        {
            return player?.Name;
        }

        private bool WorldToScreen2(in Vector3 world, out SKPoint scr, float screenWidth, float screenHeight)
        {
            return CameraManager.WorldToScreen(in world, out scr, true, true);
        }

        private bool TryProject(in Vector3 world, float w, float h, out SKPoint screen)
        {
            screen = default;
            if (world == Vector3.Zero)
                return false;
            if (!WorldToScreen2(world, out screen, w, h))
                return false;
            if (float.IsNaN(screen.X) || float.IsInfinity(screen.X) ||
                float.IsNaN(screen.Y) || float.IsInfinity(screen.Y))
                return false;

            const float margin = 200f; 
            if (screen.X < -margin || screen.X > w + margin ||
                screen.Y < -margin || screen.Y > h + margin)
                return false;

            return true;
        }

        #endregion

        #region Window Management

        private void GlControl_MouseDown(object sender, WinForms.MouseEventArgs e)
        {
            if (e.Button == WinForms.MouseButtons.Left)
            {
                try { this.DragMove(); } catch { /* ignore dragging errors */ }
            }
        }

        private void GlControl_DoubleClick(object sender, EventArgs e)
        {
            ToggleFullscreen();
        }

        private void GlControl_KeyDown(object sender, WinForms.KeyEventArgs e)
        {
            if (e.KeyCode == WinForms.Keys.F12)
            {
                ForceReleaseCursorAndHide();
                return;
            }

            if (e.KeyCode == WinForms.Keys.Escape && this.WindowState == WindowState.Maximized)
            {
                ToggleFullscreen();
            }
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            // Allow dragging the window
            if (e.ChangedButton == MouseButton.Left)
                this.DragMove();
        }

        protected override void OnClosed(System.EventArgs e)
        {
            _isClosing = true;
            try
            {
                _highFrequencyTimer?.Dispose();
                _dxOverlay?.Dispose();
                _skeletonPaint.Dispose();
                _boxPaint.Dispose();
                _crosshairPaint.Dispose();
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"ESP: OnClosed cleanup error: {ex}");
            }
            finally
            {
                base.OnClosed(e);
            }
        }

        // Method to force refresh
        public void RefreshESP()
        {
            if (_isClosing)
                return;

            try
            {
                _dxOverlay?.Render();
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"ESP Refresh error: {ex}");
            }
            finally
            {
                System.Threading.Interlocked.Exchange(ref _renderPending, 0);
            }
        }

        private void Window_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            ToggleFullscreen();
        }

        // Handler for keys (ESC to exit fullscreen)
        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.F12)
            {
                ForceReleaseCursorAndHide();
                return;
            }

            if (e.Key == Key.Escape && this.WindowState == WindowState.Maximized)
            {
                ToggleFullscreen();
            }
        }

        // Simple fullscreen toggle
        public void ToggleFullscreen()
        {
            if (_isFullscreen)
            {
                this.WindowState = WindowState.Normal;
                this.WindowStyle = WindowStyle.SingleBorderWindow;
                this.Topmost = false;
                this.ResizeMode = ResizeMode.CanResize;
                this.Width = 400;
                this.Height = 300;
                this.WindowStartupLocation = WindowStartupLocation.CenterScreen;
                _isFullscreen = false;
            }
            else
            {
                this.WindowStyle = WindowStyle.None;
                this.ResizeMode = ResizeMode.NoResize;
                this.Topmost = true;
                this.WindowState = WindowState.Normal;

                var monitor = GetTargetMonitor();
                var (width, height) = GetConfiguredResolution(monitor);

                this.Left = monitor?.Left ?? 0;
                this.Top = monitor?.Top ?? 0;

                this.Width = width;
                this.Height = height;
                _isFullscreen = true;
            }

            this.RefreshESP();
        }

        public void ApplyResolutionOverride()
        {
            if (!_isFullscreen)
                return;

            var monitor = GetTargetMonitor();
            var (width, height) = GetConfiguredResolution(monitor);
            this.Left = monitor?.Left ?? 0;
            this.Top = monitor?.Top ?? 0;
            this.Width = width;
            this.Height = height;
            this.RefreshESP();
        }

        private (double width, double height) GetConfiguredResolution(MonitorInfo monitor)
        {
            double width = App.Config.UI.EspScreenWidth > 0
                ? App.Config.UI.EspScreenWidth
                : monitor?.Width ?? SystemParameters.PrimaryScreenWidth;
            double height = App.Config.UI.EspScreenHeight > 0
                ? App.Config.UI.EspScreenHeight
                : monitor?.Height ?? SystemParameters.PrimaryScreenHeight;
            return (width, height);
        }

        private void ApplyResolutionOverrideIfNeeded()
        {
            if (!_isFullscreen)
                return;

            if (App.Config.UI.EspScreenWidth <= 0 && App.Config.UI.EspScreenHeight <= 0)
                return;

            double currentWidth, currentHeight;
            if (Dispatcher.CheckAccess())
            {
                currentWidth = Width;
                currentHeight = Height;
            }
            else
            {
                Dispatcher.Invoke(() =>
                {
                    currentWidth = Width;
                    currentHeight = Height;
                });
                return;
            }

            var monitor = GetTargetMonitor();
            var target = GetConfiguredResolution(monitor);
            if (Math.Abs(currentWidth - target.width) > 0.5 || Math.Abs(currentHeight - target.height) > 0.5)
            {
                if (Dispatcher.CheckAccess())
                {
                    Width = target.width;
                    Height = target.height;
                    Left = monitor?.Left ?? 0;
                    Top = monitor?.Top ?? 0;
                }
                else
                {
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        Width = target.width;
                        Height = target.height;
                        Left = monitor?.Left ?? 0;
                        Top = monitor?.Top ?? 0;
                    }));
                }
            }
        }

        private MonitorInfo GetTargetMonitor()
        {
            return MonitorInfo.GetMonitor(App.Config.UI.EspTargetScreen);
        }

        public void ApplyFontConfig()
        {
            ApplyDxFontConfig();
            RefreshESP();
        }

        /// <summary>
        /// Emergency escape hatch if the overlay ever captures the cursor:
        /// releases capture, resets cursors, hides the ESP, and drops Topmost.
        /// Bound to F12 on both WPF and WinForms handlers.
        /// </summary>
        private void ForceReleaseCursorAndHide()
        {
            try
            {
                Mouse.Capture(null);
                WinForms.Cursor.Current = WinForms.Cursors.Default;
                this.Cursor = System.Windows.Input.Cursors.Arrow;
                Mouse.OverrideCursor = null;
                if (_dxOverlay != null)
                {
                    _dxOverlay.Capture = false;
                }
                this.Topmost = false;
                ShowESP = false;
                Hide();
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"ESP: ForceReleaseCursor failed: {ex}");
            }
        }

        #endregion
    }
}
