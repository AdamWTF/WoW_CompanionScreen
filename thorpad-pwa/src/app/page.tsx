"use client";

import { ThorPadProvider } from "@/state/ThorPadContext";
import { ThorShell } from "@/components/ThorShell";
import { ServiceWorkerRegistration } from "@/components/ServiceWorkerRegistration";

export default function Home() {
  return (
    <ThorPadProvider>
      <ServiceWorkerRegistration />
      <ThorShell />
    </ThorPadProvider>
  );
}
