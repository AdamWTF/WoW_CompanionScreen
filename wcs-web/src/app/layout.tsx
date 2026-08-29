import type { Metadata, Viewport } from "next";
import { withBasePath } from "@/deployment/basePath";
import "./globals.css";

export const metadata: Metadata = {
  title: "WoW Companion Screen",
  description: "Controller-friendly companion screen for World of Warcraft 3.3.5a",
  applicationName: "WoW Companion Screen",
  manifest: withBasePath("/manifest.webmanifest"),
  icons: {
    icon: [
      { url: withBasePath("/icons/favicon-32.png"), sizes: "32x32", type: "image/png" },
      { url: withBasePath("/icons/wcs-192.png"), sizes: "192x192", type: "image/png" },
    ],
    apple: [
      { url: withBasePath("/icons/apple-touch-icon-180.png"), sizes: "180x180", type: "image/png" },
    ],
  },
};

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  maximumScale: 1,
  userScalable: false,
  themeColor: "#100d09",
  viewportFit: "cover",
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
