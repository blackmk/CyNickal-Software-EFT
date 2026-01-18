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

using LoneEftDmaRadar.Tarkov.GameWorld.Exits;
using MessagePack;

namespace LoneEftDmaRadar.Web.WebRadar.Data
{
    /// <summary>
    /// Exfil status for web radar.
    /// </summary>
    public enum WebExfilStatus : byte
    {
        Open = 0,
        Pending = 1,
        Closed = 2
    }

    [MessagePackObject]
    public readonly struct WebRadarExfil
    {
        /// <summary>
        /// Exfil Name.
        /// </summary>
        [Key(0)]
        public readonly string Name { get; init; }

        /// <summary>
        /// Exfil Status (Open, Pending, Closed).
        /// </summary>
        [Key(1)]
        public readonly WebExfilStatus Status { get; init; }

        /// <summary>
        /// Unity World Position.
        /// </summary>
        [Key(2)]
        public readonly Vector3 Position { get; init; }

        /// <summary>
        /// Create a WebRadarExfil from an Exfil object.
        /// </summary>
        public static WebRadarExfil? Create(IExitPoint exit)
        {
            if (exit is not Exfil exfil)
                return null;

            return new WebRadarExfil
            {
                Name = exfil.Name ?? "Unknown",
                Status = exfil.Status switch
                {
                    Exfil.EStatus.Open => WebExfilStatus.Open,
                    Exfil.EStatus.Pending => WebExfilStatus.Pending,
                    _ => WebExfilStatus.Closed
                },
                Position = exfil.Position
            };
        }
    }
}
