using LoneEftDmaRadar.UI.Misc;
using System;
using System.Collections.Concurrent;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using System.Threading;
using VmmSharpEx.Extensions.Input;

namespace LoneEftDmaRadar.UI.Hotkeys
{
    public sealed class HotkeyManagerViewModel : INotifyPropertyChanged
    {
        private static readonly ConcurrentDictionary<Win32VirtualKey, HotkeyAction> _hotkeys = new();
        internal static IReadOnlyDictionary<Win32VirtualKey, HotkeyAction> Hotkeys => _hotkeys;
        private static readonly ConcurrentBag<HotkeyManagerViewModel> _instances = new();
        private static int _suspendCount;

        static HotkeyManagerViewModel()
        {
            LoadConfigHotkeys();
        }

        public ObservableCollection<HotkeyBindingEntry> Bindings { get; }
        public HotkeyBindingEntry ListeningEntry { get; private set; }

        public HotkeyManagerViewModel()
        {
            Bindings = new ObservableCollection<HotkeyBindingEntry>();
            EnsureControllersRegistered();
            LoadConfigHotkeys();
            RefreshBindings();
            _instances.Add(this);
        }

        private bool _skipNextMouseCapture;

        public void BeginListening(HotkeyBindingEntry entry, bool skipNextMouseCapture = false)
        {
            if (entry is null)
                return;

            CancelListening();
            ListeningEntry = entry;
            ListeningEntry.IsListening = true;
            _skipNextMouseCapture = skipNextMouseCapture;
        }

        public void AssignKey(Key key)
        {
            try
            {
                if (ListeningEntry is null)
                    return;

                var vkCode = KeyInterop.VirtualKeyFromKey(key);
                AssignVirtualKey(vkCode);
            }
            catch (Exception ex)
            {
                DebugLogger.LogDebug($"[HotkeyManager] Failed to assign key: {ex}");
            }
            finally
            {
                CancelListening();
            }
        }

        public void AssignVirtualKey(int vkCode)
        {
            if (ListeningEntry is null)
                return;

            if (_skipNextMouseCapture)
            {
                _skipNextMouseCapture = false;
                return;
            }

            var vk = (Win32VirtualKey)vkCode;

            RemoveBindingForAction(ListeningEntry.ActionName);

            if (_hotkeys.TryRemove(vk, out _))
            {
                App.Config.Hotkeys.TryRemove(vk, out _);
            }

            var action = new HotkeyAction(ListeningEntry.ActionName);
            _hotkeys[vk] = action;
            App.Config.Hotkeys[vk] = action.Name;

            ListeningEntry.Key = vk;
            RefreshBindings();

            // Stop listening after successfully assigning the key
            CancelListening();
        }

        public void ClearListening()
        {
            if (ListeningEntry is null)
                return;

            CancelListening();
        }

        public void ClearBinding(HotkeyBindingEntry entry)
        {
            if (entry is null)
                return;

            RemoveBindingForAction(entry.ActionName);
            entry.Key = null;

            if (ListeningEntry == entry)
            {
                CancelListening();
            }

            RefreshBindings();
        }

        public void CancelListening()
        {
            if (ListeningEntry is not null)
            {
                ListeningEntry.IsListening = false;
                ListeningEntry = null;
                _skipNextMouseCapture = false;
            }
        }

        private void RemoveBindingForAction(string actionName)
        {
            var existing = _hotkeys.FirstOrDefault(kvp => kvp.Value.Name.Equals(actionName, StringComparison.Ordinal));
            if (!existing.Equals(default(KeyValuePair<Win32VirtualKey, HotkeyAction>)))
            {
                _hotkeys.TryRemove(existing.Key, out _);
                App.Config.Hotkeys.TryRemove(existing.Key, out _);
            }

            var entry = Bindings.FirstOrDefault(b => b.ActionName.Equals(actionName, StringComparison.Ordinal));
            if (entry is not null)
            {
                entry.Key = null;
            }
        }

        private void RefreshBindings()
        {
            Bindings.Clear();
            EnsureControllersRegistered();

            foreach (var controller in HotkeyAction.Controllers.OrderBy(c => c.Name))
            {
                Win32VirtualKey? key = null;
                var existing = _hotkeys.FirstOrDefault(kvp => kvp.Value.Name.Equals(controller.Name, StringComparison.Ordinal));
                if (!existing.Equals(default(KeyValuePair<Win32VirtualKey, HotkeyAction>)))
                {
                    key = existing.Key;
                }
                Bindings.Add(new HotkeyBindingEntry(controller, key));
            }

            OnPropertyChanged(nameof(Bindings));
        }

        private static void EnsureControllersRegistered()
        {
            if (HotkeyAction.Controllers.Any())
                return;

            // try to use existing MainWindow instance on UI thread only
            var dispatcher = Application.Current?.Dispatcher;
            if (dispatcher is null)
                return;

            if (dispatcher.CheckAccess())
            {
                MainWindowViewModel.EnsureHotkeysRegisteredStatic();
            }
            else
            {
                dispatcher.BeginInvoke(new Action(MainWindowViewModel.EnsureHotkeysRegisteredStatic));
                return;
            }

            // If still empty, and MainWindow exists, force registration now
            if (!HotkeyAction.Controllers.Any() && Application.Current?.MainWindow is MainWindow mw)
            {
                mw.ViewModel?.EnsureHotkeysRegistered();
            }

            if (HotkeyAction.Controllers.Any())
            {
                NotifyControllersRegistered();
            }
        }

        private static void LoadConfigHotkeys()
        {
            if (_hotkeys.Count > 0)
                return;

            foreach (var kvp in App.Config.Hotkeys)
            {
                var action = new HotkeyAction(kvp.Value);
                _hotkeys.TryAdd(kvp.Key, action);
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string prop) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(prop));

        internal static void NotifyControllersRegistered()
        {
            foreach (var vm in _instances)
            {
                vm.RefreshBindings();
            }
        }

        public static void SuspendHotkeys() => Interlocked.Increment(ref _suspendCount);

        public static void ResumeHotkeys()
        {
            while (true)
            {
                int current = Volatile.Read(ref _suspendCount);
                if (current <= 0) return;
                if (Interlocked.CompareExchange(ref _suspendCount, current - 1, current) == current)
                    return;
            }
        }

        internal static bool HotkeysSuspended => Volatile.Read(ref _suspendCount) > 0;
    }
}
