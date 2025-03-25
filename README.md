# SADX Backface Culling
This mod enables backface culling in Sonic Adventure DX.

## 📋 Features
* Reimplements the double-sided material flag.
* Patches many materials to use or not use the double-sided flag.
* Fixes all quad/strip parsers to maintain winding order.

## 📥 How to install
You need the following prerequisites:
* [SA Mod Manager](https://github.com/X-Hax/SA-Mod-Manager)
* [Visual C++ Redistributable Runtimes (32 Bits)](https://aka.ms/vs/17/release/vc_redist.x86.exe)

To install this mod, download the mod archive from [here](https://github.com/kellsnc/sadx-backface-culling/releases/latest) and drop the "sadx-multiplayer" folder into your "mods" folder. Then, enable the mod in the Mod Manager.

## 🤝 Contributions
If you notice rendering issues, please report them as an [issue](https://github.com/kellsnc/sadx-backface-culling/issues) or in the dedicated post on the [X-Hax discord](https://discord.gg/gqJCF47).

If you are a level, object, or character mod author, please consider making your mod compatible with backface culling.

## 🛠 How to build:
You need the following pre-requisites on Windows:
* Visual Studio 2019/2022 with the v141 toolkit with XP support.
* The [DirectX 8.1 SDK](https://archive.org/details/dx81sdk_full)

Simply pull the repository, then open and build the solution.

## ©️ Disclaimer
This mod is a fan-made project and is not affiliated with SEGA or Sonic Team. All trademarks and intellectual properties belong to their respective owners. A copy of the game is needed.