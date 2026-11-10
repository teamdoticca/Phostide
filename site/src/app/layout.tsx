import type { Metadata } from "next";
import { Inter, IBM_Plex_Mono } from "next/font/google";
import "./globals.css";

const sans = Inter({
  variable: "--font-sans",
  subsets: ["latin"],
  weight: ["400", "500", "600", "700"],
});

const mono = IBM_Plex_Mono({
  variable: "--font-mono",
  weight: ["400", "500"],
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: {
    default: "Phostide — Animated themes for the Windows taskbar",
    template: "%s · Phostide",
  },
  description:
    "Phostide paints OLED-safe motion directly into the Windows taskbar background. Free, MIT, signed installer.",
  metadataBase: new URL("https://phostide.doticca.com"),
  applicationName: "Phostide",
  themeColor: [
    { media: "(prefers-color-scheme: dark)", color: "#000000" },
    { media: "(prefers-color-scheme: light)", color: "#000000" },
  ],
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
      "Animated themes, painted into your real Windows taskbar. Free · MIT · Windows 11.",
    url: "https://phostide.doticca.com",
    siteName: "Phostide",
    type: "website",
    images: [
      {
        url: "/og.png",
        width: 1200,
        height: 630,
        alt: "Phostide — animated themes for the Windows taskbar",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "Phostide",
    description:
      "Animated themes, painted into your real Windows taskbar. Free · MIT · Windows 11.",
    images: ["/og.png"],
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
      <body className={`${sans.variable} ${mono.variable} font-sans antialiased`}>
        {children}
      </body>
    </html>
  );
}
