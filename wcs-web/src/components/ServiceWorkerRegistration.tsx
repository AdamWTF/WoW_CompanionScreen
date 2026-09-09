"use client";

import { useEffect } from "react";
import { basePath, withBasePath } from "@/deployment/basePath";

export function ServiceWorkerRegistration() {
  useEffect(() => {
    if (!("serviceWorker" in navigator)) {
      return;
    }

    const isProduction = process.env.NODE_ENV === "production";
    const isThorDev = process.env.NEXT_PUBLIC_WCS_THOR_DEV === "1";

    if (!isProduction && !isThorDev) {
      return;
    }

    const serviceWorkerPath = isThorDev
      ? withBasePath("/sw-dev.js")
      : withBasePath("/sw.js");

    navigator.serviceWorker
      .register(serviceWorkerPath, {
        scope: `${basePath}/`,
      })
      .then((registration) => {
        if (isThorDev) {
          console.log(
            "[WCS] Development service worker registered:",
            registration.scope,
          );
        }
      })
      .catch((error) => {
        console.error("[WCS] Service worker registration failed:", error);
      });
  }, []);

  return null;
}
