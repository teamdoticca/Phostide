import type { Metadata } from "next";
import { Outfit, IBM_Plex_Mono } from "next/font/google";
import "./globals.css";

const outfit = Outfit({
  variable: "--font-display",
  subsets: ["latin"],
});

const plexMono = IBM_Plex_Mono({
  variable: "--font-mono",
  weight: ["400", "500"],
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: "Phostide — OLED-safe taskbar themes",
  description:
    "Animated themes on the real Windows taskbar. Protect OLED panels without autohide or overlays.",
  metadataBase: new URL("https://phostide.doticca.com"),
  openGraph: {
    title: "Phostide",
    description:
      "OLED-safe animated themes on the real Windows taskbar — Explorer composition, not overlays.",
    url: "https://phostide.doticca.com",
    siteName: "Phostide",
    type: "website",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={`${outfit.variable} ${plexMono.variable} antialiased`}>
        {children}
      </body>
    </html>
  );
}
