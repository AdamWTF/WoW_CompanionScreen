export function normalizeBasePath(value = process.env.NEXT_PUBLIC_BASE_PATH ?? "") {
  const trimmed = value.trim();
  if (!trimmed || trimmed === "/") return "";
  return `/${trimmed.replace(/^\/+|\/+$/g, "")}`;
}

export const basePath = normalizeBasePath();

export function withBasePath(path: string, prefix = basePath) {
  const absolutePath = path.startsWith("/") ? path : `/${path}`;
  return `${normalizeBasePath(prefix)}${absolutePath}`;
}
