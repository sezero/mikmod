
Mikmod 3.something -- An official release... or something!
By Jake Stine of Hour 13 Studios
__________________________________________________________
[this tsxt document best viewed in an editor which supports automatic word wrapping]


-/- Package contents!
---------------------

This package includes three folders (two libraries -- Mikmod and MMIO) and a bin folder, which contains pre-compiled libraries.  MMIO stands for 'Mikmod Input/Output' but has grown to encompass a lot more than that over the years.  MMIO has fancy error handling features and file i/o layers for treating from-memory accesses the same as from-disk accesses.  These are all features whcih are useful if you are making your own custom module formats or using .wad type files.

See \mikmod\README for more information on compilation and useage notes.  Generally speaking, Mikmod.lib and mmio.lib must both be linked with your project in order for it to compile properly without error.  Alternatively, you can include the mikmod and mmio projects directly into your Visual Studio workspace instead of using pre-compiled lib files.

* Don't Forget! Add dxguid.lib and winmm.lib to your active project!  [Under Project Options, Link tab]  These are required really by almost any level of game development in Windows so proably you already have them in there. I hope. :)

 
-/- Using Pre-compiled Libs
---------------------------

The pre-compiled libs are for Visual Studio 6 only, and I have no idea if they would work with any other compiler on earth.  If you can figure out how, more power to ya!

First and foremost:

 - REQUIRED :  ensure that you are using Multithreaded and Debug Multithreaded CRT libraries in all of your projects.  You should always use use them regardless.  They have almost no speed/performance impact and they are just a lot safer... plus, they are required to use mikmod!

 - Recommended : Set your stucture packing to 4 bytes (VC defaults to 8), for both debug and release modes.  Unless you are using lots of MMX or SSE instructions this will speed up the rest of your code too so it's just a good idea.

  ________________________________________________________________________
  Debug mode:
    - lib\debug\mikmod_d.lib
    - lib\debug\mmio_d.lib

  Release mode:
  [There are two versions of the Release Mode pre-compiled libs included]

    - lib\release\mikmod.lib       \  Uses _fastcall
    - lib\release\mmio.lib         /

    - lib\release\mikmod_safe.lib  \  Uses _cdecl
    - lib\release\mmio_safe.lib    /

The first version is compiled using my default compilation options, which provide the most efficient results in terms of both memory usage and CPU usage.  If you want to use these libs in your project, you must ensure the following options are set in your project's C/C++ Code Generation options:

 - Use _cdecl calling convention for debug, and _fastcall calling convention
   for Release.

The second verion of the libes, postfixed with _safe, are compiled using Visual Studio's default _cdecl compilation option.

* The general rule of thumb is that if you get lots of linker errros for unresolved externals when building your release-mode app, try the other set of lib files. :)


-/- Conclusion
--------------

Pray it works.  This is very important to remember. :)

Notes:

 - There have been many contirbutors to Mikmod over the years and I feel very sorry that I really don't remember most of them and I never documented them.  Then again, I've gone uncredited in dozens of things as well so I guess it kinda evens out in the end.

Ok, so that's lame.  Maybe I'll do some research and try to write down the list of the contributors (or at least a parial list of the most significant contributors) sometime in the near future.

- Air
- Hour 13 Studios
- air@hour13.com
