# Table of Contents

- [Table of Contents](#table-of-contents)
- [Building from Source](#building-from-source)
- [Build Configurations](#build-configurations)
  - [Debug Build](#debug-build)
  - [Release Build](#release-build)
- [Debug Windows](#debug-windows)
  - [Main Window](#main-window)
  - [Debug Toggles](#debug-toggles)
  - [Observer Plugin - Capture Status](#observer-plugin---capture-status)
  - [Observer Plugin - Live Party Info](#observer-plugin---live-party-info)
  - [Observer Plugin - Live Guild Info](#observer-plugin---live-guild-info)
  - [Observer Plugin - Available Matches](#observer-plugin---available-matches)
  - [Observer Plugin - StoC Log Options](#observer-plugin---stoc-log-options)

# Building from Source
If you want to contribute or build the Observer Plugin from source, follow these steps:

1. **Set Up the GWToolbox++ Development Environment:**  
    Make sure you have a working build environment for GWToolbox++. Refer to the [GWToolbox++ setup guide](https://github.com/gwdevhub/GWToolboxpp/blob/master/README.md) for instructions.

2. **Clone the Repository:**  
    Clone this Observer Plugin repository into the `plugins/` subdirectory of your GWToolbox++ source directory (e.g., `path/to/GWToolboxpp/plugins/Observer`).  
    > **Note:** The plugin folder must be named `Observer` for the build system to detect it. For example, you should have `plugins/ObserverPlugin/Observer/Debug`.

3. **Update CMake Configuration:**  
    Add the following block to `GWToolboxpp/cmake/gwtoolboxdll_plugins.cmake` to register the plugin with the build system:
    ```cmake
    # === Observer Plugin ===
    # Requires ZLIB for log compression
    find_package(ZLIB REQUIRED)

    add_library(Observer SHARED)
    target_sources(Observer PRIVATE
         "plugins/ObserverPlugin/Observer/ObserverStoC.cpp"
         "plugins/ObserverPlugin/Observer/ObserverStoC.h"
         "plugins/ObserverPlugin/Observer/ObserverMatch.cpp"
         "plugins/ObserverPlugin/Observer/ObserverMatch.h"
         "plugins/ObserverPlugin/Observer/ObserverCapture.cpp"
         "plugins/ObserverPlugin/Observer/ObserverCapture.h"
         "plugins/ObserverPlugin/Observer/ObserverLoop.cpp"
         "plugins/ObserverPlugin/Observer/ObserverLoop.h"
         "plugins/ObserverPlugin/Observer/ObserverPlugin.cpp"
         "plugins/ObserverPlugin/Observer/ObserverPlugin.h"
         "plugins/ObserverPlugin/Observer/ObserverMatchData.h"
         "plugins/ObserverPlugin/Observer/ObserverMatchData.cpp"
         "plugins/ObserverPlugin/Observer/Debug/StoCLogWindow.h"
         "plugins/ObserverPlugin/Observer/Debug/StoCLogWindow.cpp"
         "plugins/ObserverPlugin/Observer/Debug/LivePartyInfoWindow.h"
         "plugins/ObserverPlugin/Observer/Debug/LivePartyInfoWindow.cpp"
         "plugins/ObserverPlugin/Observer/Debug/LiveGuildInfoWindow.h"
         "plugins/ObserverPlugin/Observer/Debug/LiveGuildInfoWindow.cpp"
         "plugins/ObserverPlugin/Observer/Debug/AvailableMatchesWindow.h"
         "plugins/ObserverPlugin/Observer/Debug/AvailableMatchesWindow.cpp"
         "plugins/ObserverPlugin/Observer/Debug/CaptureStatusWindow.h"
         "plugins/ObserverPlugin/Observer/Debug/CaptureStatusWindow.cpp"
         "plugins/ObserverPlugin/Observer/dllmain.cpp"
         "plugins/ObserverPlugin/Observer/Windows/MatchCompositionsWindow.h"
         "plugins/ObserverPlugin/Observer/Windows/MatchCompositionsWindow.cpp"
         "plugins/ObserverPlugin/Observer/Windows/MatchCompositionsSettingsWindow.h"
         "plugins/ObserverPlugin/Observer/Windows/MatchCompositionsSettingsWindow.cpp"
    )
    target_include_directories(Observer PRIVATE
         "plugins/ObserverPlugin/Observer"
    )

    target_link_directories(Observer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GWCA/lib)

    target_link_libraries(Observer PRIVATE
         plugin_base
         GWCA 
         ZLIB::ZLIB
         GWToolboxdll # Explicitly link for Resources::GetSkillImage
    )
    target_compile_definitions(Observer PRIVATE WIN32)

    target_compile_features(Observer PRIVATE cxx_std_23)

    target_compile_options(Observer PRIVATE /FI"algorithm")

    target_compile_options(Observer PRIVATE /W4 /WX /Gy /wd4201 /wd4505 /wd4996)
    target_compile_options(Observer PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
    target_compile_options(Observer PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
    target_link_options(Observer PRIVATE /WX /OPT:REF /OPT:ICF /SAFESEH:NO)
    target_link_options(Observer PRIVATE $<$<NOT:$<CONFIG:Debug>>:/LTCG /INCREMENTAL:NO>)
    target_link_options(Observer PRIVATE $<$<CONFIG:Debug>:/IGNORE:4098 /OPT:NOREF /OPT:NOICF>)
    target_link_options(Observer PRIVATE $<$<CONFIG:RelWithDebInfo>:/OPT:NOICF>)
    set_target_properties(Observer PROPERTIES FOLDER "plugins/")
    ```

> [!CAUTION]
> Some source files depend on **Base** sources (e.g., `../Base/stl.h`). Make sure your directory structure is correct to avoid compilation errors.

# Build Configurations

The Observer Plugin uses the same build configuration as GWToolbox++.

## Debug Build

Run:
```
cmake --build build --config Debug
```
The Debug build provides extra details about internal changes and ObserverModule states.

## Release Build

Run:
```
cmake --build build --config RelWithDebInfo
```
This is the recommended build for end users.

# Debug Windows

Here are the debug windows you can have on debug version only:

These windows provide detailed real-time information useful for debugging or analysis.

## Main Window

Main Windows looks different on debug mode.

![Debug Window Toggles](../Assets/Main_Debug.png)

## Debug Toggles

Located in the main plugin window, this section allows you to control the visibility of each debug window.

![Debug Window Toggles](../Assets/Debug_Windows.png)

## Observer Plugin - Capture Status

Shows the real-time status of the internal data capture processes.

![Capture Status Window](../Assets/Debug_Capture.png)

*   **StoC Events Capture:** Indicates if Server-to-Client packets are being recorded.
*   **Agents States Capture:** Indicates if the thread capturing agent positions and states is running.

## Observer Plugin - Live Party Info

Displays detailed information about all players, heroes, and henchmen currently detected in the instance, grouped by party ID.

![Live Party Info Window](../Assets/Party_Infos.png)

*   Shows Agent ID, Level (L), Team ID (T), Guild ID (G), Professions, Player Number (for players), Name, and recently used Skill IDs.

## Observer Plugin - Live Guild Info

Displays information about the guilds associated with players in the current instance.

![Live Guild Info Window](../Assets/Guilds_Infos.png)

*   Shows Guild ID, Name, Tag, Rank, Rating, Faction Info, and Cape Details.

## Observer Plugin - Available Matches

Lists the matches available to observe in the current outpost, mirroring the game's own observer panel. It also includes display options to customize the information shown.

**Game's Observer Panel:**

![Game Observer Panel](../Assets/Available_Matches_Base.png)

**Plugin's Available Matches Window:**

![Available Matches Window](../Assets/Available_Matches.png)

*   **Display Options:** Toggle visibility for various match details (IDs, Map, Age, Type, Team info, Cape info, etc.).
*   **Match List:** Shows detailed information for each available match based on the selected display options.

## Observer Plugin - StoC Log Options

Allows you to toggle real-time display of specific Server-to-Client (StoC) events in the game chat. *Note: All events are recorded internally for export regardless of these settings.*

![StoC Log Options Window](../Assets/StoC_Chat_Logs.png)

*   Provides granular control over which types of events (Skill, Attack, Combat, Agent, Jumbo Messages) are logged to the chat.