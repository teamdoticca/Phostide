#pragma once

#include <cstdint>

#include <windows.h>
#include <shellapi.h>

namespace phostide {

// Automatic tray balloons (OLED Care, update available, timed pause over) are only
// shown when Windows says the user accepts notifications. A balloon over a
// fullscreen video or game makes it briefly leave direct (independent flip)
// presentation, which shows as a hitch.
inline bool AutomaticBalloonAllowed(QUERY_USER_NOTIFICATION_STATE state) {
  return state == QUNS_ACCEPTS_NOTIFICATIONS || state == QUNS_APP;
}

// "OLED Care on" is announced once per idle episode: when the number of displays
// actually dimming goes from none to some. Fullscreen-paused or turned-off
// displays never count (nothing dims there).
inline bool CareDimNoticeDue(uint32_t dimmedBefore, uint32_t dimmedNow) {
  return dimmedBefore == 0 && dimmedNow > 0;
}

}  // namespace phostide
