# Observer Plugin Export (StoC Conventions)

This document outlines the semicolon-delimited format used for log entries exported by the Observer Plugin. All entries follow the basic structure:

`[MM:SS.ms] EVENT_IDENTIFIER;field1;field2;...`

Where `[MM:SS.ms]` is the instance time of the event.

---

## Agent Events (`agent_events.txt`)

| Event Identifier                | Description                                           | Format                                                             | Example                                                         |
| :------------------------------ | :---------------------------------------------------- | :----------------------------------------------------------------- | :-------------------------------------------------------------- |
| `GAME_SMSG_AGENT_MOVE_TO_POINT` | Agent started moving to a new point (Packet 0x29).    | `GAME_SMSG_AGENT_MOVE_TO_POINT;agent_id;x_coord;y_coord;plane`     | `[00:01.503] GAME_SMSG_AGENT_MOVE_TO_POINT;45;7984.00;3083.33;14` |

---

## Skill Events (`skill_events.txt`)

| Event Identifier         | Description                                                                 | Format                                                  | Example                                          |
| :----------------------- | :-------------------------------------------------------------------------- | :------------------------------------------------------ | :----------------------------------------------- |
| `SKILL_ACTIVATED`        | Normal skill activation started (ValueID `skill_activated`).                | `SKILL_ACTIVATED;skill_id;caster_id;target_id`          | `[00:05.210] SKILL_ACTIVATED;123;45;67`           |
| `INSTANT_SKILL_USED`     | Instant skill used (ValueID `instant_skill_activated`). Target = Caster.    | `INSTANT_SKILL_USED;skill_id;caster_id;target_id`       | `[00:13.676] INSTANT_SKILL_USED;1514;56;56`      |
| `SKILL_FINISHED`         | Normal skill finished (ValueID `skill_finished`).                           | `SKILL_FINISHED;caster_id;skill_id;target_id`           | `[00:06.810] SKILL_FINISHED;45;123;67`           |
| `SKILL_STOPPED`          | Normal skill stopped/cancelled (ValueID `skill_stopped`).                   | `SKILL_STOPPED;caster_id;skill_id;target_id`            | `[00:07.150] SKILL_STOPPED;45;123;67`            |

---

## Attack Skill Events (`attack_skill_events.txt`)

| Event Identifier           | Description                                                                     | Format                                                      | Example                                              |
| :------------------------- | :------------------------------------------------------------------------------ | :---------------------------------------------------------- | :--------------------------------------------------- |
| `ATTACK_SKILL_ACTIVATED`   | Attack skill activation started (ValueID `attack_skill_activated`).             | `ATTACK_SKILL_ACTIVATED;skill_id;caster_id;target_id`       | `[00:08.900] ATTACK_SKILL_ACTIVATED;456;78;90`       |
| `ATTACK_SKILL_FINISHED`    | Attack skill finished (ValueID `attack_skill_finished`).                        | `ATTACK_SKILL_FINISHED;caster_id;skill_id;target_id`        | `[00:10.500] ATTACK_SKILL_FINISHED;78;456;90`        |
| `ATTACK_SKILL_STOPPED`     | Attack skill stopped/cancelled (ValueID `attack_skill_stopped`).                | `ATTACK_SKILL_STOPPED;caster_id;skill_id;target_id`         | `[00:11.200] ATTACK_SKILL_STOPPED;78;456;90`         |

---

## Basic Attack Events (`basic_attack_events.txt`)

| Event Identifier    | Description                                                                           | Format                                          | Example                                  |
| :------------------ | :------------------------------------------------------------------------------------ | :---------------------------------------------- | :--------------------------------------- |
| `ATTACK_STARTED`    | Basic attack started (ValueID `attack_started`).                                      | `ATTACK_STARTED;caster_id;target_id`            | `[00:15.050] ATTACK_STARTED;11;22`       |
| `ATTACK_FINISHED`   | Basic attack finished (ValueID `melee_attack_finished`). `skill_id` is always 0.      | `ATTACK_FINISHED;caster_id;skill_id;target_id`  | `[00:16.150] ATTACK_FINISHED;11;0;22`    |
| `ATTACK_STOPPED`    | Basic attack stopped/cancelled (ValueID `attack_stopped`).                            | `ATTACK_STOPPED;caster_id;skill_id;target_id`           | `[00:16.500] ATTACK_STOPPED;11;0;22`           |

---

## Combat Events (`combat_events.txt`)

| Event Identifier | Description                                                                                                | Format                                                 | Example                                         |
| :--------------- | :--------------------------------------------------------------------------------------------------------- | :----------------------------------------------------- | :---------------------------------------------- |
| `DAMAGE`         | Damage dealt (ValueID `damage`, `critical`, or `armorignoring`). Type ID: 1=Normal, 2=Crit, 3=Armor-Ignore. | `DAMAGE;caster_id;target_id;value;damage_type_id`    | `[00:20.300] DAMAGE;45;67;105.000000;2`          |
| `INTERRUPTED`    | Skill interrupted (ValueID `interrupted`).                                                                 | `INTERRUPTED;caster_id;skill_id;target_id`             | `[00:25.100] INTERRUPTED;67;123;45`             |
| `KNOCKED_DOWN`   | Agent knocked down (ValueID `knocked_down`). `cause_id` is associated agent ID.                            | `KNOCKED_DOWN;target_id;cause_id`                    | `[00:30.750] KNOCKED_DOWN;90;78`                |

---

## Jumbo Messages (`jumbo_messages.txt`)

| Event Identifier            | Description                                                                      | Format                                          | Example                                                |
| :-------------------------- | :------------------------------------------------------------------------------- | :---------------------------------------------- | :----------------------------------------------------- |
| `GAME_SMSG_JUMBO_MESSAGE`   | Large screen message displayed (Packet 0x18F). See GWCA for `type`/`value` keys. | `GAME_SMSG_JUMBO_MESSAGE;type;value (Party Str)`| `[05:10.000] GAME_SMSG_JUMBO_MESSAGE;16;1 (Party 1)` |

---

## Unknown Events (`unknown_events.txt`)

Any log entry that does not match the known markers defined in `ObserverStoC.h` will be written to this file. The format will be the original timestamped log entry including any unrecognized marker or data.

**Example:** `[00:42.123] [UNKN] Some unexpected data` 