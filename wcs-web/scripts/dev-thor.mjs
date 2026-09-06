import { spawn, spawnSync } from "node:child_process";

console.log("Checking for connected Android device...");

const devices = spawnSync("adb", ["devices"], {
  encoding: "utf8",
});

if (devices.error) {
  console.error("ADB was not found.");
  process.exit(1);
}

const connected = devices.stdout
  .split("\n")
  .some((line) => /\tdevice$/.test(line.trim()));

if (!connected) {
  console.error("No authorised Android device found.");
  console.error("Connect the Thor and make sure USB debugging is enabled.");
  process.exit(1);
}

console.log("Thor detected.");
console.log("Creating ADB reverse tunnel...");

const reverse = spawnSync("adb", ["reverse", "tcp:3000", "tcp:3000"], {
  stdio: "inherit",
});

if (reverse.status !== 0) {
  console.error("Failed to create ADB reverse tunnel.");
  process.exit(1);
}

console.log("");
console.log("WCS Thor dev environment ready:");
console.log("  http://localhost:3000/?demo");
console.log("");

const next = spawn("npx", ["next", "dev"], {
  stdio: "inherit",
  shell: process.platform === "win32",
  env: {
    ...process.env,
    NEXT_PUBLIC_WCS_THOR_DEV: "1",
  },
});

next.on("exit", (code) => {
  process.exit(code ?? 0);
});
