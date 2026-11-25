#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <windows.h>

namespace phostide {

// Windows notifications (toasts) under the app's AppUserModelID. They stay in
// Notification Center and can carry buttons; the old Shell_NotifyIcon balloon
// only flashes for a few seconds and leaves no trace.

struct ToastButton {
  std::wstring label;
  std::wstring argument;  // handed back on activation
};

struct ToastContent {
  std::wstring title;
  std::wstring body;
  std::wstring launchArgument;  // clicking the toast body
  std::vector<ToastButton> buttons;
  std::wstring tag;             // same tag replaces the previous toast
  int expiryMinutes = 0;        // 0 = Windows default (kept for days)
  bool silentToCenter = false;  // no popup banner, straight to Notification Center
};

// Activation arguments.
inline constexpr wchar_t kToastArgInstallUpdate[] = L"install-update";
inline constexpr wchar_t kToastArgOpenUpdates[] = L"open-updates";
inline constexpr wchar_t kToastArgOpenSettings[] = L"open-settings";
inline constexpr wchar_t kToastArgDismiss[] = L"dismiss";

// Toast XML for ToastContent (ToastGeneric, escaped text, optional actions).
std::wstring BuildToastXml(const ToastContent& content);
std::wstring XmlEscape(std::wstring_view text);

// Toasts need the AUMID registered with Windows. Setup's Start menu shortcut
// carries it; portable zip / Scoop copies have no such shortcut and keep balloons.
bool ToastsAvailable();

// COM class Windows calls when a toast is clicked in Notification Center (an
// unpackaged app only gets clicks on the live popup otherwise). Setup puts it on
// the Start menu shortcut (AppUserModelToastActivatorCLSID) and registers
// HKCU\Software\Classes\CLSID\{...}\LocalServer32 so a click can also start
// Phostide. Keep in sync with installer/Phostide.iss.
inline constexpr GUID kToastActivatorClsid = {
    0x7d93b492, 0xa095, 0x4c16, {0x84, 0x9b, 0xa8, 0xe5, 0x0e, 0xcd, 0x54, 0xc4}};

// Activations are posted to `window` as `message` with lParam = new std::wstring*
// (the argument); the receiver deletes it. Registers the activator class object.
void ToastInit(HWND window, UINT message);

// Exit: clear our toasts and stop answering activations.
void ToastShutdown();

// Returns false if the toast could not be shown (caller may fall back).
bool ShowToast(const ToastContent& content);


}  // namespace phostide
