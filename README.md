# Observer Plugin for GWToolbox++

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

## Overview

The Observer Plugin is a utility for [Guild Wars](https://www.guildwars.com/), designed to integrate with [GWToolbox++](https://github.com/gwdevhub/GWToolboxpp), that enables the replay of observed games. This plugin captures and preserves match data, allowing you to review past matches even after the original data is removed from the game client.

*Special thanks to the GWToolbox++ team for creating the plugin system that makes this possible!*

## Installation & Usage

To use the Observer Plugin:

1.  Download the latest `Observer.dll` from the [Releases page](https://github.com/SMN1337/ObserverPlugin/releases).
2.  Place the downloaded `Observer.dll` file into your GWToolbox++ `plugins` directory.

The plugin's source code is fully open source, available for audit and contributions.

## Building from Source

If you wish to contribute or build the plugin from the source code, follow these steps:

1.  **Set up the GWToolbox++ Development Environment:** Ensure you have a working build environment for GWToolbox++. Follow the setup guide in the [official GWToolbox++ repository](https://github.com/gwdevhub/GWToolboxpp/blob/master/README.md).
2.  **Clone the Repository:** Clone this Observer Plugin repository into the `plugins/` subdirectory of your GWToolbox++ source directory (e.g., `path/to/GWToolboxpp/plugins/ObserverPlugin`).
3.  **Update CMake Configuration:** Modify the `GWToolboxpp/cmake/gwtoolboxdll_plugins.cmake` file by adding the following configuration block to register the plugin with the build system:
```c++
add_library(Observer SHARED)
target_sources(Observer PRIVATE
    "plugins/Observer/ObserverStoC.cpp"
    "plugins/Observer/ObserverStoC.h"
    "plugins/Observer/ObserverPlugin.cpp"
    "plugins/Observer/ObserverPlugin.h"
    "plugins/Observer/dllmain.cpp"
)
target_include_directories(Observer PRIVATE
    "plugins/Observer"
)
target_link_libraries(Observer PRIVATE
    plugin_base
)

target_compile_options(Observer PRIVATE /wd4201 /wd4505)
target_compile_options(Observer PRIVATE /W4 /WX /Gy)
target_compile_options(Observer PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
target_compile_options(Observer PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
target_link_options(Observer PRIVATE /WX /OPT:REF /OPT:ICF /SAFESEH:NO)
target_link_options(Observer PRIVATE $<$<NOT:$<CONFIG:Debug>>:/LTCG /INCREMENTAL:NO>)
target_link_options(Observer PRIVATE $<$<CONFIG:Debug>:/IGNORE:4098 /OPT:NOREF /OPT:NOICF>)
target_link_options(Observer PRIVATE $<$<CONFIG:RelWithDebInfo>:/OPT:NOICF>)
set_target_properties(Observer PROPERTIES FOLDER "plugins/")

```

## Roadmap

This section outlines the planned development milestones for the Observer Plugin.

| Version | Goal                                                               | Status   |
| :------ | :----------------------------------------------------------------- | :------- |
| 0.1     | Setup handling for generic Server-to-Client (StoC) game events.    |   |
| 0.1.2   | Track agent movement during matches.                               |   |
| 0.2     | Implement initial match recognition logic.                         |   |
| 0.3     | Filter StoC packet logging to only record during active matches.   |   |
| 0.4     | Refine match start/end detection.                                  |   |
| 0.5     | Implement export functionality for all StoC packets within a match.|   |
| 0.6     | Capture and export Jumbo Messages (e.g., Victory, Morale Boost).   |   |
| 0.7     | Identify and record match metadata (Teams, Guilds, Map Info).      |   |
| 0.8     | Allow exporting either summary info or the full match data.        |   |
| 0.9     | *(Reserved for further development)*                               |   |
| **1.0** | **Stable Release:** Reliable export of comprehensive match data.   |   |

**Future Enhancements & Ideas:**
*   Generate detailed statistics:
    *   Knockdown counts.
    *   Skill usage breakdown (Casted, Cancelled, Interrupted).
*   Track Guild Lord damage dealt.
*   Record and potentially visualize morale boost history.