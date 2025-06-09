# Observer Plugin Export Conventions

## Overview

This document details the semicolon-delimited formats for various data files exported by the Observer Plugin. These files capture agent states and specific game events, primarily intended for analysis, replay development, or external tool integration. All timestamps represent the in-game instance time.

---

## Agent State Snapshots (`Agents/<agent_id>.txt.gz`)

These compressed files (`gzip`) contain periodic snapshots of agent states captured by the `ObserverLoop` thread every `kLoopInterval` (currently 200ms). Each file corresponds to a single agent, identified by `<agent_id>`.

**Format:** `[MM:SS.ms] x;y;z;rotation_angle;weapon_id;model_id;gadget_id;is_alive;is_dead;health_pct;is_knocked;max_hp;has_condition;has_deep_wound;has_bleeding;has_crippled;has_blind;has_poison;has_hex;has_degen_hex;has_enchantment;has_weapon_spell;is_holding;is_casting;skill_id`

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

**Important Notes:**
*   **Sampling:** Data is captured periodically (e.g., every 200ms). Very rapid state changes occurring between samples might not be reflected.
*   **Logging Threshold (Optimization):** To reduce file size and avoid redundant entries, a snapshot for a specific agent is logged **only if** it differs significantly from the previously logged snapshot for that same agent. The check involves two conditions:
    1.  **Position Change:** The squared Euclidean distance (`(Δx)² + (Δy)² + (Δz)²`) between the current and last position must be greater than or equal to `kDistanceThresholdSq` (currently 30.0f² = 900.0f game units squared).
    2.  **Data Change:** *OR*, any other field in the log line (excluding the timestamp and position coordinates) must have changed.
    If *both* the position change is below the threshold *and* the rest of the data is identical, the snapshot is skipped.

**Example (Living Agent):**
```
[01:23.456] 8000.1;3000.2;14.0;1.57;0;12345;0;1;0;0.853;0;500;1;0;1;0;0;1;1;0;1;0;0;1;123
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
| `ATTACK_FINISHED`   | Basic attack finished (ValueID `melee_attack_finished`). `skill_id`