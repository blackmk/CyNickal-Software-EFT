﻿/*
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
using LoneEftDmaRadar.Tarkov.GameWorld.Player;
using LoneEftDmaRadar.Tarkov.GameWorld.Player.Helpers;
using LoneEftDmaRadar.Web.TarkovDev.Data;
using SkiaSharp.Views.WPF;

namespace LoneEftDmaRadar.UI.Skia
{
    public sealed class PlayerInfoWidget : AbstractSKWidget
    {
        // Column widths in pixels
        private const float COL_GRP = 20f;
        private const float COL_NAME = 50f;
        private const float COL_HANDS = 120f;
        private const float COL_SECURE = 65f;
        private const float COL_VALUE = 30f;
        private const float COL_DIST = 30f;
        private const float COL_SPACING = 10f;

        /// <summary>
        /// Constructs a Player Info Overlay.
        /// </summary>
        public PlayerInfoWidget(SKGLElement parent, SKRect location, bool minimized, float scale)
            : base(parent, "Player Info", new SKPoint(location.Left, location.Top),
                new SKSize(location.Width, location.Height), scale, false)
        {
            Minimized = minimized;
            SetScaleFactor(scale);
        }


        public void Draw(SKCanvas canvas, AbstractPlayer localPlayer, IEnumerable<AbstractPlayer> players)
        {
            if (Minimized)
            {
                Draw(canvas);
                return;
            }

            // Sort & filter
            var localPos = localPlayer.Position;
            using var filteredPlayers = players
                .Where(p => p.IsHumanHostileActive)
                .OrderBy(p => Vector3.DistanceSquared(localPos, p.Position))
                .ToPooledList();

            // Setup Frame and Draw Header
            var font = SKFonts.InfoWidgetFont;
            float pad = 2.5f * ScaleFactor;
            var drawPt = new SKPoint(
                ClientRectangle.Left + pad,
                ClientRectangle.Top + font.Spacing / 2 + pad);

            // Calculate total width
            float totalWidth = COL_GRP + COL_NAME + COL_HANDS + COL_SECURE + COL_VALUE + COL_DIST + (COL_SPACING * 5);
            Size = new SKSize(totalWidth + pad, (1 + filteredPlayers.Count) * font.Spacing);
            Draw(canvas); // Background/frame

            // Draw header
            float x = drawPt.X;
            DrawColumn(canvas, "Grp", ref x, COL_GRP, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Name", ref x, COL_NAME, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "In Hands", ref x, COL_HANDS, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Secure", ref x, COL_SECURE, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Value", ref x, COL_VALUE, font, SKPaints.TextPlayersOverlay, drawPt.Y);
            DrawColumn(canvas, "Dist", ref x, COL_DIST, font, SKPaints.TextPlayersOverlay, drawPt.Y);

            drawPt.Offset(0, font.Spacing);

            foreach (var player in filteredPlayers)
            {
                string name = Truncate(player.Name ?? "--", 10);
                string grp = player.GroupID != -1 ? player.GroupID.ToString() : "--";
                string hands = "--";
                string secure = "--";
                string value = "--";
                string dist = "--";

                if (player is ObservedPlayer obs)
                {
                    hands = Truncate(obs.Equipment?.InHands?.ShortName ?? "--", 15);
                    secure = Truncate(obs.Equipment?.SecuredContainer?.ShortName ?? "--", 10);
                    value = Utilities.FormatNumberKM(obs.Equipment?.Value ?? 0);
                    dist = ((int)Vector3.Distance(player.Position, localPos)).ToString();
                }

                var paint = GetTextPaint(player);
                x = drawPt.X;
                DrawColumn(canvas, grp, ref x, COL_GRP, font, paint, drawPt.Y);
                DrawColumn(canvas, name, ref x, COL_NAME, font, paint, drawPt.Y);
                DrawColumn(canvas, hands, ref x, COL_HANDS, font, paint, drawPt.Y);
                DrawColumn(canvas, secure, ref x, COL_SECURE, font, paint, drawPt.Y);
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

        private static SKPaint GetTextPaint(AbstractPlayer player)
        {
            if (player.IsFocused)
                return SKPaints.TextPlayersOverlayFocused;
            switch (player.Type)
            {
                case PlayerType.PMC:
                    return SKPaints.TextPlayersOverlayPMC;
                case PlayerType.PScav:
                    return SKPaints.TextPlayersOverlayPScav;
                default:
                    return SKPaints.TextPlayersOverlay;
            }
        }


        public override void SetScaleFactor(float newScale)
        {
            base.SetScaleFactor(newScale);
        }
    }
}