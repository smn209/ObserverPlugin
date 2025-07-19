# Observer Plugin Export Conventions

## Overview

This document details the semicolon-delimited formats for various data files exported by the Observer Plugin. These files capture agent states and specific game events, primarily intended for analysis, replay development, or external tool integration. All timestamps represent the in-game instance time.

---

## Agent State Snapshots (`Agents/<agent_id>.txt.gz`)

These compressed files (`gzip`) contain periodic snapshots of agent states captured by the `ObserverLoop` thread every `kLoopInterval` (currently 200ms). Each file corresponds to a single agent, identified by `<agent_id>`.

**Format:** `[MM:SS.ms] x;y;z;rotation_angle;weapon_id;model_id;gadget_id;is_alive;is_dead;health_pct;is_knocked;max_hp;has_condition;has_deep_wound;has_bleeding;has_crippled;has_blind;has_poison;has_hex;has_degen_hex;has_enchantment;has_weapon_spell;is_holding;is_casting;skill_id;weapon_item_type;offhand_item_type;weapon_item_id;offhand_item_id;move_x;move_y;visual_effects;team_id;weapon_type;weapon_attack_speed;attack_speed_modifier;dagger_status;hp_pips;model_state;animation_code;animation_id;animation_speed;animation_type;in_spirit_range;agent_model_type;item_id;item_extra_type;gadget_extra_type`

| Field Name         | Data Type | Description                                                                 | Notes                                                   |
| :----------------- | :-------- | :-------------------------------------------------------------------------- | :------------------------------------------------------ |
| `timestamp`        | String    | Instance time `[MM:SS.ms]`                                                  | Prepended by `RunLoop`, not `GetAgentLogLine`         |
| `x`                | Float     | Agent's X coordinate                                                        |                                                         |
| `y`                | Float     | Agent's Y coordinate                                                        |                                                         |
| `z`                | Float     | Agent's Z coordinate (plane)                                                | From `agent->z`                                         |
| `rotation_angle`   | Float     | Agent's rotation angle (radians)                                            | From `agent->rotation_angle`                            |
| `weapon_id`        | UInt32    | ID of the equipped weapon                                                   | **Caution:** Currently always 0 (placeholder)           |
| `model_id`         | UInt32    | Model/character ID                                                          | From `living->player_number` for living agents          |
| `gadget_id`        | UInt32    | Gadget ID for gadget-type agents                                            | From `gadget->gadget_id`, 0 for non-gadget agents       |
| `is_alive`         | Boolean   | `1` if alive, `0` if dead                                                   | Based on `!living->GetIsDead()`                         |
| `is_dead`          | Boolean   | `1` if dead, `0` if alive                                                   | Based on `living->GetIsDead()`                          |
| `health_pct`       | Float     | Current health percentage (0.0 to 1.0)                                      | From `living->hp`, formatted to 3 decimal places        |
| `is_knocked`       | Boolean   | `1` if knocked down, `0` otherwise                                          | Based on `living->GetIsKnockedDown()`                   |
| `max_hp`           | UInt32    | Maximum health points                                                       | From `living->max_hp`                                   |
| `has_condition`    | Boolean   | `1` if under any condition effect, `0` otherwise                            | Based on `living->GetIsConditioned()`                   |
| `has_deep_wound`   | Boolean   | `1` if has Deep Wound, `0` otherwise                                        | Based on `living->GetIsDeepWounded()`                   |
| `has_bleeding`     | Boolean   | `1` if has Bleeding, `0` otherwise                                          | Based on `living->GetIsBleeding()`                      |
| `has_crippled`     | Boolean   | `1` if has Crippled, `0` otherwise                                          | Based on `living->GetIsCrippled()`                      |
| `has_blind`        | Boolean   | `1` if has Blind, `0` otherwise                                             | **Caution:** Currently always 0 (placeholder)           |
| `has_poison`       | Boolean   | `1` if has Poison, `0` otherwise                                            | Based on `living->GetIsPoisoned()`                      |
| `has_hex`          | Boolean   | `1` if under any hex effect, `0` otherwise                                  | Based on `living->GetIsHexed()`                         |
| `has_degen_hex`    | Boolean   | `1` if under any degeneration hex effect, `0` otherwise                     | Based on `living->GetIsDegenHexed()`                    |
| `has_enchantment`  | Boolean   | `1` if under any enchantment effect, `0` otherwise                          | Based on `living->GetIsEnchanted()`                     |
| `has_weapon_spell` | Boolean   | `1` if under any weapon spell effect, `0` otherwise                         | Based on `living->GetIsWeaponSpelled()`                 |
| `is_holding`       | Boolean   | `1` if agent is holding an item/flag, `0` otherwise                         | Based on `(living->model_state & 0x400) != 0`         |
| `is_casting`       | Boolean   | `1` if currently casting a skill, `0` otherwise                             | Based on `living->GetIsCasting()`                       |
| `skill_id`         | UInt32    | ID of the skill currently being cast (`0` if not casting)                 | From `living->skill`                                    |
| `weapon_item_type` | UInt8     | Type ID of equipped weapon item                                           | From `living->weapon_item_type`                         |
| `offhand_item_type`| UInt8     | Type ID of equipped offhand item                                          | From `living->offhand_item_type`                        |
| `weapon_item_id`   | UInt16    | Item ID of equipped weapon                                                | From `living->weapon_item_id`                           |
| `offhand_item_id`  | UInt16    | Item ID of equipped offhand item                                          | From `living->offhand_item_id`                          |
| `move_x`           | Float     | Velocity on X axis (units per second)                                     | From `agent->velocity.x`                                |
| `move_y`           | Float     | Velocity on Y axis (units per second)                                     | From `agent->velocity.y`                                |
| `visual_effects`   | UInt16    | Number of visual effects on agent                                         | From `agent->visual_effects`                            |
| `team_id`          | UInt8     | Team ID: 0=None, 1=Blue, 2=Red, 3=Yellow                                 | From `living->team_id`                                  |
| `weapon_type`      | UInt16    | Weapon type: 1=bow, 2=axe, 3=hammer, 4=daggers, etc.                    | From `living->weapon_type`                              |
| `weapon_attack_speed` | Float  | Base attack speed of equipped weapon                                     | From `living->weapon_attack_speed`                      |
| `attack_speed_modifier` | Float | Attack speed modifier (0.67 = 33% increase)                            | From `living->attack_speed_modifier`                    |
| `dagger_status`    | UInt8     | Dagger attack status: 0x1=lead, 0x2=offhand, 0x3=dual                   | From `living->dagger_status`                            |
| `hp_pips`          | Float     | Health regeneration/degeneration pips                                    | From `living->hp_pips`                                  |
| `model_state`      | UInt32    | Model state: different values for different states                       | From `living->model_state`                              |
| `animation_code`   | UInt32    | Animation code                                                           | From `living->animation_code`                           |
| `animation_id`     | UInt32    | Current animation ID                                                     | From `living->animation_id`                             |
| `animation_speed`  | Float     | Speed of current animation                                               | From `living->animation_speed`                          |
| `animation_type`   | Float     | Type of animation                                                        | From `living->animation_type`                           |
| `in_spirit_range`  | UInt32    | Whether agent is in spirit range                                         | From `living->in_spirit_range`                          |
| `agent_model_type` | UInt16    | Model type: 0x3000=Player, 0x2000=NPC                                   | From `living->agent_model_type`                         |
| `item_id`          | UInt32    | Item ID (for AgentItem types)                                           | From `item->item_id`                                    |
| `item_extra_type`  | UInt32    | Extra type for items                                                     | From `item->extra_type`                                 |
| `gadget_extra_type`| UInt32    | Extra type for gadgets                                                   | From `gadget->extra_type`                               |

**Important Notes:**
*   **Sampling:** Data is captured periodically (e.g., every 200ms). Very rapid state changes occurring between samples might not be reflected.
*   **Logging Threshold (Optimization):** To reduce file size and avoid redundant entries, a snapshot for a specific agent is logged **only if** it differs significantly from the previously logged snapshot for that same agent. The check involves two conditions:
    1.  **Position Change:** The squared Euclidean distance (`(Δx)² + (Δy)² + (Δz)²`) between the current and last position must be greater than or equal to `kDistanceThresholdSq` (currently 30.0f² = 900.0f game units squared).
    2.  **Data Change:** *OR*, any other field in the log line (excluding the timestamp and position coordinates) must have changed.
    If *both* the position change is below the threshold *and* the rest of the data is identical, the snapshot is skipped.

**Example (Living Agent):**
```
[01:23.456] 8000.1;3000.2;14.0;1.57;0;12345;0;1;0;0.853;0;500;1;0;1;0;0;1;1;0;1;0;0;1;123;2;0;456;0;0.0;0.0;1;1;2;1.33;1.0;0;-2.5;68;96;1088;1.0;0.0;0;0x3000;0;0;0
```

---

## Server-to-Client (StoC) Packet Events

The following sections detail events captured by hooking into specific Server-to-Client (StoC) game packets or derived game state values via GWCA. These events are logged closer to real-time compared to the Agent State Snapshots.

---

## Agent Movement Events (StoC, `agent_events.txt`)

| Event Identifier                | Description                                           | Format                                                             | Example                                                         |
| :------------------------------ | :---------------------------------------------------- | :----------------------------------------------------------------- | :-------------------------------------------------------------- |
| `GAME_SMSG_AGENT_MOVE_TO_POINT` | Agent started moving to a new point (Packet 0x29).    | `GAME_SMSG_AGENT_MOVE_TO_POINT;agent_id;x_coord;y_coord;plane`     | `[00:01.503] GAME_SMSG_AGENT_MOVE_TO_POINT;45;7984.00;3083.33;14` |

---

## Skill Activation Events (StoC, `skill_events.txt`)

| Event Identifier         | Description                                                                 | Format                                                  | Example                                          |
| :----------------------- | :-------------------------------------------------------------------------- | :------------------------------------------------------ | :----------------------------------------------- |
| `SKILL_ACTIVATED`        | Normal skill activation started (ValueID `skill_activated`).                | `SKILL_ACTIVATED;skill_id;caster_id;target_id`          | `[00:05.210] SKILL_ACTIVATED;123;45;67`           |
| `INSTANT_SKILL_USED`     | Instant skill used (ValueID `instant_skill_activated`). Target = Caster.    | `INSTANT_SKILL_USED;skill_id;caster_id;target_id`       | `[00:13.676] INSTANT_SKILL_USED;1514;56;56`      |
| `SKILL_FINISHED`         | Normal skill finished (ValueID `skill_finished`).                           | `SKILL_FINISHED;caster_id;skill_id;target_id`           | `[00:06.810] SKILL_FINISHED;45;123;67`           |
| `SKILL_STOPPED`          | Normal skill stopped/cancelled (ValueID `skill_stopped`).                   | `SKILL_STOPPED;caster_id;skill_id;target_id`            | `[00:07.150] SKILL_STOPPED;45;123;67`            |

---

## Attack Skill Events (StoC, `attack_skill_events.txt`)

| Event Identifier           | Description                                                                     | Format                                                      | Example                                              |
| :------------------------- | :------------------------------------------------------------------------------ | :---------------------------------------------------------- | :--------------------------------------------------- |
| `ATTACK_SKILL_ACTIVATED`   | Attack skill activation started (ValueID `attack_skill_activated`).             | `ATTACK_SKILL_ACTIVATED;skill_id;caster_id;target_id`       | `[00:08.900] ATTACK_SKILL_ACTIVATED;456;78;90`       |
| `ATTACK_SKILL_FINISHED`    | Attack skill finished (ValueID `attack_skill_finished`).                        | `ATTACK_SKILL_FINISHED;caster_id;skill_id;target_id`        | `[00:10.500] ATTACK_SKILL_FINISHED;78;456;90`        |
| `ATTACK_SKILL_STOPPED`     | Attack skill stopped/cancelled (ValueID `attack_skill_stopped`).                | `ATTACK_SKILL_STOPPED;caster_id;skill_id;target_id`         | `[00:11.200] ATTACK_SKILL_STOPPED;78;456;90`         |

---

## Basic Attack Events (StoC, `basic_attack_events.txt`)

| Event Identifier    | Description                                                                           | Format                                          | Example                                  |
| :------------------ | :------------------------------------------------------------------------------------ | :---------------------------------------------- | :--------------------------------------- |
| `ATTACK_STARTED`    | Basic attack started (ValueID `attack_started`).                                      | `ATTACK_STARTED;caster_id;target_id`            | `[00:15.050] ATTACK_STARTED;11;22`       |
| `ATTACK_FINISHED`   | Basic attack finished (ValueID `melee_attack_finished`).                               | `ATTACK_FINISHED;caster_id;skill_id;target_id`  | `[00:16.200] ATTACK_FINISHED;11;0;22`     |
| `ATTACK_STOPPED`    | Basic attack stopped/cancelled (ValueID `attack_stopped`).                             | `ATTACK_STOPPED;caster_id;skill_id;target_id`   | `[00:17.100] ATTACK_STOPPED;11;0;22`      |

---

## Combat Events (StoC, `combat_events.txt`)

| Event Identifier | Description                                                                        | Format                                        | Example                                     |
| :--------------- | :--------------------------------------------------------------------------------- | :-------------------------------------------- | :------------------------------------------ |
| `DAMAGE`         | Damage dealt between agents (ValueID `damage`, `critical`, `armorignoring`).      | `DAMAGE;caster_id;target_id;value;damage_type`| `[00:20.150] DAMAGE;34;56;-120.500000;1`   |
| `KNOCKED_DOWN`   | Agent knocked down (ValueID `knocked_down`).                                       | `KNOCKED_DOWN;target_id;cause_id`             | `[00:22.800] KNOCKED_DOWN;78;90`           |
| `INTERRUPTED`    | Agent interrupted (ValueID `interrupted`).                                         | `INTERRUPTED;caster_id;skill_id;target_id`    | `[00:25.300] INTERRUPTED;45;123;67`        |

---

## Lord Events (StoC, `lord_events.txt`)

| Event Identifier | Description                                                                        | Format                                                     | Example                                           |
| :--------------- | :--------------------------------------------------------------------------------- | :--------------------------------------------------------- | :------------------------------------------------ |
| `LORD_DAMAGE`    | Damage dealt specifically to Guild Lords (player_number == 170).                  | `LORD_DAMAGE;caster_id;target_id;value;damage_type;attacking_team;damage;damage_before;damage_after` | `[00:30.750] LORD_DAMAGE;123;456;-85.250000;1;2;142;1250;1392` |

**Notes:**
- Only damage dealt to Guild Lords (agents with `player_number == 170` and `team_id == 1` or `2`) is logged.
- `attacking_team` represents the team dealing damage (opposite of the lord's team: 1=Blue, 2=Red).
- `damage_type`: 1=normal damage, 2=critical damage, 3=armor-ignoring damage.
- `damage`: Calculated damage value (`-value * max_hp`, rounded to long).
- `damage_before`: Total lord damage for attacking team before this hit.
- `damage_after`: Total lord damage for attacking team after this hit.
- This data is also used to update the Lord Damage Counter window via `AddTeamLordDamage()`.

---

## Agent Movement Events (StoC, `agent_events.txt`)

| Event Identifier                | Description                                           | Format                                                             | Example                                                         |
| :------------------------------ | :---------------------------------------------------- | :----------------------------------------------------------------- | :-------------------------------------------------------------- |
| `GAME_SMSG_AGENT_MOVE_TO_POINT` | Agent started moving to a new point (Packet 0x29).    | `GAME_SMSG_AGENT_MOVE_TO_POINT;agent_id;x_coord;y_coord;plane`     | `[00:01.503] GAME_SMSG_AGENT_MOVE_TO_POINT;45;7984.00;3083.33;14` |

---

## Jumbo Messages (StoC, `jumbo_messages.txt`)

These are server-wide announcements that appear in the center of the screen during matches.

| Message Type                    | Description                                           | Format                                        | Example                                    |
| :------------------------------ | :---------------------------------------------------- | :-------------------------------------------- | :----------------------------------------- |
| `BASE_UNDER_ATTACK`             | Base under attack announcement.                       | `[JMB] {message} ({party_id})`                | `[00:45.200] [JMB] Base under attack! (1)` |
| `GUILD_LORD_UNDER_ATTACK`       | Guild Lord under attack announcement.                 | `[JMB] {message} ({party_id})`                | `[01:30.800] [JMB] Guild Lord under attack! (2)` |
| `CAPTURED_SHRINE`               | Shrine captured announcement.                         | `[JMB] {message} ({party_id})`                | `[02:15.400] [JMB] Shrine captured! (1)`  |
| `CAPTURED_TOWER`                | Tower captured announcement.                          | `[JMB] {message} ({party_id})`                | `[03:00.600] [JMB] Tower captured! (2)`   |
| `PARTY_DEFEATED`                | Party defeated announcement.                          | `[JMB] {message} ({party_id})`                | `[04:30.200] [JMB] Party defeated! (1)`   |
| `MORALE_BOOST`                  | Morale boost announcement.                            | `[JMB] {message} ({party_id})`                | `[05:45.800] [JMB] Morale boost! (2)`     |
| `VICTORY`                       | Victory announcement.                                 | `[JMB] {message} ({party_id})`                | `[12:30.000] [JMB] Victory! (1)`          |
| `FLAWLESS_VICTORY`              | Flawless victory announcement.                        | `[JMB] {message} ({party_id})`                | `[08:15.400] [JMB] Flawless Victory! (2)` |