// Tray balloons from OLED Care / updates must not interrupt fullscreen apps.
#include "../Host/CareNotice.h"
#include "TestHarness.h"

using namespace phostide;

TEST(AutomaticBalloon_OnlyWhenWindowsAcceptsNotifications) {
  CHECK(AutomaticBalloonAllowed(QUNS_ACCEPTS_NOTIFICATIONS));
  CHECK(AutomaticBalloonAllowed(QUNS_APP));
  CHECK(!AutomaticBalloonAllowed(QUNS_BUSY));  // fullscreen video / game
  CHECK(!AutomaticBalloonAllowed(QUNS_RUNNING_D3D_FULL_SCREEN));
  CHECK(!AutomaticBalloonAllowed(QUNS_PRESENTATION_MODE));
  CHECK(!AutomaticBalloonAllowed(QUNS_QUIET_TIME));
  CHECK(!AutomaticBalloonAllowed(QUNS_NOT_PRESENT));
}

TEST(CareDimNotice_OncePerIdleEpisode) {
  CHECK(CareDimNoticeDue(0, 1));
  CHECK(CareDimNoticeDue(0, 2));
  CHECK(!CareDimNoticeDue(1, 2));  // second display joining: no second balloon
  CHECK(!CareDimNoticeDue(2, 1));
  CHECK(!CareDimNoticeDue(0, 0));  // only a fullscreen-paused display went idle
}
