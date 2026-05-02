# If you modify this script, you must also modify regenerate_ini_and_update_readme.ps1
import os
import re
import importlib.util

# didn't want to create __init__.py
spec = importlib.util.spec_from_file_location(
 "common_prebuild",
 os.path.join("common", "prebuild.py")
)
common_prebuild = importlib.util.module_from_spec(spec)
spec.loader.exec_module(common_prebuild)

get_mtime = common_prebuild.get_mtime

# This script only regenerates the INI and updates README.md if the latest date of modification of any of the following files:
# 1) ggxrd_hitbox_overlay\KeyDefinitions.h,
# 2) ggxrd_hitbox_overlay\SettingsDefinitions.h,
# 3) ggxrd_hitbox_overlay\SettingsTopCommentDefinition.cpp
# is greater than the date of README.md or ggxrd_hitbox_overlay.ini

# The README that we're going to update will be the root README of the entire solution.
# The INI file is located in the root as well.
# First we will update the INI, then the README.

class ParseCStringResult:
 def __init__(self, pos, txt):
  self.pos = pos
  self.txt = txt

# pos must point to an opening "
# unescapes everything
# removes comments
# stops at a # (preprocessor macro) or any character that is not inside a string and isn't a comment or a "
def parse_c_string(txt, pos):
 in_quotes = False
 builder = []
 result = ParseCStringResult(-1, "")
 quit_pos = -1
 len_txt = len(txt)
 # for loop keeps incrementing from its value even if you change i
 # let's use this crude contraption instead
 i = pos - 1
 while True:
  i += 1
  if i >= len_txt:
   break
  
  c_char = txt[i]
  c = ord(c_char)
  if not in_quotes:
   if c == 47:  # /
    if i + 1 < len_txt:
     next_c = ord(txt[i + 1])
     if next_c == 47:  # /
      next_pos = txt.find("\n", i)
      if next_pos == -1:
       break
      else:
       i = next_pos
       continue
     elif next_c == 42:  # *
      next_pos = txt.find("*/", i)
      if next_pos == -1:
       break
      else:
       i = next_pos + 1
       continue
     else:
      quit_pos = i
      break
    else:
     quit_pos = i
     break
   elif c == 34:  # "
    in_quotes = True
   elif c <= 32:  # whitespace
    continue
   else:
    quit_pos = i
    break
  elif c == 92:  # \
   i += 1
   if i < len_txt:
    next_c = ord(txt[i])
    if next_c == 110:  # n
     builder.append("\n")
    elif next_c == 116:  # t
     builder.append("\t")
    elif next_c == 34:  # "
     builder.append("\"")
    elif next_c == 92:  # \
     builder.append("\\")
   continue
  elif c == 34:  # "
   in_quotes = False
  else:
   builder.append(c_char)
 
 if quit_pos != -1:
  result.pos = quit_pos
 else:
  result.pos = len_txt
 
 result.txt = "".join(builder)
 return result

def task():
 
 if not os.path.isfile("README.md"):
  print("README.md not found.")
  return False
 
 destination_files_min_datetime = get_mtime("ggxrd_hitbox_overlay.ini")
 if destination_files_min_datetime is not None:
  destination_files_min_datetime_candidate = get_mtime("README.md")
  if destination_files_min_datetime_candidate < destination_files_min_datetime:
   destination_files_min_datetime = destination_files_min_datetime_candidate
 
 source_files_max_datetime = None
 source_files_datetimes = [
  get_mtime(os.path.join("ggxrd_hitbox_overlay", "KeyDefinitions.h")),
  get_mtime(os.path.join("ggxrd_hitbox_overlay", "SettingsDefinitions.h")),
  get_mtime(os.path.join("ggxrd_hitbox_overlay", "SettingsTopCommentDefinition.cpp"))
 ]
 for dt in source_files_datetimes:
  if source_files_max_datetime is None or dt is not None and dt > source_files_max_datetime:
   source_files_max_datetime = dt
 
 if source_files_max_datetime is not None \
    and destination_files_min_datetime is not None \
    and destination_files_min_datetime >= source_files_max_datetime:
  return True
 
 # \r char gets removed
 with open(os.path.join("ggxrd_hitbox_overlay", "KeyDefinitions.h"), "rt") as f:
  key_definitions = f.read()
 with open(os.path.join("ggxrd_hitbox_overlay", "SettingsDefinitions.h"), "rt") as f:
  settings_definitions = f.read()
 with open(os.path.join("ggxrd_hitbox_overlay", "SettingsTopCommentDefinition.cpp"), "rt") as f:
  settings_top_comment = f.read()
 
 search_str = "#define keyEnum \\"
 pos = key_definitions.find(search_str)
 if pos == -1:
  print(f"{search_str} not found in KeyDefinitions.h")
  return False
 
 pos += len(search_str)
 
 key_names = []
 while True:
  pos = key_definitions.find("\"", pos)
  if pos == -1:
   break
  pos += 1
  pos_start = pos
  pos = key_definitions.find("\"", pos)
  if pos == -1:
   break
  
  pos_end = pos
  pos = key_definitions.find("\n", pos + 1)
  
  key_names.append(key_definitions[pos_start : pos_end])
  if pos == -1:
   break
  pos += 1
 
 key_names = ", ".join(key_names)
 
 
 regex = re.compile(r'const\s+char\s*\*\s*settingsTopComment\s*=\s*"')
 match = regex.search(settings_top_comment)
 if not match:
  print("const char* settingsTopComment definition not found in SettingsTopCommentDefinition.cpp.")
  return False
 
 parsed = parse_c_string(settings_top_comment, match.span()[1] - 1)
 if parsed.pos == len(settings_top_comment):
  print("Couldn't find the end of the first part of settingsTopComment in SettingsTopCommentDefinition.cpp.")
  return False
 
 settings_top_comment_part1 = parsed.txt
 
 search_str = "keyEnum"
 if not (
    parsed.pos + len(search_str) <= len(settings_top_comment) \
    and settings_top_comment[parsed.pos : parsed.pos + len(search_str)] == search_str
 ):
  print("Couldn't find the mention of keyEnum in settingsTopComment in SettingsTopCommentDefinition.cpp.")
  return False
 
 pos = parsed.pos + len(search_str)
 pos = settings_top_comment.find("\"", pos)
 if pos == -1:
  print("Couldn't find the continuation of the string declaration after the mention of keyEnum in settingsTopComment in SettingsTopCommentDefinition.cpp.")
  return False
  
 parsed = parse_c_string(settings_top_comment, pos)
 if parsed.pos == len(settings_top_comment):
  print("Couldn't find the end of the second part of settingsTopComment in SettingsTopCommentDefinition.cpp.")
  return False
 
 if settings_top_comment[parsed.pos] != ";":
  print("The end of the second part of settingsTopComment is not ; in SettingsTopCommentDefinition.cpp.")
  return False

 settings_top_comment = settings_top_comment_part1 + key_names + parsed.txt
 
 new_ini = settings_top_comment.splitlines()
 
 line_start = 0
 line_start_next = 0
 pos = -1
 is_first = True
 len_settings_definitions = len(settings_definitions)
 parsed = ParseCStringResult(len_settings_definitions, "")
 while line_start < len_settings_definitions:
  if is_first:
   is_first = False
  else:
   shortcut_pos = -1
   if pos != -1:
    shortcut_pos = pos
   
   if parsed.pos != len_settings_definitions and (
       shortcut_pos == -1 or parsed.pos > shortcut_pos
      ):
    shortcut_pos = parsed.pos
   
   if shortcut_pos != -1 and shortcut_pos > line_start:
    line_start = shortcut_pos
   
   line_start = settings_definitions.find("\n", line_start)
   if line_start == -1:
    break
   
   line_start += 1
  
  # skip initial whitespace on the line (maybe skip ahead multiple lines)
  while line_start < len_settings_definitions and settings_definitions[line_start].isspace():
   line_start += 1
  
  if line_start >= len_settings_definitions:
   break
  
  # skip preprocessor macros
  if settings_definitions[line_start] == "#":
   continue
  
  next_line = settings_definitions.find("\n", line_start)
  if next_line == -1:
   next_line = len_settings_definitions
  
  open_brace_pos = settings_definitions.find("(", line_start)
  if open_brace_pos == -1 or open_brace_pos >= next_line:
   continue
  
  pos = open_brace_pos + 1
  macro_name = settings_definitions[line_start : open_brace_pos]
  line_start = pos
  if macro_name == "settingsKeyCombo":
   # find the , that is after the name
   pos = settings_definitions.find(",", pos + 1)
   if pos == -1:
    continue
   
   name = settings_definitions[open_brace_pos + 1 : pos]
   # find the opening " of the display name
   pos = settings_definitions.find("\"", pos + 1)
   if pos == -1:
    continue
   
   # parse the display name
   parsed = parse_c_string(settings_definitions, pos)
   if parsed.pos == len_settings_definitions:
    continue
   
   # the comma after the display name
   pos = parsed.pos
   if settings_definitions[pos] != ",":
    continue
   
   # find the opening " of the default value
   pos = settings_definitions.find("\"", pos + 1)
   if pos == -1:
    continue
   
   # parse the default value
   parsed = parse_c_string(settings_definitions, pos)
   if parsed.pos == len_settings_definitions:
    continue
   
   default_value = parsed.txt
   # the comma after the default value
   pos = parsed.pos
   if settings_definitions[pos] != ",":
    continue
   
   # find the opening " of the description
   pos = settings_definitions.find("\"", pos + 1)
   if pos == -1:
    continue
   
   # parse the description
   parsed = parse_c_string(settings_definitions, pos)
   if parsed.pos == len_settings_definitions:
    continue
   
   if settings_definitions[parsed.pos] != ")":
    continue
   
   new_ini.append("")
   new_ini += parsed.txt.splitlines()
   new_ini.append(f"{name} = {default_value}")
   
   # stop at the closing )
   pos = parsed.pos
   
  elif macro_name in ("settingsField", "settingsFieldWithInlineComment"):
   # find the , that is after the type
   pos = settings_definitions.find(",", pos + 1)
   if pos == -1:
    continue
   
   # parse the type
   typename = settings_definitions[open_brace_pos + 1 : pos]
   
   # skip the , after the type
   pos += 1
   
   # skip the whitespace before the name
   while pos < len_settings_definitions and settings_definitions[pos].isspace():
    pos += 1
   
   name_start = pos
   
   # find the , after the name
   pos = settings_definitions.find(",", pos + 1)
   if pos == -1:
    continue
   
   # parse the name
   name = settings_definitions[name_start : pos]
   
   # skip the whitespace before the default value
   pos += 1
   while pos < len_settings_definitions and settings_definitions[pos].isspace():
    pos += 1
   
   value_start = pos
   
   # find the , after the default value
   pos = settings_definitions.find(",", pos + 1)
   if pos == -1:
    continue
   
   # parse the value
   value = settings_definitions[value_start : pos]
   
   # transform the value to the INI format
   if typename == "ScreenshotPath":
    if len(value) >= 2 and value[0] == "\"" and value[-1] == "\"":
     value_parsed = parse_c_string(value, 0)
     if value_parsed.pos != len(value):
      value = value_parsed.txt
     else:
      value = ""
    
   elif typename == "color":
    if len(value) >= 2 and value.startswith("0x"):
     value = value[2:]
    
   elif typename == "float":
    if value and (value[-1] == "F" or value[-1] == "f"):
     value = value[0:-1]
    
    if value and value[-1] == ".":
     value += "0"
    elif value.find(".") == -1:
     value += ".0"
    elif value and value[0] == ".":
     value = "0" + value
    
   elif typename == "HitboxList":
    v_pos = value.find("\"")
    if v_pos != -1:
     parsed = parse_c_string(value, v_pos)
     value = parsed.txt
    
   elif typename == "MoveList" or typename == "PinnedWindowList":
    value = ""
   
   # find the opening " of the display name
   pos = settings_definitions.find("\"", pos + 1)
   if pos == -1:
    continue
   
   # parse the display name
   parsed = parse_c_string(settings_definitions, pos)
   if parsed.pos == len_settings_definitions:
    continue
   
   pos = parsed.pos
   # should be a , after the display name
   if settings_definitions[pos] != ",":
    continue
   
   # skip the comma
   pos += 1
   
   # skip the whitespace before the section
   while pos < len_settings_definitions and settings_definitions[pos].isspace():
    pos += 1
   
   # section can be either a quoted string or a variable's identifier
   if pos < len_settings_definitions and settings_definitions[pos] == "\"":
    # section is a string. Parse it
    parsed = parse_c_string(settings_definitions, pos)
    if parsed.pos == len_settings_definitions:
     continue
    
    pos = parsed.pos
    # should be a comma after the string
    if settings_definitions[pos] != ",":
     continue
    
   else:
    # find the , after the identifier
    pos = settings_definitions.find(",", pos + 1)
    if pos == -1:
     continue
    
   # find the opening " of the description
   pos = settings_definitions.find("\"", pos + 1)
   if pos == -1:
    continue
   
   # parse the description
   parsed = parse_c_string(settings_definitions, pos)
   if parsed.pos == len_settings_definitions:
    continue
   
   desc = parsed.txt
   # after the description, we're either at a ) or at a comma
   pos = parsed.pos
   if settings_definitions[pos] == ")":
    inline_comment = ""
   elif settings_definitions[pos] == ",":
    # find the opening " of the inline comment
    pos = settings_definitions.find("\"", pos + 1)
    if pos == -1:
     continue
    
    # parse the inline comment
    parsed = parse_c_string(settings_definitions, pos)
    if parsed.pos == len_settings_definitions:
     continue
    
    inline_comment = parsed.txt
    pos = parsed.pos
    if settings_definitions[pos] != ")":
     continue
    
   else:
    continue
   
   new_ini.append("")
   new_ini += desc.splitlines()
   if inline_comment:
    new_ini.append(f"{name} = {value} {inline_comment}")
   else:
    new_ini.append(f"{name} = {value}")
   
 if os.path.isfile("ggxrd_hitbox_overlay.ini"):
  os.remove("ggxrd_hitbox_overlay.ini")
 
 with open("ggxrd_hitbox_overlay.ini", "wt", encoding="utf-8", newline="\n") as f:
  for line in new_ini:
   f.write(line)
   f.write("\n")
 
 with open("New_README.md", "wt", encoding="utf-8", newline="\n") as f:
  
  inside_ini = False
  with open("README.md", "rt") as rf:
   last_line = False
   while not last_line:
    line = rf.readline()
    last_line = not line or line[-1] != "\n"
    if not inside_ini:
     f.write(line)
     if line == "```ini\n":
      inside_ini = True
      for ini_line in new_ini:
       f.write(ini_line)
       f.write("\n")
    elif line == "```\n":
     f.write(line)
     inside_ini = False
 
 os.remove("README.md")
 os.rename("New_README.md", "README.md")
 return True
