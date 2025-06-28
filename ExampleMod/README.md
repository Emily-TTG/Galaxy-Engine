## Modding

This directory contains an example "mod" for GalaxyEngine ("GE"). The modding
interface is incredibly simple -- you just define a cmake `SHARED` library
target, and link against `GalaxyEngineInterface` in cmake (from a subdirectory,
`FetchContent` call, or however you want to expose the GE cmake tree).

Once that's done -- you just include GE headers as if you were writing code
in-tree, and define any function you see declared in one of the GE headers. The
mod loader will automatically locate these functions and call them on the
_rising edge_ of the "real" GE call.

The only "magic" functions you are required to implement are the static members
of `GEMod` from `Mod/modState.h` - where the `Startup` function is expected to
report mod metadata.

**Some limitations at present:**

- Modded calls cannot terminate prematurely; i.e. you cannot presently wholly
override "vanilla" behaviours. This will be resolved in the future.
- Modded calls cannot _tail_ a "vanilla" call. This will be resolved in the
future.
- Any functions defined inline in headers will not be able to be overridden. If
you encounter critical behaviour defined as such - please create an issue,
notify the devs, or create a PR to move it out to source.
- Templates and inline functions cannot be easily marked for hooking as-is. 
Even if loading from instantiated function template symbol names, there is the
chance that other modules (i.e. main, sibling mods etc.) would hold a local
definition which could lead to mismatches. The majority of operative function
bodies should be written in source for this reason, with minimal adapter code
in the template. **This is not a limitation on _using_ templates from mods,
which will work fine.**
- Functions defined in dependencies cannot be hooked.
- RAII constructs (i.e. constructors etc.) and operators cannot be hooked.
