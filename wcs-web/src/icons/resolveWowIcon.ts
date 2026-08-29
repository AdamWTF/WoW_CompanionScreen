import { withBasePath } from "@/deployment/basePath";

export function resolveWowIcon(iconPath: string) {
  const filename = iconPath
    .replace(/\\/g, "/")
    .split("/")
    .filter(Boolean)
    .at(-1)
    ?.toLowerCase()
    .replace(/[^a-z0-9_-]/g, "");
  return filename ? withBasePath(`/assets/wow-icons/${filename}.webp`) : withBasePath("/icons/action-fallback.svg");
}
