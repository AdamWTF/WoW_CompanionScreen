const scope = new URL(self.registration.scope);
const scoped = (path = "") => new URL(path.replace(/^\/+/, ""), scope).toString();
const CACHE = `wcs-shell-v5-${scope.pathname}`;
const SHELL = [
  "",
  "manifest.webmanifest",
  "icons/favicon-32.png",
  "icons/apple-touch-icon-180.png",
  "icons/wcs-192.png",
  "icons/wcs-512.png",
  "icons/wcs-maskable-512.png",
  "icons/action-fallback.svg",
].map(scoped);

self.addEventListener("install", (event) => event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(SHELL)).then(() => self.skipWaiting())));
self.addEventListener("activate", (event) => event.waitUntil(caches.keys().then((keys) => Promise.all(keys.filter((key) => key !== CACHE).map((key) => caches.delete(key)))).then(() => self.clients.claim())));
self.addEventListener("fetch", (event) => {
  if (event.request.method !== "GET") return;
  if (event.request.mode === "navigate") {
    event.respondWith(fetch(event.request).then((response) => {
      if (response.ok) caches.open(CACHE).then((cache) => cache.put(event.request, response.clone()));
      return response;
    }).catch(() => caches.match(event.request).then((cached) => cached || caches.match(scoped()))));
    return;
  }
  event.respondWith(caches.match(event.request).then((cached) => cached || fetch(event.request).then((response) => {
    if (response.ok && new URL(event.request.url).origin === self.location.origin) caches.open(CACHE).then((cache) => cache.put(event.request, response.clone()));
    return response;
  }).catch(() => undefined)));
});
