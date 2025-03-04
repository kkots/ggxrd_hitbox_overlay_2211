#!/bin/bash
# Make sure to give yourself permission to launch this script with 'chmod u+x launch_ggxrd_hitbox_injector_linux.sh' command.
# Then cd into the same directory as this script and launch it using './launch_ggxrd_hitbox_injector_linux.sh' or 'bash launch_ggxrd_hitbox_injector_linux.sh'.

# This script launches the mod's .exe in Wine under the same wineserver as GuiltyGearXrd.exe process so that the mod can actually find the Guilty Gear process.

# This mod does not depend on any additional runtimes

# This mod's tool that we want to launch.
TOOLNAME32="${1-ggxrd_hitbox_injector.exe}"  # ${VARNAME-DEFAULTVALUE} substitutes DEFAULTVALUE in case the variable is empty
TOOLNAME64="${1-ggxrd_hitbox_injector64bit.exe}"
ONLY_PRINT_WINEPREFIX=false
if [ "$1" == "--only-print-wineprefix" ]; then
  ONLY_PRINT_WINEPREFIX=true
fi

GUILTYGEAR_PID=$(pidof -s GuiltyGearXrd.exe)
if [ $? != 0 ]; then # if last command's (pidof's) exit code is not success
    echo Guilty Gear Xrd not launched. Please launch it and try again.
    exit 1
fi

GUILTYGEAR_WINEPREFIX=$(
    
    # The environment variables the process was launched with are stored in
    # /proc/123456789/environ file.
    # Each entry is in the format VARIABLE_NAME=VALUE and the entries are
    # separated by the null ('\0') character.
    cat /proc/$GUILTYGEAR_PID/environ                                                              \
                                                                                                   \
     `# Replaces the null character with a newline so that grep can work with it.`                 \
    | tr '\0' '\n'                                                                                 \
                                                                                                   \
     `# Finds lines starting with WINEPREFIX=.`                                                    \
     `# We need the WINEPREFIX variable so we can launch our mod with the same`                    \
     `# WINEPREFIX as Guilty Gear so that it can see Gealty Gear's process.`                       \
     `# WINEPREFIX basically uniquely identifies a wineserver. A bunch of Wine programs`           \
     `# with the same WINEPREFIX run on the same wineserver and if the WINEPREFIXes are different,`\
     `# then they run on different wineservers and are completely isolated.`                       \
    | grep "^WINEPREFIX="                                                                          \
                                                                                                   \
     `# Stops reading the file through cat if there's already one line of output.`                 \
    | head -n1                                                                                     \
    | sed -r 's/^WINEPREFIX=//'  # Replaces the initial WINEPREFIX= with nothing, so we only get
                                 # the value part.
)
if [ "$GUILTYGEAR_WINEPREFIX" == "" ]; then
    echo "Couldn't determine Guilty Gear Xrd's WINEPREFIX."
    exit 1
fi

if [ $ONLY_PRINT_WINEPREFIX == true ]; then
  echo "$GUILTYGEAR_WINEPREFIX"
  exit
fi

GUILTYGEAR_WINELOADER=$(
    cat /proc/$GUILTYGEAR_PID/environ                                                      \
    | tr '\0' '\n'                                                                         \
                                                                                           \
     `# Finds lines starting with WINELOADER=.`                                            \
     `# The WINELOADER variable specifies the location of the Wine executable that`        \
     `# this Wine program was launched with. So even if you have multiple wineservers or`  \
     `# multiple Wine versions installed and running concurrently, we'll determine and use`\
     `# the exact same version as Guilty Gear is using.`                                   \
    | grep "^WINELOADER="                                                                  \
    | head -n1                                                                             \
    | sed -r 's/^WINELOADER=//'
)
if [ "$GUILTYGEAR_WINELOADER" == "" ]; then
    echo "Couldn't determine Guilty Gear Xrd's WINELOADER."
    exit 1
fi

if [ -d "$GUILTYGEAR_WINEPREFIX/drive_c/windows/syswow64" ]; then
  TOOLNAME=$TOOLNAME64
else
  TOOLNAME=$TOOLNAME32
fi

if [ ONLY_PRINT_WINEPREFIX == false ] && [ ! -f "$TOOLNAME" ]; then
    echo "$TOOLNAME" not found in the current directory. Please cd into this script\'s directory and launch it again.
    exit 1
fi

# The 2> /dev/null pipes stderr (error output) into nowhere.
# We need this because if this is a console app it will run in your current terminal window,
# and Wine often says irrelevant stuff into stderr during any program and even breaks in
# the middle of what the program is saying, and your terminal would display that as well if
# we don't silence it.
# The WINEFSYNC variable is needed because Wine would refuse to launch otherwise.
WINEFSYNC=1 WINEPREFIX="$GUILTYGEAR_WINEPREFIX" "$GUILTYGEAR_WINELOADER" $TOOLNAME 2> /dev/null
