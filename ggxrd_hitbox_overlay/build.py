# if you modify the portions of this script that call regenerate_ini_and_update_readme and update ggxrd_hitbox_overlay.rc, you must also modify prebuild.ps1 in the same way
import sys
import xml.etree.ElementTree
import os
import subprocess
import shutil
import re
import importlib.util
# didn't want to create __init__.py
spec = importlib.util.spec_from_file_location(
 "common_prebuild",
 os.path.join("..", "common", "prebuild.py")
)
common_prebuild = importlib.util.module_from_spec(spec)
spec.loader.exec_module(common_prebuild)

get_mtime = common_prebuild.get_mtime
compare_filetimes = common_prebuild.compare_filetimes

spec = importlib.util.spec_from_file_location(
 "regenerate_ini_and_update_readme",
 os.path.join("..", "regenerate_ini_and_update_readme.py")
)
regenerate_ini_and_update_readme = importlib.util.module_from_spec(spec)
spec.loader.exec_module(regenerate_ini_and_update_readme)

cwd = os.getcwd()
solution_folder = os.path.dirname(cwd)

is_debug = False
is_clean = False
if len(sys.argv) > 1:
 for i in range(1, len(sys.argv)):
  arg = sys.argv[i]
  if arg == "-d" or arg == "--debug":
   is_debug = True
  elif arg == "--help" or arg == "-h" or arg == "/?":
   print("This program is supposed to be launched from the ggxrd_hitbox_overlay folder"
         " (please cd into it first) and it cross-compiles ggxrd_hitbox_overlay DLL."
         " You must set CHOST environment variable to the compiler toolchain prefix"
         " prior to running this. For example, if your MinGW compiler is"
         " i686-w64-mingw32-gcc, set CHOST to i686-w64-mingw32. Example:\n"
         "CHOST=i686-w64-mingw32 python3 build.py\n"
         "or\n"
         "export CHOST=i686-w64-mingw32\n"
         "python3 build.py\n"
         "or\n"
         "CHOST=i686-w64-mingw32\n"
         "export CHOST\n"
         "python3 build.py"
   )
  elif arg == "--clean" or arg == "-clean" or arg == "clean":
   is_clean = True

if not is_debug:
 build_folder = "Release"
else:
 build_folder = "Debug"

if is_clean:
 print(f"rm {build_folder}/*")
 if os.path.isdir(build_folder):
  for name in os.listdir(build_folder):
   os.remove(os.path.join(build_folder, name))
 exit(0)

print(f"Will build a {build_folder} build.")

if "CHOST" not in os.environ:
 print("CHOST not specified. Please specify the prefix of the cross-compiler toolchain there, for example: if compiler is i686-w64-mingw32-gcc, then CHOST must be i686-w64-mingw32. Exiting.")
 exit(1)

CHOST = os.environ["CHOST"]
print(f"Will use CHOST={CHOST}.")

ignored_warnings = [
 # converting to non-pointer type ‘DWORD’ {aka ‘long unsigned int’} from NULL: DWORD suspenderThreadId = NULL;
 "-Wno-conversion-null",
 
 # comparison between ‘const enum Moves::RamlethalStateName’ and ‘enum Moves::RamlethalStateName2’: info.state == Moves::ram2_loop
 "-Wno-enum-compare",
 
 # trigraph ??) ignored, use -trigraphs to enable: "68 rel(?? ?? ?? ??)" (is it talking about the string literal? Those quotation marks are in a string. How can they possibly mean anything else?)
 "-Wno-trigraphs",
 
 #  the value of ‘INPUTS_ICON_ATLAS_WIDTH’ is not usable in a constant expression: uEnd((float)(x + INPUTS_ICON_SIZE) / INPUTS_ICON_ATLAS_WIDTH),
 "-Wno-invalid-constexpr",
 
 # ‘offsetof’ within non-standard-layout type ‘Settings’ is conditionally-supported: offsetof(Settings, fieldName)
 "-Wno-invalid-offsetof"
]

if "CFLAGS" in os.environ:
 CFLAGS = os.environ["CFLAGS"]
else:
 # the GCC manual: https://gcc.gnu.org/onlinedocs/gcc/index.html#SEC_Contents
 # -fpermissive: because doesn't allow to implicitly convert function pointers to PVOID
 # -mdll removed. Incompatible with -static, and CMake produces makefiles with -static instead of -mdll.
 CFLAGS = "-m32" \
          " -mwindows" \
          " -masm=intel" \
          " -march=pentium3" \
          " -mavx" \
          " -mmmx" \
          " -msse" \
          " -mhard-float" \
          " -mms-bitfields" \
          " -DUNICODE" \
          " -fpermissive"
 if not is_debug:
  # try removing some optimization flags if the build doesn't work
  CFLAGS += " --optimize -flto -fuse-linker-plugin -fno-fat-lto-objects"
 CFLAGS += " " + " ".join(ignored_warnings)
print(f"Will use CFLAGS={CFLAGS}.")

tree = xml.etree.ElementTree.parse("ggxrd_hitbox_overlay.vcxproj")
root = tree.getroot()
ns = "{http://schemas.microsoft.com/developer/msbuild/2003}"
PROPERTY_GROUP = ns + "PropertyGroup"
INCLUDE_PATH = ns + "IncludePath"
ITEM_DEFINITION_GROUP = ns + "ItemDefinitionGroup"
CL_COMPILE = ns + "ClCompile"
PREPROCESSOR_DEFINITIONS = ns + "PreprocessorDefinitions"
ITEM_GROUP = ns + "ItemGroup"
EXCLUDED_FROM_BUILD = ns + "ExcludedFromBuild"

include_folders_parent = None
for child in root:
 if child.tag == PROPERTY_GROUP \
    and "Condition" in child.attrib \
    and child.attrib["Condition"] == "'$(Configuration)|$(Platform)'=='Release|Win32'" \
    and "Label" not in child.attrib:
  include_folders_parent = child
  break

if include_folders_parent is None:
 print("Could not find Release|Win32 PropertyGroup")
 exit(1)

include_folders_node = None
for child in include_folders_parent:
 if child.tag == INCLUDE_PATH:
  include_folders_node = child
  break

if include_folders_node is None:
 print("IncludePath not found in Release|Win32 PropertyGroup")
 exit(1)

include_folders = [
  x.replace("\\", os.sep) for x in include_folders_node.text.split(";") \
  if x != "$(ProjectDir)" \
     and x != "$(IncludePath)" \
     and x
]
include_folders.append(".")

print(f"Parsed IncludePath: {", ".join(include_folders)}.")

if is_debug:
 search_condition = "'$(Configuration)|$(Platform)'=='Debug|Win32'"
else:
 search_condition = "'$(Configuration)|$(Platform)'=='Release|Win32'"

item_definition_group = None
for child in root:
 if child.tag == ITEM_DEFINITION_GROUP \
    and "Condition" in child.attrib \
    and child.attrib["Condition"] == search_condition:
  item_definition_group = child
  break


if item_definition_group is None:
 print("Couldn't find ItemDefinitionGroup to get preprocessor definitions.")
 exit(1)

cl_compile = None
for child in item_definition_group:
 if child.tag == CL_COMPILE:
  cl_compile = child
  break

if cl_compile is None:
 print("Couldn't find ClCompile in ItemDefinitionGroup to get preprocessor definitions.")
 exit(1)

preprocessor_definitions = None
for child in cl_compile:
 if child.tag == PREPROCESSOR_DEFINITIONS:
  preprocessor_definitions = child.text
  break

if preprocessor_definitions is None:
 print("Couldn't find PreprocessorDefinitions in ClCompile in ItemDefinitionGroup to get preprocessor definitions.")
 exit(1)

preprocessor_definitions = [
 x for x in preprocessor_definitions.split(";") \
 if x \
    and x != "%(PreprocessorDefinitions)"
]

print(f"Parsed PreprocessorDefinitions: {", ".join(preprocessor_definitions)}.")

if os.path.exists(build_folder) and not os.path.isdir(build_folder):
 print(f"Name '{ build_folder }' occupied by something that isn't a directory. Please delete that.")
 exit(1)

if not os.path.exists(build_folder):
 print(f"Created {build_folder} folder.")
 os.mkdir(build_folder)

new_version = common_prebuild.parse_version("Version.h")
if new_version is None:
 exit(1)

print(f"Parsed Version.h, got version {new_version}.")

with open("ggxrd_hitbox_overlay.rc", "rt", encoding="utf-16") as f:
 resource_lines = f.read().splitlines()

need_write_resource_file = False
version_parts = new_version.split(".")
new_version_four_digits = version_parts[0] + ",0," + version_parts[1] + ",0"

if common_prebuild.replace_file_and_product_versions(resource_lines, new_version_four_digits):
 need_write_resource_file = True

# if you modify this function, modify the same function in prebuild.ps1
def replace_absolute_paths_with_relative(content_lines, solution_folder):
 need_write = False
 # .rc file doubles all slashes, so we double too
 project_abs_path = solution_folder.replace("\\", "\\\\")
 project_abs_path_lowercase = project_abs_path.lower()
 for i in range(0, len(content_lines)):
  line = content_lines[i]
  match = re.match(r'^([\w\d_]+\s+[\w\d_]+\s+")(\w:\\\\[^"]*)"$', line)
  if match:
   line_path = match.group(2)
   if line_path.lower().startswith(project_abs_path_lowercase):
    # this .rc file is of the project. From the project we get to the solution folder using .., so that's why we replace the solution folder with ..
    content_lines[i] = match.group(1) + ".." + line_path[len(project_abs_path) : ] + "\""
    need_write = True
 return need_write

if replace_absolute_paths_with_relative(resource_lines, solution_folder):
 need_write_resource_file = True

if need_write_resource_file:
 print("Updating ggxrd_hitbox_overlay.rc...")
 with open("ggxrd_hitbox_overlay.rc", "wt", encoding="utf-16le", newline="\r\n") as f:
  f.write("\ufeff")  # this is called a BOM (Byte Order Mark)
  f.write("\n".join(resource_lines))

# The resource file is in utf-16 little-endian, but MinGW preprocessor can't read that, it needs utf-8.
# If you specify -c utf-16le to WINDRES it will just ignore that, and if you specify --preprocessor-arg='-finput-charset=utf-16le' to WINDRES, it will pass -finput-charset=utf-16le to the preprocessor, but then the preprocessor won't understand the files that the .rc includes, because they're in utf-8, and you can't specify more than one encoding, or a separate encoding for each file, and it doesn't seem to autodetect encodings despite the large obvious BOM (FF FE) at the start.
# Use iconv --from-code=utf-16le --to-code=utf-8 -o thing_utf8.rc thing_utf16.rc to convert the encoding or, the thing we'll be using right now, a python script.
# By the way, windres won't be able to find files if they're specified with backslashes, so backslashes must be replaced with forward slashes, too.
if need_write_resource_file or compare_filetimes("ggxrd_hitbox_overlay_utf8.rc", "ggxrd_hitbox_overlay.rc") < 0:
 print("Writing ggxrd_hitbox_overlay_utf8.rc...")
 with open("ggxrd_hitbox_overlay_utf8.rc", "wt", encoding="utf-8") as f:
  f.write("\n".join(resource_lines).replace("\\\\", "/"))

if not is_debug:
 jwasm_out_dir_name = "GccUnixR"
else:
 jwasm_out_dir_name = "GccUnixD"

jwasm_dir = os.path.join(solution_folder, "JWasm")
jwasm_out_dir = os.path.join(jwasm_dir, jwasm_out_dir_name)
jwasm_bin = os.path.join(jwasm_out_dir, "jwasm")

asmhooks_o_path = os.path.join(build_folder, "asmhooks.o")
if compare_filetimes(asmhooks_o_path, "asmhooks.asm") < 0:
 if not (
         os.path.isdir(jwasm_dir) \
         and os.path.isfile(os.path.join(jwasm_dir, "GccUnix.mak"))
 ):
  print("Downloading JWasm...")
  subprocess.run(
   ["git", "clone", "https://github.com/JWasm/JWasm.git"],
   cwd = solution_folder
  ).check_returncode()
 
 if not os.path.isfile(jwasm_bin):
  print("Building JWasm...")
  if is_debug:
   new_args = ["make", "-f", "GccUnix.mak", "DEBUG=1"]
  else:
   new_args = ["make", "-f", "GccUnix.mak"]
  
  # We expect our CC and CFLAGS to be for the cross-compiler, whereas jwasm must be compiled
  # for the host operating system instead of the target operating system.
  new_env = {k:v for k,v in os.environ.items() if k not in ("CC", "CFLAGS")}
  
  subprocess.run(
   new_args,
   cwd = jwasm_dir,
   env = new_env
  ).check_returncode()
 
 print("Building asmhooks.asm using JWasm...")
 if os.path.isfile("asmhooks.o"):
  os.remove("asmhooks.o")
 elif os.path.exists("asmhooks.o"):
  print("asmhooks.o already exists and isn't a file.")
  exit(1)
 
 # https://baron-von-riedesel.github.io/JWasm/Html/Manual.html#CMDOPTMZ
 # -6 means .686 CPU
 subprocess.run([jwasm_bin, "-6", "-coff", "asmhooks.asm"]).check_returncode()
 
 if os.path.isfile(asmhooks_o_path):
  os.remove(asmhooks_o_path)
 elif os.path.exists(asmhooks_o_path):
  print(f"{asmhooks_o_path} already exists and isn't a file.")
  exit(1)
 
 shutil.move("asmhooks.o", asmhooks_o_path)

if not os.path.isfile("../Detours/lib.X86/detours.a"):
 print("Building Detours...")
 detours_folder = os.path.join(solution_folder, "Detours")
 
 new_env = os.environ.copy()
 if "CFLAGS" in new_env:
  new_env["CFLAGS"] += " -D_WIN32_WINNT=0x501"  # this define wrecks std::thread and std::mutex
 
 subprocess.run(
  ["make"],
  cwd = detours_folder,
  env = new_env
 ).check_returncode()

if not os.path.isfile("../zlib/libz.a"):
 print("Building zlib...")
 zlib_folder = os.path.join(solution_folder, "zlib")
 
 subprocess.run(
  ["sh", "configure", "--static"],
  cwd = zlib_folder
 ).check_returncode()
 
 subprocess.run(
  ["make"],
  cwd = zlib_folder
 ).check_returncode()

if not os.path.isfile("../libpng/libpng.a"):
 print("Building libpng...")
 
 new_env = os.environ.copy()
 new_env["CC"] = CHOST + "-gcc"
 new_env["AR"] = CHOST + "-ar"
 new_env["RANLIB"] = CHOST + "-ranlib"

 has_optimization = False
 if "CFLAGS" in new_env:
  for flag in new_env["CFLAGS"].split(" "):
   if flag.strip().startswith("-O"):
    has_optimization = True
 
 if not has_optimization:
  new_env["CFLAGS"] = CFLAGS + " -O2"
 
 subprocess.run(
  ["make", "-f", "scripts/makefile.gcc"],
  cwd = os.path.join(solution_folder, "libpng"),
  env = new_env
 ).check_returncode()
 
CFLAGS_split = [x.strip() for x in CFLAGS.split(" ") if x]
include_folders_i = ["-I" + x for x in include_folders]
preprocessor_definitions_d = ["-D" + x for x in preprocessor_definitions]

source_files = []
for item_group in root:
 if item_group.tag == ITEM_GROUP:
  for cl_compile in item_group:
   if cl_compile.tag == CL_COMPILE:
    is_excluded = False
    for nested_elem in cl_compile:
     if nested_elem.tag == EXCLUDED_FROM_BUILD:
      is_excluded = True
      break
    if not is_excluded:
     source_files.append(cl_compile.attrib["Include"].replace("\\", os.sep))

root = None
tree = None

compiler_bin = CHOST + "-gcc"
cpp_compiler_bin = CHOST + "-g++"
# the GCC manual: https://gcc.gnu.org/onlinedocs/gcc/index.html#SEC_Contents
# Check "Option Summary"
args_c = [compiler_bin, *CFLAGS_split, "-c", *include_folders_i, *preprocessor_definitions_d, "source_file", "-o", "output_file"]
args_c_cpp = args_c.copy()
args_c_cpp[0] = cpp_compiler_bin
args_MMD = [compiler_bin, *CFLAGS_split, "-MMD", "-c", *include_folders_i, *preprocessor_definitions_d, "source_file", "-o", "output_file"]
args_MMD_cpp = args_MMD.copy()
args_MMD_cpp[0] = cpp_compiler_bin
args_MM = [compiler_bin, *CFLAGS_split, "-MM", "-E", *include_folders_i, *preprocessor_definitions_d, "source_file"]
args_MM_cpp = args_MM.copy()
args_MM_cpp[0] = cpp_compiler_bin
windres_args = [CHOST + "-windres", *include_folders_i, *preprocessor_definitions_d, "source_file", "output_file"]
res_out = os.path.join(build_folder, "ggxrd_hitbox_overlay.rc.o")

# From imgui/backends/imgui_impl_win32.cpp:
#pragma comment(lib, "gdi32")   // Link with gdi32.lib for GetDeviceCaps(). MinGW will require linking with '-lgdi32'
#pragma comment(lib, "dwmapi")  // Link with dwmapi.lib. MinGW will require linking with '-ldwmapi'

# The "linker" program is called ld. GCC calls it (CHOST-ld, but actually some_weird_folder_somewhere/collect2 but sh-h-h... check by passing -v to g++) when linking, and you can pass more arguments to it directly with -Wl,ARGUMENT than GCC is aware of.
# Description of ld arguments: https://linux.die.net/man/1/ld
args_linker = [CHOST + "-g++", *CFLAGS_split]
if not is_debug:
 args_linker.append("-DNDEBUG")  # stolen from a generated CMake makefile
 args_linker.append("-g0")  # doesn't do anything, in my experience, filesize is the same with or without it and the debug sections are present anyway with line numbers, despite me specifying this option
else:
 args_linker.append("-D_DEBUG")  # the vcxproj has this, I don't know if it's actually needed...
args_linker += [
 "-shared",  # stolen from CMake. Replaces -mdll, but I don't know what the exact difference is, was just trying stuff around (the DLL started working for a different reason), so sticking with what I got in the end
 "-static-libgcc",  # I get GetLastError() 126 "The specified module could not be found." after a LoadLibrary call to load the produced DLL fails. The DLL depends on libgcc_s_dw2-1.dll for __deregister_frame_info and __register_frame_info (possibly more) functions. Link statically to avoid having to ship this dependency? 
 "-static-libstdc++",  # same as above
 "-Wl,--start-group",
 "-L../d3d9",
 "-l:d3dcompiler.lib",
 "-l:d3dx9.lib",
 "-lgdi32",  # for ImGui
 "-ldwmapi",  # for ImGui
 "-lcomdlg32",  # for _imp__GetSaveFileNameW@4, _imp__GetOpenFileNameW@4, _imp__CommDlgExtendedError@0 used by UI.o
 "../zlib/libz.a",
 "../libpng/libpng.a",
 "../Detours/lib.X86/detours.a",
 res_out,
 asmhooks_o_path]

output_files = []

for source_file in source_files:
 source_name = os.path.basename(source_file)
 source_pair = os.path.splitext(source_name)
 is_cpp = source_pair[1].lower() == ".cpp"
 d_path = os.path.join(build_folder, source_pair[0] + ".d")
 o_path = os.path.join(build_folder, source_pair[0] + ".o")
 output_files.append(o_path)
 d_mtime = get_mtime(d_path)
 o_mtime = get_mtime(o_path)
 source_mtime = get_mtime(source_file)
 if source_mtime is None:
  print(f"{source_file} missing.")
  exit(1)
 
 class VarHolder:
  pass
 
 var_holder = VarHolder()
 var_holder.already_re_d = False
 var_holder.already_re_o = False
 
 # remake the d_path file
 def re_d():
  if var_holder.already_re_d:
   return
  var_holder.already_re_d = True
  if not is_cpp:
   args_MM_use = args_MM
  else:
   args_MM_use = args_MM_cpp
  args_MM_use[-1] = source_file
  print(" ".join(args_MM_use) + " > " + d_path)
  if d_mtime is not None:
   os.remove(d_path)
  with open(d_path, "wt") as f:
   subprocess.run(args_MM_use, universal_newlines=True, stdout=f.fileno()).check_returncode()
 
 # remake o_path
 def re_o():
  if var_holder.already_re_o:
   return
  var_holder.already_re_o = True
  if var_holder.already_re_d:
   if is_cpp:
    args_c_use = args_c_cpp
   else:
    args_c_use = args_c
  else:
   var_holder.already_re_d = True
   if is_cpp:
    args_c_use = args_MMD_cpp
   else:
    args_c_use = args_MMD
  args_c_use[-3] = source_file
  args_c_use[-1] = o_path
  print(" ".join(args_c_use))
  if o_mtime is not None:
   os.remove(o_path)
  subprocess.run(args_c_use).check_returncode()
 
 # check if any dependency from the d_path file is newer than either d_path or o_path
 # set var_holder.already_re_d to True and re_o() if o_path is old
 # set var_holder.already_re_d to True and re_d() if d_path is old
 def check_d():
  with open(d_path, "rt") as f:
   deps = f.read()
  
  pos = deps.find(":")
  if pos != -1:
   deps = deps[pos + 1:]
  for dep_name in deps.replace("\\\n", " ").split(" "):
   if not dep_name:
    continue
   dep_mtime = get_mtime(dep_name)
   if dep_mtime is None:
    continue
   
   if not var_holder.already_re_o and dep_mtime > o_mtime:
    re_o()
    if var_holder.already_re_d:
     break
   
   if not var_holder.already_re_d and dep_mtime > d_mtime:
    re_d()
    if var_holder.already_re_o:
     break
    # gotta restart, dependencies might have changed
    check_d()
    break
 
 if o_mtime is None:
  re_o()
 else:
  if d_mtime is None:
   re_d()
  check_d()
 
if compare_filetimes(res_out, "ggxrd_hitbox_overlay_utf8.rc") < 0:
 windres_args[-2] = "ggxrd_hitbox_overlay_utf8.rc"
 windres_args[-1] = res_out
 print(" ".join(windres_args))
 subprocess.run(windres_args).check_returncode()

args_linker += output_files

args_linker.append("-Wl,--end-group")  # group stuff is needed because the compiler only resolves undefined references in all previously encountered objects (.o) when it sees a new object or archive (.a). If it encounters an object after that, it won't resolve undefined symbols in it using all the previously seen symbols, as if it forgot all of them. So you have to put dependencies (the things you depend on) after the dependant (the you who depends), and dependencies must not back-reference the dependant so as not to create an unresolvable circular reference. Anyway, I'll stop acting like I came up with this and just post where I got this from: https://stackoverflow.com/questions/2738292/how-to-deal-with-recursive-dependencies-between-static-libraries-using-the-binut They say you could also put everything into an .a file using CHOST-ar with CHOST-ranlib sprinkled on top (see zlib or libpng makefiles, they use both ar and ranlib), or repeat all files after every other file (will that cause an object file to get linked twice, creating duplicate code?).

dll_path = os.path.join(build_folder, "ggxrd_hitbox_overlay.dll")
args_linker += ["-o", dll_path]
print(" ".join(args_linker))
subprocess.run(args_linker).check_returncode()

if not is_debug:
 subprocess.run(
  [CHOST + "-strip", "--strip-debug", dll_path]
 ).check_returncode()

print(f"Built {dll_path}.")

os.chdir("..")
if not regenerate_ini_and_update_readme.task():
 print("Error updating ggxrd_hitbox_overlay.ini and README.md.")
 exit(1)


