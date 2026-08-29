"use client";

import { CompanionScreenProvider } from "@/state/CompanionScreenContext";
import { CompanionScreenShell } from "@/components/CompanionScreenShell";
import { ServiceWorkerRegistration } from "@/components/ServiceWorkerRegistration";

export default function Home() {
  return (
    <CompanionScreenProvider>
      <ServiceWorkerRegistration />
      <CompanionScreenShell />
    </CompanionScreenProvider>
  );
}
