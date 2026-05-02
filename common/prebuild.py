import os
import re

# if you modify this file, you must modify prebuild.ps1 same way

def get_mtime(file):
 try:
  import os
  return os.stat(file).st_mtime
 except FileNotFoundError:
  return None

def compare_filetimes(fileLeft, fileRight):
 timeLeft = get_mtime(fileLeft)
 timeRight = get_mtime(fileRight)
 
 if timeLeft == timeRight:
  return 0
 elif timeLeft is None:
  return -1
 elif timeRight is None:
  return 1
 elif timeLeft > timeRight:
  return 1
 else:  # timeLeft < timeRight:
  return -1
 # timeLeft == timeRight already done earlier

def parse_version(filename):
 version = None
 with open(filename, "rt") as f:
  line_final = False
  while not line_final:
   line = f.readline()
   line_final = not line or line[-1] != '\n'
   if not line_final:
    line = line[0:-1]
   find_result = re.search(r'\s*#define\s+\S+\s+"([^"]+)"', line)
   if find_result:
    return find_result.group(1)
 
 if version is None:
  print(f"Couldn't parse version from {filename}")
  return None

# content_lines is an array of lines of text
# content_lines must not contain newlines at the end of each line
# new_version is the string with the version you want to substitute
# new_version must be in format 1.2
# returns True if substituion was needed
# returns False if it is not needed
# if needed, content_lines will change a line in it
def replace_about_dlg(content_lines, new_version):
 inside_dialog = False
 
 for i in range(0, len(content_lines)):
  line = content_lines[i]
  if re.match(r'\s*IDD_ABOUTBOX DIALOGEX', line) is not None:
   inside_dialog = True
  elif inside_dialog:
   match = re.match(r'(\s*LTEXT\s+"\S+\s+)([^"\s]+)"', line)
   if match:
    if match.group(2) != new_version:
     content_lines[i] = match.expand("\\g<1>" + new_version + "\"") + line[match.span()[1] : ]
     return True
    return False
 return False

# same as replace_about_dlg, but replaces IDS_APP_TITLE text
def replace_app_title(content_lines, new_version):
 for i in range(0, len(content_lines)):
  line = content_lines[i]
  match = re.match(r'(\s*IDS_APP_TITLE\s+"\S+\s+)([^"\s]+)"', line)
  if match:
   if match.group(2) != new_version:
    content_lines[i] = match.expand("\\g<1>" + new_version + "\"") + line[match.span()[1] : ]
    return True
   return False
 return False

# same as replace_about_dlg, but replaces:
# FileVersion,
# ProductVersion,
# FILEVERSION,
# PRODUCTVERSION;
# and new_version_four_digits must be the version string comprised of 4 numbers,
# separated by either . or ,
def replace_file_and_product_versions(content_lines, new_version_four_digits):
 need_write = False
 new_version_with_dots = new_version_four_digits.replace(",", ".")
 new_version_with_commas = new_version_four_digits.replace(".", ",")
 
 for i in range(0, len(content_lines)):
  line = content_lines[i]
  match = re.match(r'(\s*VALUE\s+"FileVersion",\s*")([^"\s]+)"', line)
  if not match:
   match = re.match(r'(\s*VALUE\s+"ProductVersion",\s*")([^"\s]+)"', line)
  if match:
   if match.group(2) != new_version_with_dots:
    need_write = True
    content_lines[i] = match.expand("\\g<1>" + new_version_with_dots + "\"") + line[match.span()[1] : ]
  else:
   match = re.match(r'(\s*FILEVERSION\s+)(\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+)', line)
   if not match:
    match = re.match(r'(\s*PRODUCTVERSION\s+)(\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+)', line)
   if match:
    if match.group(2).replace(" ", "").replace("\t", "") != new_version_with_commas:
     need_write = True
     content_lines[i] = match.expand("\\g<1>" + new_version_with_commas) + line[match.span()[1] : ]
 return need_write
