import type { Metadata } from "next";
import { Darker_Grotesque, Karla, IBM_Plex_Mono } from "next/font/google";
import "./globals.css";

const display = Darker_Grotesque({
  variable: "--font-display",
  subsets: ["latin"],
  weight: ["500", "600", "700", "800"],
});

const body = Karla({
  variable: "--font-body",
  subsets: ["latin"],
});

const mono = IBM_Plex_Mono({
  variable: "--font-mono",
  weight: ["400", "500"],
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: {
    default: "Phostide — OLED-safe taskbar themes",
    template: "%s · Phostide",
  },
  description:
    "Animated themes on the real Windows taskbar. Protect OLED panels without autohide or overlays.",
  metadataBase: new URL("https://phostide.doticca.com"),
  applicationName: "Phostide",
  icons: {
    icon: [
      { url: "/favicon.ico", sizes: "any" },
      { url: "/favicon-32.png", sizes: "32x32", type: "image/png" },
      { url: "/icon-192.png", sizes: "192x192", type: "image/png" },
      { url: "/icon-512.png", sizes: "512x512", type: "image/png" },
    ],
    apple: [{ url: "/apple-touch-icon.png", sizes: "180x180", type: "image/png" }],
  },
  manifest: "/manifest.webmanifest",
  openGraph: {
    title: "Phostide",
    description:
      "OLED-safe animated themes on the real Windows taskbar — Explorer composition, not overlays.",
    url: "https://phostide.doticca.com",
    siteName: "Phostide",
    type: "website",
    images: [
      {
        url: "/og-ai.jpg",
        width: 1200,
        height: 630,
        alt: "Phostide — OLED-safe taskbar themes",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "Phostide",
    description:
      "OLED-safe animated themes on the real Windows taskbar — Explorer composition, not overlays.",
    images: ["/og-ai.jpg"],
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" data-type="software" data-page-type="software">
      <head>
        <meta name="page-type" content="software" />
        <meta name="site-kind" content="software" />
      </head>
      <body
        className={`${display.variable} ${body.variable} ${mono.variable} antialiased`}
      >
        {children}
      </body>
    </html>
  );
}
