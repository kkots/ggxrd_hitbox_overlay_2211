# if you modify this script, you must also modify prebuild.ps1 in the same fashion
# could probably rewrite this in sh using sed to do all parsing and text processing
import os
import importlib.util
# didn't want to create __init__.py
spec = importlib.util.spec_from_file_location(
 "common_prebuild",
 os.path.join("..", "common", "prebuild.py")
)
common_prebuild = importlib.util.module_from_spec(spec)
spec.loader.exec_module(common_prebuild)

compare_filetimes = common_prebuild.compare_filetimes

with open("ggxrd_hitbox_injector_console.rc", "rt", encoding="utf-16") as f:
 resource_lines = f.read().splitlines()

new_version = common_prebuild.parse_version("InjectorConsoleVersion.h")
if new_version is None:
 exit(1)

need_write = False
new_version = new_version[2:] + ",0,0,0"
if common_prebuild.replace_file_and_product_versions(resource_lines, new_version):
 need_write = True

if need_write:
 print("Updating ggxrd_hitbox_injector_console.rc...")
 with open("ggxrd_hitbox_injector_console.rc", "wt", encoding="utf-16le", newline="\r\n") as f:
  f.write("\ufeff")  # this is called a BOM (Byte Order Mark)
  f.write("\n".join(resource_lines))

# The resource file is in utf-16 little-endian, but MinGW preprocessor can't read that, it needs utf-8.
# If you specify -c utf-16le to WINDRES it will just ignore that, and if you specify --preprocessor-arg='-finput-charset=utf-16le' to WINDRES, it will pass -finput-charset=utf-16le to the preprocessor, but then the preprocessor won't understand the files that the .rc includes, because they're in utf-8, and you can't specify more than one encoding, or a separate encoding for each file, and it doesn't seem to autodetect encodings despite the large obvious BOM (FF FE) at the start.
# Use iconv --from-code=utf-16le --to-code=utf-8 -o thing_utf8.rc thing_utf16.rc to convert the encoding or, the thing we'll be using right now, a python script.
# By the way, windres won't be able to find files if they're specified with backslashes, so backslashes must be replaced with forward slashes, too.
if need_write or compare_filetimes("ggxrd_hitbox_injector_console_utf8.rc", "ggxrd_hitbox_injector_console.rc") < 0:
 print("Writing ggxrd_hitbox_injector_console_utf8.rc...")
 with open("ggxrd_hitbox_injector_console_utf8.rc", "wt", encoding="utf-8") as f:
  f.write("\n".join(resource_lines).replace("\\\\", "/"))