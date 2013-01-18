Build Notes
===========

## GCC: Building Release and Debug
Rerun MWC command to specify Release or Debug (default: Release)
$ mwc.pl -type make liquibook.mwc -value_template configurations=Debug

## GCC: Building with ACE or OpenDDS
To build with OpenDDS or ACE, use a different MWC build type:
$ mwc.pl -type gnuace liquibook.mwc

Then, to switch between __Debug__ and __Release__ builds, modify the file $ACE_ROOT/include/makeinclude/platform_macros.GNU:

Debug:
boost=1
debug=1
inline=0
optimize=0
include $(ACE_ROOT)/include/makeinclude/platform_linux.GNU

Release:
boost=1
debug=0
inline=1
optimize=1
include $(ACE_ROOT)/include/makeinclude/platform_linux.GNU

