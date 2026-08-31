# Native controller support

WoW Companion Screen handles controller input inside the 32-bit WoW 3.3.5a client. It isn't a keyboard mapper, so you don't need JoyToKey or another remapping tool running in the background.

Movement, camera control, actions, Jump, Smart Interact and supported menu navigation all go straight through the game.

## Default controls

| Control | What it does |
|---|---|
| Left stick | Move and strafe, including diagonals |
| Right stick | Turn and move the camera |
| L1 / left shoulder | Previous hostile target |
| R1 / right shoulder | Next hostile target |
| L3 / left-stick click | Cycle WoW's camera views |
| R3 / right-stick click | Next friendly target |
| Start / Menu / Options | Open or close the WoW game menu |
| Select / View / Share | Open or close the world map |
| L2 and R2 | Switch between the Default, L2, R2 and L2+R2 action layers |
| D-pad and face buttons | Use one of eight actions in the current layer |

Face buttons are named by position so the guide works across different controllers:

* **South:** A or Cross
* **East:** B or Circle
* **West:** X or Square
* **North:** Y or Triangle

The 32 controller actions use WoW's existing action slots:

| Layer | WoW action slots |
|---|---|
| Default | The live ActionButton 1–8 slots for the current page/form |
| L2 | 9–12 and 49–52 |
| R2 | 53–60 |
| L2 + R2 | 61–68 |

Within each layer, the order is D-pad Up, Down, Left and Right, followed by South, East, West and North.

The in-game overlay shows the active layer and its assignments. Separate trigger press and release thresholds stop the layer from flickering when a trigger is held near its activation point.

## Change the mappings

Open WoW Companion Screen using the minimap button or:

```text
/wcs
```

On **Controller Mapping**, choose a layer and drag a spell, item, macro or existing WoW action into any of its eight slots. Right-click a slot to clear it.

WoW only allows protected action changes out of combat.

## Jump and Smart Interact

The **System Actions** palette contains two actions that can be dragged onto the controller like spells:

* **Jump** uses WoW's real jump control. It also handles swimming and flying ascent; it does not fake a Space key press.
* **Interact** runs one Smart Interact attempt. It keeps a valid interactable target, avoids replacing targets automatically during combat, and otherwise looks for a suitable NPC or game object based on distance and camera direction.

By default, Smart Interact searches up to 12 yards away and 60 degrees either side of the camera direction.

If it picks the wrong target, enable `SmartInteractDebug=1` in `Extensions\wcs-gamepad\wcs-gamepad.cfg` and check the native log for its candidate scores.

## Navigate WoW's interface

Controller UI navigation can be switched on or off separately under **Display & Connection**.

When a supported panel is open and you're out of combat:

* the D-pad moves the highlight
* **South** confirms or activates the highlighted item by default
* **East** goes back by default
* movement, camera, targeting and action inputs pause until you close the panel or enter combat

The **Menu Confirm** option under **Display & Connection** reverses South and East for menus only. It never changes combat action assignments.

Supported panels include the game menu, confirmation popups, gossip and quest windows, merchants, bags and WoW Companion Screen's own settings.

Using a highlighted bag item follows WoW's normal right-click behaviour.

## Controllers and backends

`Backend=Auto` tries the available backends in the most sensible order.

* **XInput** works without any extra runtime and covers Xbox-compatible controllers.
* **SDL3 Gamepad** supports mapped PlayStation and handheld controllers when a 32-bit SDL3 runtime is placed beside `Wow.exe`.
* **SDL3 Joystick** is a diagnostic mode for devices that do not yet have a verified mapping in `gamecontrollerdb.txt`.

Button glyphs can be detected automatically or forced to Xbox, PlayStation or AYN Thor style.

On supported DualShock controllers, the touchpad moves the cursor. A normal click produces a left-click, and a two-finger click produces a right-click.

## Advanced settings

Copy:

```text
Extensions\wcs-gamepad\wcs-gamepad.cfg.example
```

to:

```text
Extensions\wcs-gamepad\wcs-gamepad.cfg
```

The file lets you adjust deadzones, camera sensitivity, response curve, Y-axis inversion, trigger thresholds, polling rate and targeting key combinations.

Press **F9** to open the native diagnostics. Use `/wcs` for normal controller mappings, glyphs and display settings.

## When controller input is active

The extension can detect controllers on the login, realm, character-selection and loading screens, but it will not send gameplay input there.

Input only becomes active once your character is in the rendered world. Disconnecting the controller, changing backend, switching windows, loading into another area or moving between UI and gameplay modes releases any held input. The controls must return to neutral before input resumes, which helps prevent stuck movement or accidental actions.
