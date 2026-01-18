using LoneEftDmaRadar.Tarkov;
using LoneEftDmaRadar.UI.Misc;
using SDK;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Windows.Input;
using System.Windows.Threading;

namespace LoneEftDmaRadar.UI.Radar.ViewModels
{
    public sealed class MemoryInspectorViewModel : INotifyPropertyChanged
    {
        private readonly DispatcherTimer _refreshTimer;
        private string _searchText = "";
        private OffsetStructInfo _selectedStruct;
        private bool _autoRefresh = false;
        private string _baseAddressHex = "";
        private ulong _baseAddress = 0;

        public MemoryInspectorViewModel()
        {
            RefreshCommand = new SimpleCommand(RefreshValues);
            NavigateCommand = new SimpleCommand<OffsetFieldInfo>(Navigate);
            NavigateBackCommand = new SimpleCommand(NavigateBack);
            
            DiscoverOffsetStructs();
            
            _refreshTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(100)
            };
            _refreshTimer.Tick += (_, _) => { if (AutoRefresh && SelectedStruct != null) RefreshValues(); };
        }

        #region Properties

        public ObservableCollection<OffsetStructInfo> AllStructs { get; } = new();
        public ObservableCollection<OffsetStructInfo> FilteredStructs { get; } = new();
        public ObservableCollection<OffsetFieldInfo> CurrentFields { get; } = new();

        public string SearchText
        {
            get => _searchText;
            set
            {
                if (_searchText != value)
                {
                    _searchText = value;
                    OnPropertyChanged(nameof(SearchText));
                    FilterStructs();
                }
            }
        }

        public OffsetStructInfo SelectedStruct
        {
            get => _selectedStruct;
            set
            {
                if (_selectedStruct != value)
                {
                    _selectedStruct = value;
                    OnPropertyChanged(nameof(SelectedStruct));
                    OnStructSelected();
                }
            }
        }

        public bool AutoRefresh
        {
            get => _autoRefresh;
            set
            {
                if (_autoRefresh != value)
                {
                    _autoRefresh = value;
                    OnPropertyChanged(nameof(AutoRefresh));
                    if (value) _refreshTimer.Start(); else _refreshTimer.Stop();
                }
            }
        }

        public string BaseAddressHex
        {
            get => _baseAddressHex;
            set
            {
                if (_baseAddressHex != value)
                {
                    _baseAddressHex = value;
                    OnPropertyChanged(nameof(BaseAddressHex));
                    TryParseBaseAddress();
                }
            }
        }

        public string BaseAddressStatus => _baseAddress != 0 ? $"0x{_baseAddress:X}" : "Not Set";

        private bool _showOnEsp = false;
        public bool ShowOnEsp
        {
            get => _showOnEsp;
            set
            {
                if (_showOnEsp != value)
                {
                    _showOnEsp = value;
                    OnPropertyChanged(nameof(ShowOnEsp));
                    UpdateEspOverlayData();
                }
            }
        }

        public static volatile MemoryInspectorEspData EspOverlayData = new();

        #endregion

        #region Commands

        public ICommand RefreshCommand { get; }
        public ICommand NavigateCommand { get; }
        public ICommand NavigateBackCommand { get; }

        private readonly Stack<(OffsetStructInfo Struct, ulong Address, string Hint)> _navigationHistory = new();

        #endregion

        #region Methods

        private void Navigate(OffsetFieldInfo field)
        {
            if (field == null || field.Address == 0) return;

            var targetStructName = GuessStructName(field.Name, field.FieldType);
            if (string.IsNullOrEmpty(targetStructName)) return;

            var targetStruct = AllStructs.FirstOrDefault(s => s.Name.Equals(targetStructName, StringComparison.OrdinalIgnoreCase));
            if (targetStruct == null)
            {
                targetStruct = AllStructs.FirstOrDefault(s => targetStructName.Contains(s.Name, StringComparison.OrdinalIgnoreCase));
            }

            if (targetStruct != null)
            {
                _navigationHistory.Push((SelectedStruct, _baseAddress, _autoDetectHint));
                OnPropertyChanged(nameof(CanNavigateBack));

                SelectedStruct = targetStruct;
                
                ulong newBase = 0;
                try
                {
                    newBase = Memory.ReadPtr(field.Address, false);
                }
                catch { }

                if (newBase != 0)
                {
                    _baseAddress = newBase;
                    _baseAddressHex = $"0x{newBase:X}";
                    OnPropertyChanged(nameof(BaseAddressHex));
                    OnPropertyChanged(nameof(BaseAddressStatus));
                    
                    _autoDetectHint = $"-> {field.Name}";
                    OnPropertyChanged(nameof(AutoDetectHint));
                    
                    RefreshValues();
                }
            }
        }

        private void NavigateBack()
        {
            if (_navigationHistory.Count > 0)
            {
                var state = _navigationHistory.Pop();
                SelectedStruct = state.Struct;
                _baseAddress = state.Address;
                _baseAddressHex = $"0x{state.Address:X}";
                _autoDetectHint = state.Hint;
                
                OnPropertyChanged(nameof(BaseAddressHex));
                OnPropertyChanged(nameof(BaseAddressStatus));
                OnPropertyChanged(nameof(AutoDetectHint));
                OnPropertyChanged(nameof(CanNavigateBack));
                
                RefreshValues();
            }
        }

        public bool CanNavigateBack => _navigationHistory.Count > 0;

        private string GuessStructName(string fieldName, Type fieldType)
        {
            switch (fieldName)
            {
                case "Breath": return "BreathEffector";
                case "Shootingg": return "ShotEffector";
                case "MotionReact": return "MotionEffector";
                case "NewShotRecoil": return "NewShotRecoil";
                case "HandsContainer": return "PlayerSpring"; 
                case "FPSCamera": return "FPSCamera";
                case "Physical": return "Physical";
                case "MovementContext": return "MovementContext";
                case "_playerBody": return "PlayerBody";
                case "_handsController": return "FirearmController"; 
                case "HandsController": return "FirearmController";
                case "WeaponAnimation": return "ProceduralWeaponAnimation";
                case "Profile": return "Profile";
                case "Info": return "PlayerInfo"; 
                case "SkeletonRootJoint": return "DizSkinningSkeleton";
                case "Item": return "LootItem";
                case "Template": return "ItemTemplate";
                case "Grids": return "LootItemMod"; 
                case "Shots": return "NewShotRecoil"; 
            }

            if (AllStructs.Any(s => s.Name.Equals(fieldName, StringComparison.OrdinalIgnoreCase)))
                return fieldName;

            string cleanName = fieldName.TrimStart('_');
            if (AllStructs.Any(s => s.Name.Equals(cleanName, StringComparison.OrdinalIgnoreCase)))
                return cleanName;

            return null;
        }

        private void UpdateEspOverlayData()
        {
            if (!ShowOnEsp || SelectedStruct == null)
            {
                EspOverlayData = new MemoryInspectorEspData { Enabled = false };
                return;
            }

            var data = new MemoryInspectorEspData
            {
                Enabled = true,
                StructName = SelectedStruct.Name,
                BaseAddress = _baseAddress,
                Fields = CurrentFields.Select(f => new MemoryInspectorEspField
                {
                    Name = f.Name,
                    Value = f.Value,
                    Offset = f.Offset
                }).ToList()
            };
            EspOverlayData = data;
        }


        private void DiscoverOffsetStructs()
        {
            AllStructs.Clear();
            
            var offsetsType = typeof(Offsets);
            
            foreach (var nestedType in offsetsType.GetNestedTypes(BindingFlags.Public | BindingFlags.NonPublic))
            {
                if (nestedType.IsValueType && nestedType.Name != "Offsets")
                {
                    var structInfo = new OffsetStructInfo
                    {
                        Name = nestedType.Name,
                        Type = nestedType,
                        Fields = new List<OffsetFieldInfo>()
                    };

                    foreach (var field in nestedType.GetFields(BindingFlags.Public | BindingFlags.Static))
                    {
                        if (field.IsLiteral && !field.IsInitOnly) 
                        {
                            var value = field.GetValue(null);
                            structInfo.Fields.Add(new OffsetFieldInfo
                            {
                                Name = field.Name,
                                Offset = Convert.ToUInt32(value),
                                FieldType = field.FieldType,
                                Comment = "" 
                            });
                        }
                    }

                    structInfo.Fields = structInfo.Fields.OrderBy(f => f.Offset).ToList();
                    AllStructs.Add(structInfo);
                }
            }

            var sorted = AllStructs.OrderBy(s => s.Name).ToList();
            AllStructs.Clear();
            foreach (var s in sorted) AllStructs.Add(s);

            FilterStructs();
        }

        private void FilterStructs()
        {
            FilteredStructs.Clear();
            var filter = SearchText?.ToLowerInvariant() ?? "";
            
            foreach (var s in AllStructs)
            {
                if (string.IsNullOrEmpty(filter) || 
                    s.Name.ToLowerInvariant().Contains(filter) ||
                    s.Fields.Any(f => f.Name.ToLowerInvariant().Contains(filter)))
                {
                    FilteredStructs.Add(s);
                }
            }
        }

        private void OnStructSelected()
        {
            CurrentFields.Clear();
            if (SelectedStruct == null) return;

            foreach (var field in SelectedStruct.Fields)
            {
                CurrentFields.Add(new OffsetFieldInfo
                {
                    Name = field.Name,
                    Offset = field.Offset,
                    FieldType = field.FieldType,
                    Value = "(not read)",
                    Comment = field.Comment
                });
            }

            TryAutoSetBaseAddress();
        }

        private void TryAutoSetBaseAddress()
        {
            if (SelectedStruct == null) return;

            var localPlayer = Memory.LocalPlayer;
            if (localPlayer == null) return;

            ulong baseAddr = 0;
            string hint = "";
            var structName = SelectedStruct.Name;

            try
            {
                switch (structName)
                {
                    case "Player":
                        baseAddr = localPlayer.Base;
                        hint = "LocalPlayer";
                        break;
                    
                    case "MovementContext":
                        baseAddr = localPlayer.MovementContext;
                        hint = "LocalPlayer.MovementContext";
                        break;
                    
                    case "Profile":
                        baseAddr = Memory.ReadPtr(localPlayer.Base + Offsets.Player.Profile, false);
                        hint = "LocalPlayer.Profile";
                        break;
                    
                    case "PlayerInfo":
                        var profile = Memory.ReadPtr(localPlayer.Base + Offsets.Player.Profile, false);
                        if (profile != 0)
                            baseAddr = Memory.ReadPtr(profile + Offsets.Profile.Info, false);
                        hint = "LocalPlayer.Profile.Info";
                        break;
                    
                    case "Physical":
                        baseAddr = Memory.ReadPtr(localPlayer.Base + Offsets.Player.Physical, false);
                        hint = "LocalPlayer.Physical";
                        break;
                    
                    case "PlayerBody":
                        baseAddr = Memory.ReadPtr(localPlayer.Base + Offsets.Player._playerBody, false);
                        hint = "LocalPlayer._playerBody";
                        break;

                    case "FirearmController":
                    case "ClientFirearmController":
                    case "ItemHandsController":
                        baseAddr = localPlayer.HandsController;
                        hint = "LocalPlayer.HandsController";
                        break;

                    case "ProceduralWeaponAnimation":
                        baseAddr = localPlayer.PWA;
                        hint = "LocalPlayer.PWA";
                        break;
                    
                    case "BreathEffector":
                        if (localPlayer.PWA != 0)
                            baseAddr = Memory.ReadPtr(localPlayer.PWA + Offsets.ProceduralWeaponAnimation.Breath, false);
                        hint = "PWA.Breath";
                        break;
                    
                    case "ShotEffector":
                        if (localPlayer.PWA != 0)
                            baseAddr = Memory.ReadPtr(localPlayer.PWA + Offsets.ProceduralWeaponAnimation.Shootingg, false);
                        hint = "PWA.Shootingg";
                        break;
                    
                    case "NewShotRecoil":
                        if (localPlayer.PWA != 0)
                        {
                            var shotEffector = Memory.ReadPtr(localPlayer.PWA + Offsets.ProceduralWeaponAnimation.Shootingg, false);
                            if (shotEffector != 0)
                                baseAddr = Memory.ReadPtr(shotEffector + Offsets.ShotEffector.NewShotRecoil, false);
                        }
                        hint = "PWA.Shootingg.NewShotRecoil";
                        break;

                    case "LootItemWeapon":
                        if (localPlayer.HandsController != 0)
                            baseAddr = Memory.ReadPtr(localPlayer.HandsController + Offsets.ItemHandsController.Item, false);
                        hint = "HandsController.Item (Weapon)";
                        break;
                    
                    case "LootItem":
                    case "ItemTemplate":
                        if (localPlayer.HandsController != 0)
                        {
                            var item = Memory.ReadPtr(localPlayer.HandsController + Offsets.ItemHandsController.Item, false);
                            if (structName == "ItemTemplate" && item != 0)
                                baseAddr = Memory.ReadPtr(item + Offsets.LootItem.Template, false);
                            else
                                baseAddr = item;
                        }
                        hint = structName == "ItemTemplate" ? "Weapon.Template" : "HandsController.Item";
                        break;

                    case "FPSCamera":
                        baseAddr = Tarkov.GameWorld.Camera.CameraManager.FPSCameraPtr;
                        hint = "CameraManager.FPSCamera";
                        break;

                    case "MovementState":
                    case "PlayerStateContainer":
                        if (localPlayer.MovementContext != 0)
                            baseAddr = Memory.ReadPtr(localPlayer.MovementContext + Offsets.MovementContext.CurrentState, false);
                        hint = "MovementContext.CurrentState";
                        break;

                    case "DizSkinningSkeleton":
                        var playerBody = Memory.ReadPtr(localPlayer.Base + Offsets.Player._playerBody, false);
                        if (playerBody != 0)
                            baseAddr = Memory.ReadPtr(playerBody + Offsets.PlayerBody.SkeletonRootJoint, false);
                        hint = "_playerBody.SkeletonRootJoint";
                        break;

                    default:
                        hint = "(no auto-detection)";
                        break;
                }
            }
            catch (Exception ex)
            {
                baseAddr = 0;
                hint = $"Error: {ex.Message}";
            }

            if (baseAddr != 0)
            {
                _baseAddress = baseAddr;
                _baseAddressHex = $"0x{baseAddr:X}";
                OnPropertyChanged(nameof(BaseAddressHex));
                OnPropertyChanged(nameof(BaseAddressStatus));
            }
            
            _autoDetectHint = hint;
            OnPropertyChanged(nameof(AutoDetectHint));
        }

        private string _autoDetectHint = "";
        public string AutoDetectHint => _autoDetectHint;

        private void TryParseBaseAddress()
        {
            var hex = BaseAddressHex?.Trim() ?? "";
            if (hex.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
                hex = hex.Substring(2);

            if (ulong.TryParse(hex, System.Globalization.NumberStyles.HexNumber, null, out var addr))
            {
                _baseAddress = addr;
            }
            else
            {
                _baseAddress = 0;
            }
            OnPropertyChanged(nameof(BaseAddressStatus));
        }

        private void RefreshValues()
        {
            if (_baseAddress == 0 || CurrentFields.Count == 0) return;

            foreach (var field in CurrentFields)
            {
                try
                {
                    var addr = _baseAddress + field.Offset;
                    string value = ReadValueAtAddress(addr, field);
                    field.Value = value;
                    field.Address = addr;
                }
                catch (Exception ex)
                {
                    field.Value = $"Error: {ex.Message}";
                }
            }

            OnPropertyChanged(nameof(CurrentFields));
            
            if (ShowOnEsp)
                UpdateEspOverlayData();
        }

        private string ReadValueAtAddress(ulong addr, OffsetFieldInfo field)
        {
            var name = field.Name.ToLowerInvariant();
            
            if (name.StartsWith("is") || name.Contains("enabled") || name.Contains("on") && field.Offset % 4 != 0)
            {
                var boolVal = Memory.ReadValue<bool>(addr, false);
                return $"{boolVal}";
            }
            
            if (name.Contains("object") || name.Contains("random") || name.Contains("controller") || 
                name.Contains("effector") || name.Contains("spring") || name.Contains("params") ||
                name.Contains("curves") || name.Contains("processors") || name.Contains("level") ||
                name.Contains("amplitudes") || field.FieldType == typeof(ulong))
            {
                var ptr = Memory.ReadPtr(addr, false);
                return ptr != 0 ? $"0x{ptr:X}" : "null";
            }

            var floatVal = Memory.ReadValue<float>(addr, false);
            if (float.IsNaN(floatVal) || float.IsInfinity(floatVal) || Math.Abs(floatVal) > 1e10)
            {
                var intVal = Memory.ReadValue<int>(addr, false);
                return $"{intVal} (0x{intVal:X})";
            }
            
            return $"{floatVal:F4}";
        }

        private void CopyFieldAddress(OffsetFieldInfo field)
        {
            if (field != null && field.Address != 0)
            {
                try
                {
                    System.Windows.Clipboard.SetText($"0x{field.Address:X}");
                }
                catch { }
            }
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string name) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

        #endregion
    }

    #region Models

    public class OffsetStructInfo
    {
        public string Name { get; set; }
        public Type Type { get; set; }
        public List<OffsetFieldInfo> Fields { get; set; }
        public int FieldCount => Fields?.Count ?? 0;
        public override string ToString() => $"{Name} ({FieldCount} fields)";
    }

    public class OffsetFieldInfo : INotifyPropertyChanged
    {
        public string Name { get; set; }
        public uint Offset { get; set; }
        public Type FieldType { get; set; }
        public string Comment { get; set; }
        public ulong Address { get; set; }

        private string _value = "";
        public string Value
        {
            get => _value;
            set
            {
                if (_value != value)
                {
                    _value = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Value)));
                }
            }
        }

        public string OffsetHex => $"0x{Offset:X}";

        public event PropertyChangedEventHandler PropertyChanged;
    }

    #endregion

    #region ESP Overlay Data

    public class MemoryInspectorEspData
    {
        public bool Enabled { get; set; }
        public string StructName { get; set; } = "";
        public ulong BaseAddress { get; set; }
        public List<MemoryInspectorEspField> Fields { get; set; } = new();
    }

    public class MemoryInspectorEspField
    {
        public string Name { get; set; } = "";
        public string Value { get; set; } = "";
        public uint Offset { get; set; }
    }

    #endregion
}
