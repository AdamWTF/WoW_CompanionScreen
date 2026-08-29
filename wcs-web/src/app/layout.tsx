import type { Metadata, Viewport } from "next";
import { withBasePath } from "@/deployment/basePath";
import "./globals.css";

export const metadata: Metadata = {
  title: "WoW Companion Screen",
  description: "Controller-friendly companion screen for World of Warcraft 3.3.5a",
  applicationName: "WoW Companion Screen",
  manifest: withBasePath("/manifest.webmanifest"),
  icons: { icon: withBasePath("/icons/wcs.svg"), apple: withBasePath("/icons/wcs.svg") },
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
