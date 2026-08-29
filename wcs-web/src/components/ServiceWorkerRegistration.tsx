"use client";

import { useEffect } from "react";
import { basePath, withBasePath } from "@/deployment/basePath";

export function ServiceWorkerRegistration() {
  useEffect(() => {
    if ("serviceWorker" in navigator && process.env.NODE_ENV === "production") {
      navigator.serviceWorker.register(withBasePath("/sw.js"), { scope: `${basePath}/` }).catch(() => undefined);
    }
  }, []);
  return null;
}
