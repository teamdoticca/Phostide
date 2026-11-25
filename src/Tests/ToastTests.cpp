// Windows notification XML for update / status toasts.
#include "../Host/Toast.cpp"
#include "TestHarness.h"

#include <winrt/Windows.Foundation.Collections.h>

using namespace phostide;

TEST(XmlEscape_EscapesMarkup) {
  CHECK(XmlEscape(L"a & b <c> \"d\" 'e'") ==
        L"a &amp; b &lt;c&gt; &quot;d&quot; &apos;e&apos;");
  CHECK(XmlEscape(L"Phostide v0.1.42") == L"Phostide v0.1.42");
}

TEST(BuildToastXml_UpdateWithButtons) {
  ToastContent t;
  t.title = L"Phostide v0.1.42 is available";
  t.body = L"Install now & restart";
  t.launchArgument = kToastArgOpenUpdates;
  t.buttons = {{L"Install update", kToastArgInstallUpdate}, {L"Later", kToastArgDismiss}};
  const std::wstring xml = BuildToastXml(t);
  CHECK(xml.find(L"launch=\"open-updates\"") != std::wstring::npos);
  CHECK(xml.find(L"<text>Install now &amp; restart</text>") != std::wstring::npos);
  CHECK(xml.find(L"content=\"Install update\" arguments=\"install-update\"") !=
        std::wstring::npos);
  CHECK(xml.find(L"arguments=\"dismiss\"") != std::wstring::npos);
  CHECK(xml.find(L"<audio silent=\"true\"/>") != std::wstring::npos);
}

TEST(BuildToastXml_ParsesAsXml) {
  // The exact XML Windows receives must load (bad escaping fails the whole toast).
  ToastContent t;
  t.title = L"<Phostide> & \"quotes\"";
  t.body = L"OLED Care on — 1 display(s) dimmed.";
  t.launchArgument = kToastArgOpenSettings;
  bool loaded = false;
  try {
    winrt::Windows::Data::Xml::Dom::XmlDocument doc;
    doc.LoadXml(BuildToastXml(t));
    loaded = doc.GetElementsByTagName(L"text").Size() == 2;
  } catch (...) {
  }
  CHECK(loaded);
}

TEST(BuildToastXml_NoActionsWithoutButtons) {
  ToastContent t;
  t.title = L"Phostide";
  t.body = L"Themes on.";
  CHECK(BuildToastXml(t).find(L"<actions>") == std::wstring::npos);
  CHECK(BuildToastXml(t).find(L"launch=") == std::wstring::npos);
}
