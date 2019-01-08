# The Pringle

Halo 3 Online hack built on top of [ElDorito](https://github.com/ElDewrito/ElDorito).

The `hacked` branch contains the hacked code, the `master` branch has the upstream code that can be pulled and merged over into `hacked` on updates.

## Features

 - Aimbot
   - Projectile prediction with accurate time-to-impact and player position estimation/confidence by calculating derivatives
 - Chams
 - ESP
 - Markers
 - Service tag changing
 - No fog
 - Speed hack
 - Air acceleration (flying)
 - Easily extensible via hooks

## Credits

 - [C0BRA](https://github.com/AshleighAdams)
 - [fr1kin](https://github.com/fr1kin)
 - [Victormeriqui](https://github.com/Victormeriqui)
 - [babbaj](https://github.com/babbaj)
 - [0x22](https://github.com/0-x-2-2)
 - [popbob](https://github.com/oremonger)

## Videos

 - [Projectile prediction](https://www.youtube.com/watch?v=EcZkf2AP190)
   - [Receiving end](https://www.youtube.com/watch?v=TGsBHE85F6Q)
   - [Time to impact iteration demo 1](https://www.youtube.com/watch?v=xOa3K9HbOOY)
   - [Time to impact iteration demo 2](https://www.youtube.com/watch?v=)
 - [Aimbot, speedhack, and air acceleration](https://www.youtube.com/watch?v=DBBoimZ9wCQ)
 - [Aimbotting and griffball trolling](https://www.youtube.com/watch?v=c6bW97Q75DE)

## Setup

Copy the fully patched and updated `ElDorito` runtime to `./HaloOnline/`, open the solution, and set ElDorito as the startup project, along with the following configuration options set:

Option                    | Setting                  
--------------------------|-------------------------
*General*                 |
Windows SDK Version       | 8.1
Output Directory          | $(SolutionDir)HaloOnline\\
Intermediate Directory    | $(ProjectDir)$(Configuration)\\
Target Name               | mtndew
Target Extension          | .dll
Platform Toolset          | Visual Studio 2017 (v141)
*Debugging*               |
Command                   | $(SolutionDir)HaloOnline\eldorado.exe
Working Directory         | $(ProjectDir)

The same can be done for CefProcess too, using `custom_menu.exe` instead of `mtndew.dll`.

## Hacked Code

To keep the hacked code all together, and to make it easy to merge upstream changes, hack implementations must live in `./ElDorito/Source/ThePringle/` and be fully namespaced, with code changes outside being:

 - As minimal as possible.
 - Hook where behavior can be implemented by subscribing to those hooks.
 - Not changing program flow unless absolutely necessary.

### Speedhack Example

The function `Patches/PlayerScale.cpp:833: void BipedMovementPhysics(s_biped_physics_data1 *data, ...)` is responsible for moving and accelerating the physics object the local player controls. In this function, there is a local variable `scale`, that if modified, will adjust the speed the player runs at. We can then create the following struct with a reference to that scale as such:

```c++
struct ModifySpeedMultiplier
{
	float& Speed;
	ModifySpeedMultiplier(float& speed) : Speed(speed) { }
};
```

We can then add the line:

```c++
Pringle::Hook::Call<Pringle::Hooks::ModifySpeedMultiplier>(scale);
```

To just below where the scale is initialized. Due to a reference to the scale being passed (in the member variable `Speed`, the hack side code can subscribe to the `ModifySpeedMultiplier` event, and mutate the Speed variable to control it in an extensible way, as such:

```c++
Hook::SubscribeMember<ModifySpeedMultiplier>(this, &SpeedHack::OnModifySpeedMultiplier);
// ...
void SpeedHack::OnModifySpeedMultiplier(const ModifySpeedMultiplier & msg)
{
	if (this->Enabled->ValueInt != 0)
		msg.Speed *= this->Factor->ValueFloat;
}
```

Note how the logic is done hack side and not game side, with the change compounding (i.e. not overwriting) the existing value. This allows the functionality to be extended in many places, not just one place. For example, you could add jitter to the speed to make you harder to hit without changing the overall average running speed, while also still being compatible with the speedhack.