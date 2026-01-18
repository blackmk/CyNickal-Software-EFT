using ImGuiNET;
using Vortice.Direct3D;
using Vortice.Direct3D11;
using Vortice.DXGI;
using Vortice.Mathematics;
using WinForms = System.Windows.Forms;

namespace LoneEftDmaRadar.UI.ESP
{
    internal enum DxTextSize
    {
        Small,
        Medium,
        Large
    }

    internal sealed class ImGuiOverlayControl : WinForms.Control, IDisposable
    {
        public event Action<Exception> DeviceInitFailed;
        private ID3D11Device _device;
        private ID3D11DeviceContext _deviceContext;
        private IDXGISwapChain _swapChain;
        private ID3D11RenderTargetView _renderTargetView;
        
        // ImGui DX11 Resources
        private ID3D11VertexShader _vertexShader;
        private ID3D11PixelShader _pixelShader;
        private ID3D11InputLayout _inputLayout;
        private ID3D11Buffer _constantBuffer;
        private ID3D11BlendState _blendState;
        private ID3D11RasterizerState _rasterizerState;
        private ID3D11SamplerState _samplerState;
        private ID3D11Buffer _vertexBuffer;
        private ID3D11Buffer _indexBuffer;
        private int _vertexBufferSize = 50000, _indexBufferSize = 100000; // Larger initial sizes

        private ID3D11Texture2D _fontTexture;
        private ID3D11ShaderResourceView _fontTextureView;
        
        // Fonts
        private ImFontPtr _fontSmall;
        private ImFontPtr _fontMedium;
        private ImFontPtr _fontLarge;

        // Font Configuration
        private string _fontFamily = "Segoe UI";
        private string _fontWeight = "Regular";
        private int _fontSizeSmall = 15;
        private int _fontSizeMedium = 18;
        private int _fontSizeLarge = 32;

        // Map Texture
        private ID3D11Texture2D _mapTexture;
        private ID3D11ShaderResourceView _mapTextureView;
        private int _mapWidth, _mapHeight;
        private bool _mapUpdatePending;
        private byte[] _pendingMapPixels;
        private int _pendingMapW, _pendingMapH;

        public Action<ImGuiRenderContext> RenderFrame;
        
        private readonly object _deviceLock = new();
        private bool _isDeviceInitialized;

        private IntPtr _imGuiContext;
        
        // Cached arrays to avoid per-frame allocations
        private ID3D11Buffer[] _constantBufferArray;
        private ID3D11SamplerState[] _samplerArray;
        private ID3D11Buffer[] _vertexBufferArray;
        private uint[] _strideArray;
        private uint[] _offsetArray;
        private ID3D11ShaderResourceView[] _srvArray;
        private Viewport[] _viewportArray;
        private RectI[] _scissorArray;
        private int _lastWidth, _lastHeight;

        private int _renderWidth;
        private int _renderHeight;

        public ImGuiOverlayControl()
        {
            SetStyle(WinForms.ControlStyles.AllPaintingInWmPaint |
                     WinForms.ControlStyles.Opaque |
                     WinForms.ControlStyles.UserPaint, true);
            DoubleBuffered = false; 
            BackColor = System.Drawing.Color.Black;
            
            // Create context but don't bind it to this thread permanently yet
            _imGuiContext = ImGui.CreateContext();
            ImGui.SetCurrentContext(_imGuiContext);
            ImGui.StyleColorsDark();
            
            _renderWidth = Width;
            _renderHeight = Height;
        }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            try
            {
                InitializeDevice();
            }
            catch (Exception ex)
            {
                DeviceInitFailed?.Invoke(ex);
            }
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            lock (_deviceLock)
            {
                _renderWidth = Width;
                _renderHeight = Height;
                ResizeSwapChain();
            }
        }

        private void InitializeDevice()
        {
            lock (_deviceLock)
            {
                if (_isDeviceInitialized) return;

                var swapChainDesc = new SwapChainDescription
                {
                    BufferCount = 1, // Double buffering managed by DXGI usually, but 1 backbuffer + frontbuffer is standard for Discard
                    BufferDescription = new ModeDescription((uint)Width, (uint)Height, new Rational(60, 1), Format.R8G8B8A8_UNorm),
                    BufferUsage = Usage.RenderTargetOutput,
                    OutputWindow = Handle,
                    SampleDescription = new SampleDescription(1, 0),
                    SwapEffect = SwapEffect.Discard, // Safest for child windows
                    Windowed = true
                };

                if (Vortice.Direct3D11.D3D11.D3D11CreateDeviceAndSwapChain(
                    null,
                    DriverType.Hardware,
                    DeviceCreationFlags.BgraSupport, 
                    new[] { FeatureLevel.Level_11_0 },
                    swapChainDesc,
                    out _swapChain,
                    out _device,
                    out _,
                    out _deviceContext).Failure)
                {
                    throw new Exception("Failed to create DX11 Device/SwapChain");
                }
                
                CreateRenderTarget();
                
                // Ensure ImGui context is current for resource init
                ImGui.SetCurrentContext(_imGuiContext);
                InitImGuiResources();
                
                // Initialize cached arrays for zero-allocation rendering
                _constantBufferArray = new[] { _constantBuffer };
                _samplerArray = new[] { _samplerState };
                _vertexBufferArray = new ID3D11Buffer[1];
                unsafe { _strideArray = new[] { (uint)sizeof(ImDrawVert) }; }
                _offsetArray = new[] { 0u };
                _srvArray = new ID3D11ShaderResourceView[1];
                _viewportArray = new Viewport[1];
                _scissorArray = new RectI[1];
                
                _isDeviceInitialized = true;
            }
        }
        
        private void CreateRenderTarget()
        {
            using var backBuffer = _swapChain.GetBuffer<ID3D11Texture2D>(0);
            _renderTargetView = _device.CreateRenderTargetView(backBuffer);
        }

        private void ResizeSwapChain()
        {
            // Called within _deviceLock
            if (!_isDeviceInitialized) return;
            
#pragma warning disable CS8625
            _deviceContext.OMSetRenderTargets((ID3D11RenderTargetView)null, null);
#pragma warning restore CS8625
            _renderTargetView?.Dispose();
            
            _swapChain.ResizeBuffers(0, (uint)Math.Max(_renderWidth, 1), (uint)Math.Max(_renderHeight, 1), Format.Unknown, SwapChainFlags.None);
            CreateRenderTarget();
            
            // Set Viewport
            _deviceContext.RSSetViewports(new[] { new Viewport(0, 0, _renderWidth, _renderHeight) });
        }

        public void Render()
        {
            lock (_deviceLock)
            {
                if (!_isDeviceInitialized || _imGuiContext == IntPtr.Zero) return;
                
                // Now we are on UI thread, set context just in case
                ImGui.SetCurrentContext(_imGuiContext);

                try 
                {
                    // Handle Map Update
                    if (_mapUpdatePending)
                    {
                        UpdateMapTextureInternal(_pendingMapW, _pendingMapH, _pendingMapPixels);
                        _mapUpdatePending = false;
                        _pendingMapPixels = null;
                    }

                    _deviceContext.OMSetRenderTargets(_renderTargetView);
                    
                    // Only update viewport when size changes
                    if (_lastWidth != _renderWidth || _lastHeight != _renderHeight)
                    {
                        _lastWidth = _renderWidth;
                        _lastHeight = _renderHeight;
                        _viewportArray[0] = new Viewport(0, 0, _renderWidth, _renderHeight);
                    }
                    _deviceContext.RSSetViewports(_viewportArray);
                    
                    // Clear to transparent so window edges are invisible
                    _deviceContext.ClearRenderTargetView(_renderTargetView, new Color4(0, 0, 0, 0));

                    var io = ImGui.GetIO();
                    io.DisplaySize = new Vector2(_renderWidth, _renderHeight);
                    io.DeltaTime = 1.0f / 144.0f; // Assume higher refresh for smoother animation

                    ImGui.NewFrame();
                    
                    // Draw opaque black background first (fuser effect)
                    // This gives us black background while keeping text clean (no halos)
                    var bgDrawList = ImGui.GetBackgroundDrawList();
                    bgDrawList.AddRectFilled(Vector2.Zero, new Vector2(_renderWidth, _renderHeight), 0xFF000000); // ABGR: opaque black
                    
                    // Invoke User Rendering
                    RenderFrame?.Invoke(new ImGuiRenderContext(bgDrawList, _mapTextureView, _fontSmall, _fontMedium, _fontLarge));
                    
                    ImGui.Render();
                    RenderImGuiDrawData(ImGui.GetDrawData());
                    
                    _swapChain.Present(0, PresentFlags.None); // VSync off (CompositionTarget.Rendering handles 60hz limit)
                }
                catch (Exception ex)
                {
                   // Swallow render errors to prevent app crash, maybe log
                   System.Diagnostics.Debug.WriteLine($"Render Error: {ex}");
                }
            }
        }

        private unsafe void RenderImGuiDrawData(ImDrawDataPtr drawData)
        {
            if (drawData.CmdListsCount == 0) return;

            // Setup Render State - use cached arrays
            float L = drawData.DisplayPos.X;
            float R = drawData.DisplayPos.X + drawData.DisplaySize.X;
            float T = drawData.DisplayPos.Y;
            float B = drawData.DisplayPos.Y + drawData.DisplaySize.Y;
            var mvp = Matrix4x4.CreateOrthographicOffCenter(L, R, B, T, -1.0f, 1.0f);
            
            _deviceContext.UpdateSubresource(in mvp, _constantBuffer);

            _deviceContext.IASetInputLayout(_inputLayout);
            _deviceContext.VSSetShader(_vertexShader);
            _deviceContext.PSSetShader(_pixelShader);
            _deviceContext.VSSetConstantBuffers(0, _constantBufferArray);
            _deviceContext.PSSetSamplers(0, _samplerArray);
            _deviceContext.OMSetBlendState(_blendState);
            _deviceContext.RSSetState(_rasterizerState);
            _deviceContext.IASetPrimitiveTopology(PrimitiveTopology.TriangleList);

            // Resize buffers if needed
            if (_vertexBuffer == null || _vertexBufferSize < drawData.TotalVtxCount)
            {
                _vertexBuffer?.Dispose();
                _vertexBufferSize = Math.Max(_vertexBufferSize * 2, drawData.TotalVtxCount + 50000);
                int vtxSizeAligned = (_vertexBufferSize * sizeof(ImDrawVert) + 15) & ~15;
                _vertexBuffer = _device.CreateBuffer(new BufferDescription((uint)vtxSizeAligned, BindFlags.VertexBuffer, ResourceUsage.Dynamic, cpuAccessFlags: CpuAccessFlags.Write));
            }
            if (_indexBuffer == null || _indexBufferSize < drawData.TotalIdxCount)
            {
                _indexBuffer?.Dispose();
                _indexBufferSize = Math.Max(_indexBufferSize * 2, drawData.TotalIdxCount + 100000);
                int idxSizeAligned = (_indexBufferSize * sizeof(ushort) + 15) & ~15;
                _indexBuffer = _device.CreateBuffer(new BufferDescription((uint)idxSizeAligned, BindFlags.IndexBuffer, ResourceUsage.Dynamic, cpuAccessFlags: CpuAccessFlags.Write));
            }

            // Upload Vertex/Index Data
            MappedSubresource vtxResource = default;
            MappedSubresource idxResource = default;
            
            try
            {
                vtxResource = _deviceContext.Map(_vertexBuffer, 0, MapMode.WriteDiscard, Vortice.Direct3D11.MapFlags.None);
                idxResource = _deviceContext.Map(_indexBuffer, 0, MapMode.WriteDiscard, Vortice.Direct3D11.MapFlags.None);
                
                var vtxDst = (ImDrawVert*)vtxResource.DataPointer;
                var idxDst = (ushort*)idxResource.DataPointer;

                for (int n = 0; n < drawData.CmdListsCount; n++)
                {
                    var cmdList = drawData.CmdLists[n];
                    long vtxBytes = cmdList.VtxBuffer.Size * sizeof(ImDrawVert);
                    long idxBytes = cmdList.IdxBuffer.Size * sizeof(ushort);

                    Buffer.MemoryCopy((void*)cmdList.VtxBuffer.Data, vtxDst, vtxBytes, vtxBytes);
                    Buffer.MemoryCopy((void*)cmdList.IdxBuffer.Data, idxDst, idxBytes, idxBytes);
                    
                    vtxDst += cmdList.VtxBuffer.Size;
                    idxDst += cmdList.IdxBuffer.Size;
                }
            }
            finally
            {
                if (vtxResource.DataPointer != IntPtr.Zero) _deviceContext.Unmap(_vertexBuffer, 0);
                if (idxResource.DataPointer != IntPtr.Zero) _deviceContext.Unmap(_indexBuffer, 0);
            }

            // Use cached arrays for vertex buffer binding
            _vertexBufferArray[0] = _vertexBuffer;
            _deviceContext.IASetVertexBuffers(0, _vertexBufferArray, _strideArray, _offsetArray);
            _deviceContext.IASetIndexBuffer(_indexBuffer, Format.R16_UInt, 0);

            // Render Commands - track last texture to minimize state changes
            IntPtr lastTextureId = IntPtr.Zero;
            int vtxOffset = 0;
            int idxOffset = 0;
            
            for (int n = 0; n < drawData.CmdListsCount; n++)
            {
                var cmdList = drawData.CmdLists[n];
                for (int i = 0; i < cmdList.CmdBuffer.Size; i++)
                {
                    var cmd = cmdList.CmdBuffer[i];
                    if (cmd.UserCallback != IntPtr.Zero)
                    {
                        continue;
                    }
                    
                    // Only change texture if different from last
                    if (cmd.TextureId != lastTextureId)
                    {
                        lastTextureId = cmd.TextureId;
                        if (cmd.TextureId == _fontTextureView.NativePointer)
                        {
                            _srvArray[0] = _fontTextureView;
                        }
                        else if (_mapTextureView != null && cmd.TextureId == _mapTextureView.NativePointer)
                        {
                            _srvArray[0] = _mapTextureView;
                        }
                        _deviceContext.PSSetShaderResources(0, _srvArray);
                    }
                    
                    // Use cached scissor array
                    _scissorArray[0] = new RectI((int)cmd.ClipRect.X, (int)cmd.ClipRect.Y, (int)(cmd.ClipRect.Z - cmd.ClipRect.X), (int)(cmd.ClipRect.W - cmd.ClipRect.Y));
                    _deviceContext.RSSetScissorRects(_scissorArray);
                    _deviceContext.DrawIndexed(cmd.ElemCount, (uint)(idxOffset + (int)cmd.IdxOffset), vtxOffset + (int)cmd.VtxOffset);
                }
                vtxOffset += cmdList.VtxBuffer.Size;
                idxOffset += cmdList.IdxBuffer.Size;
            }
        }

        private void InitImGuiResources()
        {
            // Create Shaders
            // NOTE: We use precompiled bytecode for simplicity to avoid d3dcompiler dependency if possible, but Vortice includes D3DCompiler.
            // Using a simple HLSL for ImGui
            string vertexShaderSrc = @"
cbuffer vertexBuffer : register(b0) { float4x4 ProjectionMatrix; };
struct VS_INPUT { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv = input.uv;
    return output;
}";
            string pixelShaderSrc = @"
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);
float4 main(PS_INPUT input) : SV_Target {
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
}";

            var vsBlob = Vortice.D3DCompiler.Compiler.Compile(vertexShaderSrc, "main", "VS", "vs_4_0", Vortice.D3DCompiler.ShaderFlags.None);
            var psBlob = Vortice.D3DCompiler.Compiler.Compile(pixelShaderSrc, "main", "PS", "ps_4_0", Vortice.D3DCompiler.ShaderFlags.None);
            
            _vertexShader = _device.CreateVertexShader(vsBlob.Span, null);
            _pixelShader = _device.CreatePixelShader(psBlob.Span, null);

            _inputLayout = _device.CreateInputLayout(new[]
            {
                new InputElementDescription("POSITION", 0, Format.R32G32_Float, 0, 0, InputClassification.PerVertexData, 0),
                new InputElementDescription("TEXCOORD", 0, Format.R32G32_Float, 8, 0, InputClassification.PerVertexData, 0),
                new InputElementDescription("COLOR", 0, Format.R8G8B8A8_UNorm, 16, 0, InputClassification.PerVertexData, 0)
            }, vsBlob.Span);
            
            _constantBuffer = _device.CreateBuffer(new BufferDescription(64, BindFlags.ConstantBuffer, ResourceUsage.Default, cpuAccessFlags: CpuAccessFlags.None)); // 4x4 matrix

            // Blend State - Standard alpha blending for transparent overlay
            var blendDesc = new BlendDescription { AlphaToCoverageEnable = false };
            blendDesc.RenderTarget[0] = new RenderTargetBlendDescription
            {
                BlendEnable = true,
                SourceBlend = Blend.SourceAlpha,
                DestinationBlend = Blend.InverseSourceAlpha,
                BlendOperation = BlendOperation.Add,
                SourceBlendAlpha = Blend.One,  // Preserve source alpha
                DestinationBlendAlpha = Blend.InverseSourceAlpha,
                BlendOperationAlpha = BlendOperation.Add,
                RenderTargetWriteMask = ColorWriteEnable.All
            };
            _blendState = _device.CreateBlendState(blendDesc);

            // Rasterizer State
            var rasterDesc = new RasterizerDescription
            {
                FillMode = FillMode.Solid,
                CullMode = CullMode.None,
                ScissorEnable = true,
                DepthClipEnable = true
            };
            _rasterizerState = _device.CreateRasterizerState(rasterDesc);

            // Sampler State
            var samplerDesc = new SamplerDescription
            {
                Filter = Filter.MinMagMipLinear,
                AddressU = TextureAddressMode.Wrap,
                AddressV = TextureAddressMode.Wrap,
                AddressW = TextureAddressMode.Wrap,
                ComparisonFunc = ComparisonFunction.Always
            };
            _samplerState = _device.CreateSamplerState(samplerDesc);

            LoadFonts();
        }

        private void LoadFonts()
        {
            var io = ImGui.GetIO();
            io.Fonts.Clear();

            unsafe
            {
                var fontConfig = ImGuiNative.ImFontConfig_ImFontConfig();

                fontConfig->OversampleH = 2;
                fontConfig->OversampleV = 2;
                fontConfig->PixelSnapH = 0;
                fontConfig->GlyphExtraSpacing = new Vector2(0f, 0);

                // Set font weight multiplier based on weight setting
                // RasterizerMultiply > 1.0 makes font bolder, < 1.0 makes it thinner
                fontConfig->RasterizerMultiply = GetFontWeightMultiplier(_fontWeight);

                IntPtr glyphRanges = BuildFullUnicodeRanges(io.Fonts);

                try
                {
                    string fontPath = GetFontPath(_fontFamily, _fontWeight);

                    io.Fonts.TexDesiredWidth = 4096;
                    io.Fonts.Flags = ImFontAtlasFlags.NoPowerOfTwoHeight;

                    // Load fonts with configured sizes
                    _fontSmall = io.Fonts.AddFontFromFileTTF(fontPath, _fontSizeSmall, fontConfig, glyphRanges);
                    _fontMedium = io.Fonts.AddFontFromFileTTF(fontPath, _fontSizeMedium, fontConfig, glyphRanges);
                    _fontLarge = io.Fonts.AddFontFromFileTTF(fontPath, _fontSizeLarge, fontConfig, glyphRanges);
                }
                catch
                {
                    io.Fonts.AddFontDefault();
                    _fontSmall = io.Fonts.Fonts[0];
                    _fontMedium = io.Fonts.Fonts[0];
                    _fontLarge = io.Fonts.Fonts[0];
                }
                finally
                {
                    ImGuiNative.ImFontConfig_destroy(fontConfig);
                }
            }

            io.Fonts.GetTexDataAsRGBA32(out IntPtr pixels, out int width, out int height);

            if (width <= 0 || height <= 0 || pixels == IntPtr.Zero)
            {
                io.Fonts.Clear();
                io.Fonts.AddFontDefault();
                io.Fonts.GetTexDataAsRGBA32(out pixels, out width, out height);
            }

            _fontTexture = _device.CreateTexture2D(new Texture2DDescription
            {
                Width = (uint)width,
                Height = (uint)height,
                MipLevels = 1,
                ArraySize = 1,
                Format = Format.R8G8B8A8_UNorm,
                SampleDescription = new SampleDescription(1, 0),
                Usage = ResourceUsage.Default,
                BindFlags = BindFlags.ShaderResource,
                CPUAccessFlags = CpuAccessFlags.None
            }, new SubresourceData(pixels, (uint)(width * 4)));

            _fontTextureView = _device.CreateShaderResourceView(_fontTexture);
            io.Fonts.SetTexID(_fontTextureView.NativePointer);
        }

        private static float GetFontWeightMultiplier(string weight)
        {
            return weight.ToLowerInvariant() switch
            {
                "thin" => 0.7f,
                "light" => 0.8f,
                "regular" or "normal" or "default" => 1.0f,
                "medium" => 1.1f,
                "semibold" => 1.2f,
                "bold" => 1.3f,
                "extrabold" => 1.4f,
                "black" or "heavy" => 1.5f,
                _ => 1.0f
            };
        }

        private static string GetFontPath(string fontFamily, string fontWeight)
        {
            string fontsDir = @"C:\Windows\Fonts";
            if (!Directory.Exists(fontsDir))
                return $@"C:\Windows\Fonts\segoeui.ttf";

            string cleanName = fontFamily.Replace(" ", "").ToLowerInvariant();
            string[] extensions = { ".ttc", ".ttf", ".otf" };

            // Try exact match first
            foreach (string ext in extensions)
            {
                string exactPath = Path.Combine(fontsDir, cleanName + ext);
                if (File.Exists(exactPath))
                    return exactPath;
            }

            // Search through font files for matching name
            try
            {
                foreach (string file in Directory.GetFiles(fontsDir, "*.ttc"))
                {
                    string fileName = Path.GetFileNameWithoutExtension(file);
                    if (fileName.Replace(" ", "").Equals(cleanName, StringComparison.OrdinalIgnoreCase))
                        return file;
                }

                foreach (string file in Directory.GetFiles(fontsDir, "*.ttf"))
                {
                    string fileName = Path.GetFileNameWithoutExtension(file);
                    if (fileName.Replace(" ", "").Equals(cleanName, StringComparison.OrdinalIgnoreCase))
                        return file;
                }

                foreach (string file in Directory.GetFiles(fontsDir, "*.otf"))
                {
                    string fileName = Path.GetFileNameWithoutExtension(file);
                    if (fileName.Replace(" ", "").Equals(cleanName, StringComparison.OrdinalIgnoreCase))
                        return file;
                }
            }
            catch
            {
                // Ignore directory access errors
            }

            return $@"C:\Windows\Fonts\segoeui.ttf";
        }

        private string GetFontPath(string fontFamily)
        {
            return GetFontPath(fontFamily, _fontWeight);
        }

        public void RequestMapTextureUpdate(int width, int height, byte[] pixels)
        {
            lock (_deviceLock)
            {
                _pendingMapW = width;
                _pendingMapH = height;
                _pendingMapPixels = pixels;
                _mapUpdatePending = true;
            }
        }

        private void UpdateMapTextureInternal(int width, int height, byte[] pixels)
        {
             if (_mapTexture == null || _mapWidth != width || _mapHeight != height)
             {
                 _mapTextureView?.Dispose();
                 _mapTexture?.Dispose();
                 
                 _mapWidth = width;
                 _mapHeight = height;
                 
                 // Create texture - use Dynamic for CPU write access
                 _mapTexture = _device.CreateTexture2D(new Texture2DDescription
                 {
                     Width = (uint)width,
                     Height = (uint)height,
                     MipLevels = 1,
                     ArraySize = 1,
                     Format = Format.B8G8R8A8_UNorm, // Matches SkiaSharp Bgra8888
                     SampleDescription = new SampleDescription(1, 0),
                     Usage = ResourceUsage.Dynamic,
                     BindFlags = BindFlags.ShaderResource,
                     CPUAccessFlags = CpuAccessFlags.Write
                 });
                 
                 _mapTextureView = _device.CreateShaderResourceView(_mapTexture);
             }
             
             // Map and copy pixels directly
             var mapped = _deviceContext.Map(_mapTexture, 0, MapMode.WriteDiscard, Vortice.Direct3D11.MapFlags.None);
             try
             {
                 unsafe
                 {
                     int srcRowPitch = width * 4;
                     int dstRowPitch = (int)mapped.RowPitch;
                     byte* dst = (byte*)mapped.DataPointer;
                     
                     fixed (byte* src = pixels)
                     {
                         // Optimize: single copy if pitches match (common case for power-of-2 textures)
                         if (srcRowPitch == dstRowPitch)
                         {
                             Buffer.MemoryCopy(src, dst, srcRowPitch * height, srcRowPitch * height);
                         }
                         else
                         {
                             // Copy row by row to handle pitch differences
                             for (int y = 0; y < height; y++)
                             {
                                 Buffer.MemoryCopy(src + y * srcRowPitch, dst + y * dstRowPitch, srcRowPitch, srcRowPitch);
                             }
                         }
                     }
                 }
             }
             finally
             {
                 _deviceContext.Unmap(_mapTexture, 0);
             }
        }
        
        public void SetFontConfig(string fontFamily, int small, int medium, int large)
        {
            SetFontConfig(fontFamily, "Regular", small, medium, large);
        }

        public void SetFontConfig(string fontFamily, string fontWeight, int small, int medium, int large)
        {
            lock (_deviceLock)
            {
                if (!string.IsNullOrWhiteSpace(fontFamily))
                    _fontFamily = fontFamily.Trim();

                if (!string.IsNullOrWhiteSpace(fontWeight))
                    _fontWeight = fontWeight.Trim();

                _fontSizeSmall = ClampFontSize(small);
                _fontSizeMedium = ClampFontSize(medium);
                _fontSizeLarge = ClampFontSize(large);

                if (_isDeviceInitialized)
                {
                    RebuildFonts();
                }
            }
        }

        private static int ClampFontSize(int value)
        {
            if (value <= 0) return 10;
            return Math.Clamp(value, 6, 72);
        }

        private void RebuildFonts()
        {
            if (!_isDeviceInitialized) return;

            ImGui.SetCurrentContext(_imGuiContext);

            _fontTextureView?.Dispose();
            _fontTexture?.Dispose();

            LoadFonts();
        }

        private unsafe IntPtr BuildFullUnicodeRanges(ImFontAtlasPtr fonts) // not really full but I will name it anyway lol
        {
            var builder = ImGuiNative.ImFontGlyphRangesBuilder_ImFontGlyphRangesBuilder();

            ushort* defaultRanges = (ushort*)fonts.GetGlyphRangesDefault();
            ushort* greekRanges = (ushort*)fonts.GetGlyphRangesGreek();
            ushort* koreanRanges = (ushort*)fonts.GetGlyphRangesKorean();
            ushort* japaneseRanges = (ushort*)fonts.GetGlyphRangesJapanese();
            ushort* chineseRanges = (ushort*)fonts.GetGlyphRangesChineseFull();
            ushort* cyrillicRanges = (ushort*)fonts.GetGlyphRangesCyrillic();
            ushort* thaiRanges = (ushort*)fonts.GetGlyphRangesThai();
            ushort* vietnameseRanges = (ushort*)fonts.GetGlyphRangesVietnamese();

            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, defaultRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, greekRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, koreanRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, japaneseRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, chineseRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, cyrillicRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, thaiRanges);
            ImGuiNative.ImFontGlyphRangesBuilder_AddRanges(builder, vietnameseRanges);

            ImVector resultRanges;
            ImGuiNative.ImFontGlyphRangesBuilder_BuildRanges(builder, &resultRanges);

            ImGuiNative.ImFontGlyphRangesBuilder_destroy(builder);

            return resultRanges.Data;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (_deviceLock)
                {
                    _deviceContext?.ClearState();
                    _deviceContext?.Flush();
                    _renderTargetView?.Dispose();
                    _renderTargetView = null;
                    _fontTextureView?.Dispose();
                    _fontTextureView = null;
                    _fontTexture?.Dispose();
                    _fontTexture = null;
                    _mapTextureView?.Dispose();
                    _mapTextureView = null;
                    _mapTexture?.Dispose();
                    _mapTexture = null;

                    if (_imGuiContext != IntPtr.Zero)
                    {
                        ImGui.DestroyContext(_imGuiContext);
                        _imGuiContext = IntPtr.Zero;
                    }

                    _vertexShader?.Dispose();
                    _vertexShader = null;
                    _pixelShader?.Dispose();
                    _pixelShader = null;
                    _inputLayout?.Dispose();
                    _inputLayout = null;
                    _constantBuffer?.Dispose();
                    _constantBuffer = null;
                    _blendState?.Dispose();
                    _blendState = null;
                    _rasterizerState?.Dispose();
                    _rasterizerState = null;
                    _samplerState?.Dispose();
                    _samplerState = null;
                    _vertexBuffer?.Dispose();
                    _vertexBuffer = null;
                    _indexBuffer?.Dispose();
                    _indexBuffer = null;
                    _swapChain?.Dispose();
                    _swapChain = null;
                    _deviceContext?.Dispose();
                    _deviceContext = null;
                    _device?.Dispose();
                    _device = null;

                    _isDeviceInitialized = false;
                }
            }
            base.Dispose(disposing);
        }
    }

    internal readonly struct ImGuiRenderContext
    {
        private readonly ImDrawListPtr _drawList;
        private readonly ID3D11ShaderResourceView _mapTexture;
        private readonly ImFontPtr _fontSmall;
        private readonly ImFontPtr _fontMedium;
        private readonly ImFontPtr _fontLarge;

        public float Width => ImGui.GetIO().DisplaySize.X;
        public float Height => ImGui.GetIO().DisplaySize.Y;

        public ImGuiRenderContext(ImDrawListPtr drawList, ID3D11ShaderResourceView mapTexture, ImFontPtr small, ImFontPtr medium, ImFontPtr large)
        {
            _drawList = drawList;
            _mapTexture = mapTexture;
            _fontSmall = small;
            _fontMedium = medium;
            _fontLarge = large;
        }

        public void DrawLine(SharpDX.Mathematics.Interop.RawVector2 p1, SharpDX.Mathematics.Interop.RawVector2 p2, SharpDX.Mathematics.Interop.RawColorBGRA color, float thickness)
        {
             _drawList.AddLine(new Vector2(p1.X, p1.Y), new Vector2(p2.X, p2.Y), PackColor(color), thickness);
        }
        
        public void DrawRect(System.Drawing.RectangleF rect, SharpDX.Mathematics.Interop.RawColorBGRA color, float thickness)
        {
            _drawList.AddRect(new Vector2(rect.Left, rect.Top), new Vector2(rect.Right, rect.Bottom), PackColor(color), 0f, ImDrawFlags.None, thickness);
        }

        public void DrawFilledRect(System.Drawing.RectangleF rect, SharpDX.Mathematics.Interop.RawColorBGRA color)
        {
            _drawList.AddRectFilled(new Vector2(rect.Left, rect.Top), new Vector2(rect.Right, rect.Bottom), PackColor(color));
        }

        public void DrawCircle(SharpDX.Mathematics.Interop.RawVector2 center, float radius, SharpDX.Mathematics.Interop.RawColorBGRA color, bool filled)
        {
            if (filled)
                _drawList.AddCircleFilled(new Vector2(center.X, center.Y), radius, PackColor(color));
            else
                _drawList.AddCircle(new Vector2(center.X, center.Y), radius, PackColor(color));
        }
        
        public void DrawText(string text, float x, float y, SharpDX.Mathematics.Interop.RawColorBGRA color, DxTextSize size = DxTextSize.Small, bool centerX = false, bool centerY = false)
        {
             var font = GetFont(size);
             if (string.IsNullOrEmpty(text)) return;
             
             // Calculate size if centering
             if (centerX || centerY)
             {
                 var dim = font.CalcTextSizeA(font.FontSize, float.MaxValue, float.MaxValue, text);
                 if (centerX) x -= dim.X / 2f;
                 if (centerY) y -= dim.Y / 2f;
             }
             
             _drawList.AddText(font, font.FontSize, new Vector2(x, y), PackColor(color), text);
        }

        public SharpDX.Mathematics.Interop.RawRectangle MeasureText(string text, DxTextSize size)
        {
             var font = GetFont(size);
             if (string.IsNullOrEmpty(text)) return new SharpDX.Mathematics.Interop.RawRectangle(0,0,0,0);
             
             var dim = font.CalcTextSizeA(font.FontSize, float.MaxValue, float.MaxValue, text);
             return new SharpDX.Mathematics.Interop.RawRectangle(0, 0, (int)dim.X, (int)dim.Y);
        }

        private ImFontPtr GetFont(DxTextSize size) => size switch
        {
            DxTextSize.Small => _fontSmall,
            DxTextSize.Large => _fontLarge,
            _ => _fontMedium
        };


        
        public void DrawMapTexture(System.Drawing.RectangleF destRect, float opacity)
        {
            if (_mapTexture != null)
            {
               var color = ImGui.GetColorU32(new Vector4(1, 1, 1, opacity));
               // _mapTexture is actually the ShaderResourceView passed from ImGuiOverlayControl
               _drawList.AddImage(_mapTexture.NativePointer, new Vector2(destRect.Left, destRect.Top), new Vector2(destRect.Right, destRect.Bottom), Vector2.Zero, Vector2.One, color);
            }
        }
        
        public void Clear(SharpDX.Mathematics.Interop.RawColorBGRA color)
        {
             // Handled by device clear usually, but we can draw a big rect
             // _drawList.AddRectFilled(...)
             // But we cleared device in Render().
        }

        private static uint PackColor(SharpDX.Mathematics.Interop.RawColorBGRA color)
        {
            // ImGui uses RGBA packed uint? or ABGR?
            // ImGui.GetColorU32 expects 0xAABBGGRR usually? 
            // SharpDX RawColorBGRA is B, G, R, A members.
            // Let's verify ImGui.NET packing. It usually matches the backend. 
            // Helper: ImGui.GetColorU32(Vector4) is safest.
            return ImGui.GetColorU32(new Vector4(color.R / 255f, color.G / 255f, color.B / 255f, color.A / 255f));
        }

    }
}
