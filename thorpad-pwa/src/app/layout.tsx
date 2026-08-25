import type { Metadata, Viewport } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "ThorPad",
  description: "World of Warcraft companion for the AYN Thor",
  applicationName: "ThorPad",
  manifest: "/manifest.webmanifest",
  icons: { icon: "/icons/thorpad.svg", apple: "/icons/thorpad.svg" },
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
