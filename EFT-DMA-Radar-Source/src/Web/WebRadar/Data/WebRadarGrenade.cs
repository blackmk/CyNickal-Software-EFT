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

using LoneEftDmaRadar.Tarkov.GameWorld.Explosives;
using MessagePack;

namespace LoneEftDmaRadar.Web.WebRadar.Data
{
    /// <summary>
    /// Type of explosive for web radar.
    /// </summary>
    public enum WebExplosiveType : byte
    {
        Grenade = 0,
        Tripwire = 1
    }

    [MessagePackObject]
    public readonly struct WebRadarGrenade
    {
        /// <summary>
        /// Type of explosive (Grenade or Tripwire).
        /// </summary>
        [Key(0)]
        public readonly WebExplosiveType Type { get; init; }

        /// <summary>
        /// Unity World Position.
        /// </summary>
        [Key(1)]
        public readonly Vector3 Position { get; init; }

        /// <summary>
        /// Create a WebRadarGrenade from an IExplosiveItem object.
        /// </summary>
        public static WebRadarGrenade? Create(IExplosiveItem explosive)
        {
            if (explosive is null || explosive.Position == Vector3.Zero)
                return null;

            if (explosive is Tripwire tripwire)
            {
                if (!tripwire.IsActive)
                    return null;
                    
                return new WebRadarGrenade
                {
                    Type = WebExplosiveType.Tripwire,
                    Position = tripwire.Position
                };
            }
            else if (explosive is Grenade grenade)
            {
                return new WebRadarGrenade
                {
                    Type = WebExplosiveType.Grenade,
                    Position = grenade.Position
                };
            }

            return null;
        }
    }
}
