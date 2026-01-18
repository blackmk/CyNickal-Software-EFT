/*
 * ESP Preview Control
 * Displays a live preview of ESP settings for PMC/AI entities
 */

using LoneEftDmaRadar.UI.Misc;
using SkiaSharp;
using SkiaSharp.Views.WPF;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

namespace LoneEftDmaRadar.UI.Radar.Views
{
    /// <summary>
    /// Preview mode for the ESP preview control.
    /// </summary>
    public enum EspPreviewMode
    {
        Player,
        AI
    }

    /// <summary>
    /// A SkiaSharp-based preview control that renders a mock PMC/AI entity
    /// with current ESP settings applied.
    /// </summary>
    public class EspPreviewControl : SKElement
    {
        private readonly DispatcherTimer _refreshTimer;
        
        /// <summary>
        /// Dependency property for the preview mode (Player or AI).
        /// </summary>
        public static readonly DependencyProperty PreviewModeProperty =
            DependencyProperty.Register(
                nameof(PreviewMode),
                typeof(EspPreviewMode),
                typeof(EspPreviewControl),
                new PropertyMetadata(EspPreviewMode.Player));

        /// <summary>
        /// Gets or sets the preview mode (Player or AI).
        /// </summary>
        public EspPreviewMode PreviewMode
        {
            get => (EspPreviewMode)GetValue(PreviewModeProperty);
            set => SetValue(PreviewModeProperty, value);
        }
        
        // Mock skeleton bone positions (normalized 0-1 space, will be scaled to canvas)
        private static readonly Dictionary<string, SKPoint> _mockBones = new()
        {
            ["Head"] = new SKPoint(0.5f, 0.12f),
            ["Neck"] = new SKPoint(0.5f, 0.18f),
            ["Spine3"] = new SKPoint(0.5f, 0.28f),
            ["Spine2"] = new SKPoint(0.5f, 0.35f),
            ["Spine1"] = new SKPoint(0.5f, 0.42f),
            ["Pelvis"] = new SKPoint(0.5f, 0.50f),
            ["LeftShoulder"] = new SKPoint(0.38f, 0.22f),
            ["LeftElbow"] = new SKPoint(0.28f, 0.32f),
            ["LeftHand"] = new SKPoint(0.22f, 0.42f),
            ["RightShoulder"] = new SKPoint(0.62f, 0.22f),
            ["RightElbow"] = new SKPoint(0.72f, 0.32f),
            ["RightHand"] = new SKPoint(0.78f, 0.42f),
            ["LeftThigh"] = new SKPoint(0.42f, 0.55f),
            ["LeftKnee"] = new SKPoint(0.40f, 0.70f),
            ["LeftFoot"] = new SKPoint(0.38f, 0.88f),
            ["RightThigh"] = new SKPoint(0.58f, 0.55f),
            ["RightKnee"] = new SKPoint(0.60f, 0.70f),
            ["RightFoot"] = new SKPoint(0.62f, 0.88f),
        };

        // Bone connections for skeleton drawing
        private static readonly (string, string)[] _boneConnections = new[]
        {
            ("Head", "Neck"),
            ("Neck", "Spine3"),
            ("Spine3", "Spine2"),
            ("Spine2", "Spine1"),
            ("Spine1", "Pelvis"),
            ("Neck", "LeftShoulder"),
            ("LeftShoulder", "LeftElbow"),
            ("LeftElbow", "LeftHand"),
            ("Neck", "RightShoulder"),
            ("RightShoulder", "RightElbow"),
            ("RightElbow", "RightHand"),
            ("Pelvis", "LeftThigh"),
            ("LeftThigh", "LeftKnee"),
            ("LeftKnee", "LeftFoot"),
            ("Pelvis", "RightThigh"),
            ("RightThigh", "RightKnee"),
            ("RightKnee", "RightFoot"),
        };

        public EspPreviewControl()
        {
            // Set up a timer to refresh the preview periodically
            _refreshTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(100) // 10 FPS for preview
            };
            _refreshTimer.Tick += (s, e) => InvalidateVisual();
            
            this.Loaded += OnLoaded;
            this.Unloaded += OnUnloaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            _refreshTimer.Start();
        }

        private void OnUnloaded(object sender, RoutedEventArgs e)
        {
            _refreshTimer.Stop();
        }

        protected override void OnPaintSurface(SKPaintSurfaceEventArgs e)
        {
            base.OnPaintSurface(e);
            
            var canvas = e.Surface.Canvas;
            var info = e.Info;
            
            // Clear with dark background
            canvas.Clear(new SKColor(0x0F, 0x11, 0x15));
            
            // Get config
            var config = App.Config.UI;
            bool isAI = PreviewMode == EspPreviewMode.AI;
            
            // Calculate entity area (centered, with padding)
            float padding = 30f;
            float entityWidth = info.Width - padding * 2;
            float entityHeight = info.Height - padding * 2;
            float entityLeft = padding;
            float entityTop = padding;
            
            // Scale bones to canvas
            var scaledBones = new Dictionary<string, SKPoint>();
            foreach (var bone in _mockBones)
            {
                scaledBones[bone.Key] = new SKPoint(
                    entityLeft + bone.Value.X * entityWidth,
                    entityTop + bone.Value.Y * entityHeight
                );
            }
            
            // Get entity color based on mode
            var entityColor = isAI 
                ? ParseColor(config.EspColorAI) 
                : ParseColor(config.EspColorPlayers);
            
            // Get settings based on mode
            bool drawSkeleton = isAI ? config.EspAISkeletons : config.EspPlayerSkeletons;
            bool drawBox = isAI ? config.EspAIBoxes : config.EspPlayerBoxes;
            bool drawHeadCircle = isAI ? config.EspHeadCircleAI : config.EspHeadCirclePlayers;
            
            // Draw skeleton if enabled
            if (drawSkeleton)
            {
                DrawSkeleton(canvas, scaledBones, entityColor);
            }
            
            // Calculate bounding box
            float minX = float.MaxValue, minY = float.MaxValue;
            float maxX = float.MinValue, maxY = float.MinValue;
            foreach (var bone in scaledBones.Values)
            {
                minX = Math.Min(minX, bone.X);
                minY = Math.Min(minY, bone.Y);
                maxX = Math.Max(maxX, bone.X);
                maxY = Math.Max(maxY, bone.Y);
            }
            
            // Add some padding to bbox
            float bboxPadding = 8f;
            var bbox = new SKRect(minX - bboxPadding, minY - bboxPadding, maxX + bboxPadding, maxY + bboxPadding);
            
            // Draw bounding box if enabled
            if (drawBox)
            {
                DrawBoundingBox(canvas, bbox, entityColor);
            }
            
            // Draw head circle if enabled
            if (drawHeadCircle)
            {
                DrawHeadCircle(canvas, scaledBones["Head"], bbox, entityColor);
            }
            
            // Draw label
            DrawLabel(canvas, bbox, entityColor, info.Width, config, isAI);
            
            // Draw watermark
            using var watermarkPaint = new SKPaint
            {
                Color = new SKColor(0x40, 0x40, 0x40),
                TextSize = 12,
                IsAntialias = true,
                Typeface = SKTypeface.FromFamilyName("Segoe UI")
            };
        }

        private void DrawSkeleton(SKCanvas canvas, Dictionary<string, SKPoint> bones, SKColor color)
        {
            using var paint = new SKPaint
            {
                Color = color,
                StrokeWidth = 2f,
                IsAntialias = true,
                Style = SKPaintStyle.Stroke,
                StrokeCap = SKStrokeCap.Round
            };

            foreach (var (from, to) in _boneConnections)
            {
                if (bones.TryGetValue(from, out var p1) && bones.TryGetValue(to, out var p2))
                {
                    canvas.DrawLine(p1, p2, paint);
                }
            }
        }

        private void DrawBoundingBox(SKCanvas canvas, SKRect bbox, SKColor color)
        {
            using var paint = new SKPaint
            {
                Color = color,
                StrokeWidth = 1.5f,
                IsAntialias = true,
                Style = SKPaintStyle.Stroke
            };
            
            canvas.DrawRect(bbox, paint);
        }

        private void DrawHeadCircle(SKCanvas canvas, SKPoint headPos, SKRect bbox, SKColor color)
        {
            float radius = Math.Min(bbox.Width, bbox.Height) * 0.08f;
            radius = Math.Clamp(radius, 4f, 16f);
            
            using var paint = new SKPaint
            {
                Color = color,
                StrokeWidth = 1.5f,
                IsAntialias = true,
                Style = SKPaintStyle.Stroke
            };
            
            canvas.DrawCircle(headPos, radius, paint);
        }

        private void DrawLabel(SKCanvas canvas, SKRect bbox, SKColor color, float canvasWidth, UIConfig config, bool isAI)
        {
            var parts = new List<string>();
            
            // Get settings based on mode
            bool showName = isAI ? config.EspAINames : config.EspPlayerNames;
            bool showHealth = isAI ? config.EspAIHealth : config.EspPlayerHealth;
            bool showDistance = isAI ? config.EspAIDistance : config.EspPlayerDistance;
            bool showWeapon = isAI ? config.EspAIWeapon : config.EspPlayerWeapon;
            
            if (showName)
                parts.Add(isAI ? "Scav" : "PMC_Player");
            
            if (showHealth)
                parts.Add("(Injured)");
            
            if (showDistance)
                parts.Add("(42m)");
            
            // Player-only options
            if (!isAI)
            {
                if (config.EspGroupIds)
                    parts.Add("[G:1]");
                
                if (config.EspPlayerFaction)
                    parts.Add("[USEC]");
            }
            
            string primaryText = string.Join(" ", parts);
            
            string weaponText = showWeapon ? (isAI ? "TOZ-106" : "AK-74M") : null;
            
            if (string.IsNullOrEmpty(primaryText) && string.IsNullOrEmpty(weaponText))
                return;
            
            // Get font settings from config
            string fontFamily = config.EspFontFamily ?? "Segoe UI";
            int fontSizeMedium = Math.Max(8, config.EspFontSizeMedium);
            int fontSizeSmall = Math.Max(6, config.EspFontSizeSmall);
            var fontWeight = GetFontWeight(config.EspFontWeight);
            
            using var primaryPaint = new SKPaint
            {
                Color = color,
                TextSize = fontSizeMedium,
                IsAntialias = true,
                Typeface = SKTypeface.FromFamilyName(fontFamily, fontWeight, SKFontStyleWidth.Normal, SKFontStyleSlant.Upright),
                TextAlign = SKTextAlign.Center
            };
            
            using var weaponPaint = new SKPaint
            {
                Color = color,
                TextSize = fontSizeSmall,
                IsAntialias = true,
                Typeface = SKTypeface.FromFamilyName(fontFamily, fontWeight, SKFontStyleWidth.Normal, SKFontStyleSlant.Upright),
                TextAlign = SKTextAlign.Center
            };
            
            float drawX = bbox.MidX;
            float startY;
            
            var labelPos = isAI ? config.EspLabelPositionAI : config.EspLabelPosition;
            int totalHeight = 0;
            
            if (!string.IsNullOrEmpty(primaryText))
                totalHeight += fontSizeMedium + 2;
            if (!string.IsNullOrEmpty(weaponText))
                totalHeight += fontSizeSmall + 2;
            
            if (labelPos == EspLabelPosition.Top)
            {
                startY = bbox.Top - totalHeight - 4;
            }
            else
            {
                startY = bbox.Bottom + fontSizeMedium + 4;
            }
            
            if (!string.IsNullOrEmpty(primaryText))
            {
                canvas.DrawText(primaryText, drawX, startY, primaryPaint);
                startY += fontSizeMedium + 2;
            }
            
            if (!string.IsNullOrEmpty(weaponText))
            {
                canvas.DrawText(weaponText, drawX, startY, weaponPaint);
            }
        }

        private static SKColor ParseColor(string hexColor)
        {
            if (string.IsNullOrEmpty(hexColor))
                return SKColors.White;
            
            try
            {
                // Remove # if present
                hexColor = hexColor.TrimStart('#');
                
                if (hexColor.Length == 6)
                {
                    // RGB format
                    byte r = Convert.ToByte(hexColor.Substring(0, 2), 16);
                    byte g = Convert.ToByte(hexColor.Substring(2, 2), 16);
                    byte b = Convert.ToByte(hexColor.Substring(4, 2), 16);
                    return new SKColor(r, g, b);
                }
                else if (hexColor.Length == 8)
                {
                    // ARGB format
                    byte a = Convert.ToByte(hexColor.Substring(0, 2), 16);
                    byte r = Convert.ToByte(hexColor.Substring(2, 2), 16);
                    byte g = Convert.ToByte(hexColor.Substring(4, 2), 16);
                    byte b = Convert.ToByte(hexColor.Substring(6, 2), 16);
                    return new SKColor(r, g, b, a);
                }
            }
            catch
            {
                // Ignore parse errors
            }
            
            return SKColors.White;
        }

        /// <summary>
        /// Converts a font weight string (e.g. "Regular", "Bold") to SKFontStyleWeight.
        /// </summary>
        private static SKFontStyleWeight GetFontWeight(string weight)
        {
            if (string.IsNullOrEmpty(weight))
                return SKFontStyleWeight.Normal;

            return weight.ToLowerInvariant() switch
            {
                "thin" => SKFontStyleWeight.Thin,
                "extralight" or "extra light" => SKFontStyleWeight.ExtraLight,
                "light" => SKFontStyleWeight.Light,
                "regular" or "normal" or "default" => SKFontStyleWeight.Normal,
                "medium" => SKFontStyleWeight.Medium,
                "semibold" or "semi bold" or "demibold" => SKFontStyleWeight.SemiBold,
                "bold" => SKFontStyleWeight.Bold,
                "extrabold" or "extra bold" => SKFontStyleWeight.ExtraBold,
                "black" or "heavy" => SKFontStyleWeight.Black,
                _ => SKFontStyleWeight.Normal
            };
        }
    }
}
