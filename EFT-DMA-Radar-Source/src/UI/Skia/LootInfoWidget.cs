using Collections.Pooled;
using LoneEftDmaRadar.Misc;
using LoneEftDmaRadar.Tarkov.GameWorld.Loot;
using SkiaSharp;
using SkiaSharp.Views.WPF;

namespace LoneEftDmaRadar.UI.Skia
{
    public sealed class LootInfoWidget : AbstractSKWidget
    {
        private const float COL_NAME = 150f;
        private const float COL_VALUE = 60f;
        private const float COL_DIST = 40f;
        private const float COL_SPACING = 10f;

        private List<LootItem> _lastDrawnItems = new();

        /// <summary>
        /// Constructs a Loot Info Overlay.
        /// </summary>
        public LootInfoWidget(SKGLElement parent, SKRect location, bool minimized, float scale)
            : base(parent, "Loot Info", new SKPoint(location.Left, location.Top),
                new SKSize(location.Width, location.Height), scale, false)
        {
            Minimized = minimized;
            SetScaleFactor(scale);
        }

        public void Draw(SKCanvas canvas, IEnumerable<LootItem> loot, Vector3 localPos)
        {
            if (Minimized)
            {
                Draw(canvas);
                return;
            }

            using var filteredLoot = loot
                .Where(x => x.IsValuableLoot || x.Price > App.Config.Loot.MinValue)
                .OrderByDescending(x => x.Price)
                .ToPooledList();

            _lastDrawnItems.Clear();
            _lastDrawnItems.AddRange(filteredLoot);

            var font = SKFonts.InfoWidgetFont;
            float pad = 2.5f * ScaleFactor;
            var drawPt = new SKPoint(
                ClientRectangle.Left + pad,
                ClientRectangle.Top + font.Spacing / 2 + pad);

            float totalWidth = COL_NAME + COL_VALUE + COL_DIST + (COL_SPACING * 2);
            int drawCount = Math.Min(filteredLoot.Count, 20);
            
            Size = new SKSize(totalWidth + pad, (1 + drawCount) * font.Spacing);
            Draw(canvas);

            float x = drawPt.X;
            DrawColumn(canvas, "Name", ref x, COL_NAME, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Value", ref x, COL_VALUE, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Dist", ref x, COL_DIST, font, SKPaints.TextPlayersOverlay, drawPt.Y);

            drawPt.Offset(0, font.Spacing);

            for (int i = 0; i < drawCount; i++)
            {
                var item = filteredLoot[i];
                string name = Truncate(item.Name ?? "--", 25);
                string value = Utilities.FormatNumberKM(item.Price);
                string dist = ((int)Vector3.Distance(item.Position, localPos)).ToString();

                var rowRect = new SKRect(
                    ClientRectangle.Left,
                    drawPt.Y - font.Spacing / 2,
                    ClientRectangle.Right,
                    drawPt.Y + font.Spacing / 2
                );
                
                if (LastMousePosition.X >= ClientRectangle.Left && LastMousePosition.X <= ClientRectangle.Right &&
                    LastMousePosition.Y >= rowRect.Top && LastMousePosition.Y <= rowRect.Bottom)
                {
                    using var hoverPaint = new SKPaint
                    {
                        Color = SKColors.White.WithAlpha(50),
                        Style = SKPaintStyle.Fill
                    };
                    canvas.DrawRect(rowRect, hoverPaint);
                }

                SKPaint paint = SKPaints.TextLoot;
                if (item.IsValuableLoot) paint = SKPaints.TextImportantLoot;
                else if (item.IsQuestItem) paint = SKPaints.TextQuestItem;
                
                x = drawPt.X;
                DrawColumn(canvas, name, ref x, COL_NAME, font, paint, drawPt.Y);
                DrawColumn(canvas, value, ref x, COL_VALUE, font, paint, drawPt.Y);
                DrawColumn(canvas, dist, ref x, COL_DIST, font, paint, drawPt.Y);

                drawPt.Offset(0, font.Spacing);
            }
        }

        private static void DrawColumn(SKCanvas canvas, string text, ref float x, float width, SKFont font, SKPaint paint, float y)
        {
            canvas.DrawText(text, x, y, SKTextAlign.Left, font, paint);
            x += width + COL_SPACING;
        }

        private static string Truncate(string value, int maxLength)
        {
            if (string.IsNullOrEmpty(value) || value.Length <= maxLength)
                return value;
            return value.Substring(0, maxLength);
        }

        public override void SetScaleFactor(float newScale)
        {
            base.SetScaleFactor(newScale);
        }
        
        protected override void OnMouseClick(SKPoint position)
        {
            if (Minimized || _lastDrawnItems.Count == 0) return;
            
            var font = SKFonts.InfoWidgetFont;
            float pad = 2.5f * ScaleFactor;
            float listStartY = ClientRectangle.Top + pad + font.Spacing;
            
            if (position.Y < listStartY) return;
            
            int index = (int)((position.Y - listStartY) / font.Spacing);
            
            if (index >= 0 && index < _lastDrawnItems.Count)
            {
                _lastDrawnItems[index].TriggerPulse();
            }
        }
    }
}
