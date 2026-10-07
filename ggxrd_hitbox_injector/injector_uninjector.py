import win32api
import win32event
import win32gui
import win32process
import pywintypes
from pathlib import Path
import ctypes
from ctypes import wintypes
import struct

# Returns a str in case of error.
# Returns True in case of success.
# May throw exceptions due to running win32gui/api/process functions, run in a try.
def inject_dll(dll_path, /, uninject=False):
 return inject_multiple_dlls([dll_path], uninject=uninject)

# Returns a str in case of error.
# Returns True in case of success.
# May throw exceptions due to running win32gui/api/process functions, run in a try.
def inject_multiple_dlls(list_of_dll_paths, /, uninject=False):
 window_handle = win32gui.FindWindow("LaunchUnrealUWindowsClient", "Guilty Gear Xrd -REVELATOR-")
 if window_handle == 0:
  return "Guilty Gear Xrd is not open."
 thread_id, process_id = win32process.GetWindowThreadProcessId(window_handle)
 PROCESS_ALL_ACCESS = 0x1FFFFF
 process_handle = win32api.OpenProcess(PROCESS_ALL_ACCESS, 0, process_id)
 if not process_handle:
  return "Failed to open Guilty Gear Xrd's process."
 LIST_MODULES_32BIT = 1
 
 def find_module(name):
  name_upper = name.upper()
  if not name_upper.endswith(".DLL"):
   name += ".DLL"
   name_upper += ".DLL"
  
  for module_handle in win32process.EnumProcessModulesEx(process_handle, LIST_MODULES_32BIT):
   module_file_path = win32process.GetModuleFileNameEx(process_handle, module_handle)
   module_name = Path(module_file_path).name
   if module_name.upper() == name_upper:
    return module_handle
  return 0
 
 modules_to_operate_on = []
 for dll_path in list_of_dll_paths:
  found = find_module(Path(dll_path).name)
  
  if uninject:
   if found == 0:
    continue
   modules_to_operate_on.append((dll_path,found))
  elif found == 0:
   modules_to_operate_on.append(dll_path)
 
 if not modules_to_operate_on:
   if len(list_of_dll_paths) == 1:
    if uninject:
     return "Module is already absent from the game."
    else:
     return "Module is already injected."
   elif uninject:
    return "All modules have already been uninjected."
   else:
    return "All modules have already been injected."
 
 kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
 if not uninject:
  VirtualAllocEx = kernel32.VirtualAllocEx
  VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
  VirtualAllocEx.restype = wintypes.LPVOID
  MEM_RESERVE = 0x00002000
  MEM_COMMIT = 0x00001000
  PAGE_READWRITE = 4
  
  longest_binary_string_len = 0
  for dll_path in modules_to_operate_on:
   binary_string = dll_path.encode("utf-16le") + b"\x00\x00"
   binary_string_len = len(binary_string)
   if binary_string_len > longest_binary_string_len:
    longest_binary_string_len = binary_string_len
  
  allocated_mem = VirtualAllocEx(int(process_handle), 0, longest_binary_string_len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
  if not allocated_mem:
   return "Failed to allocate memory inside of Guilty Gear Xrd."
  
  MEM_RELEASE = 0x8000
  VirtualFreeEx = kernel32.VirtualFreeEx
  VirtualFreeEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD]
  VirtualFreeEx.restype = wintypes.BOOL
  
  WriteProcessMemory = kernel32.WriteProcessMemory
  WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.PDWORD]
  WriteProcessMemory.restype = wintypes.BOOL
 
 ReadProcessMemory = kernel32.ReadProcessMemory
 ReadProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.PDWORD]
 ReadProcessMemory.restype = wintypes.BOOL
 NULL_LPVOID = wintypes.PDWORD(ctypes.c_ulong(0))
 
 def find_imported_function(process_handle, dll_name, function_name):
  dll_name_uppercase = dll_name.upper()
  if not dll_name_uppercase.endswith(".DLL"):
   dll_name += ".DLL"
   dll_name_uppercase += ".DLL"
  try:
   process_base = win32process.EnumProcessModulesEx(process_handle, LIST_MODULES_32BIT)[0]
   
   class ReadProcessMemoryException(Exception):
    pass
   
   def read_dword(addr):
    buf = b"\x00\x00\x00\x00"
    if ReadProcessMemory(int(process_handle), addr, buf, 4, NULL_LPVOID) == 0:
     raise ReadProcessMemoryException
    return struct.unpack("<I", buf)[0]
   
   pe_header_start_offset = process_base + read_dword(process_base + 0x3c)
   imports_data_directory_rva_and_size = pe_header_start_offset + 0x80
   imports_size = read_dword(imports_data_directory_rva_and_size + 4)
   imports_rva = read_dword(imports_data_directory_rva_and_size)
   import_ptr_next = process_base + imports_rva
   foreign_name = b"\x00" * (len(dll_name) + 1)
   while imports_size > 0:
    imports_size -= 0x14
    import_ptr = import_ptr_next
    import_ptr_next += 0x14
    import_lookup_table_rva = read_dword(import_ptr)
    if not import_lookup_table_rva:
     break
    name_rva = read_dword(import_ptr + 0xc)
    foreign_name_addr = process_base + name_rva
    if ReadProcessMemory(int(process_handle), foreign_name_addr, foreign_name, len(foreign_name), NULL_LPVOID) == 0:
     return 0
    if foreign_name[-1] != 0:
     continue
    foreign_name_decoded = foreign_name[0:-1].decode("utf-8")
    if foreign_name_decoded.upper() != dll_name_uppercase:
     continue
    
    import_address_table_rva = read_dword(import_ptr + 0x10)
    func_ptr_next = process_base + import_address_table_rva
    image_import_by_name_rva_ptr_next = process_base + import_lookup_table_rva
    foreign_name = b"\x00" * (len(function_name) + 1)
    while True:
     func_ptr = func_ptr_next
     func_ptr_next += 4
     image_import_by_name_rva_ptr = image_import_by_name_rva_ptr_next
     image_import_by_name_rva_ptr_next += 4
     image_import_by_name_rva = read_dword(image_import_by_name_rva_ptr)
     if image_import_by_name_rva == 0:
      break
     import_by_name = process_base + image_import_by_name_rva
     if ReadProcessMemory(int(process_handle), import_by_name + 2, foreign_name, len(foreign_name), NULL_LPVOID) == 0:
      return 0
     if foreign_name[-1] != 0:
      continue
     foreign_name_decoded = foreign_name[0:-1].decode("utf-8")
     if foreign_name_decoded != function_name:
      continue
     return read_dword(func_ptr)
    break
   return 0
  except ReadProcessMemoryException:
   return 0
 
 if uninject:
  remote_FreeLibrary = find_imported_function(process_handle, "kernel32", "FreeLibrary")
  if remote_FreeLibrary == 0:
   return "Failed to find FreeLibrary in Guilty Gear Xrd."
 else:
  remote_LoadLibraryW = find_imported_function(process_handle, "kernel32", "LoadLibraryW")
  if remote_LoadLibraryW == 0:
   VirtualFreeEx(int(process_handle), allocated_mem, 0, MEM_RELEASE)
   return "Failed to find LoadLibraryW in Guilty Gear Xrd."
 
 for dll_path_or_pair in modules_to_operate_on:
  if uninject:
   dll_path = dll_path_or_pair[0]
   thread_handle, thread_id = win32process.CreateRemoteThread(process_handle, pywintypes.SECURITY_ATTRIBUTES(), 0, remote_FreeLibrary, dll_path_or_pair[1], 0)
  else:
   dll_path = dll_path_or_pair
   binary_string = dll_path.encode("utf-16le") + b"\x00\x00"
   if WriteProcessMemory(int(process_handle), allocated_mem, binary_string, len(binary_string), NULL_LPVOID) == 0:
    VirtualFreeEx(int(process_handle), allocated_mem, 0, MEM_RELEASE)
    return "Failed to write a string into Guilty Gear Xrd's memory."
   
   thread_handle, thread_id = win32process.CreateRemoteThread(process_handle, pywintypes.SECURITY_ATTRIBUTES(), 0, remote_LoadLibraryW, allocated_mem, 0)
   
  if thread_handle == 0:
   if not uninject:
    VirtualFreeEx(int(process_handle), allocated_mem, 0, MEM_RELEASE)
   return "Failed to start a new thread inside of Guilty Gear Xrd."
  
  WAIT_OBJECT_0 = 0
  WAIT_FAILED = 0xFFFFFFFF
  wait_result = win32event.WaitForSingleObject(thread_handle, 0xFFFFFFFF)
  if wait_result == WAIT_OBJECT_0:
   exit_code = win32process.GetExitCodeThread(thread_handle)
   if uninject:
    if exit_code != 0:
     retval = True
    else:
     retval = f"Failed to uninject {Path(dll_path).name}."
   else:
    found_addr = find_module(Path(dll_path).name)
    if exit_code == found_addr:
     retval = True
    else:
     retval = f"Exit code of the remote thread is incorrect: 0x{exit_code:x}, while injected {Path(dll_path).name}'s address is 0x{found_addr:x}. Injection may have failed."
  else:
   retval = "Failed to wait for the injection thread to finish. Injection may have failed."
  
  if retval != True:
   if not uninject:
    VirtualFreeEx(int(process_handle), allocated_mem, 0, MEM_RELEASE)
   return retval
  
  if not uninject:
   if not dll_path.upper().endswith(".DLL"):
    dll_path += ".DLL"
   
   def find_run_init():
    with open(dll_path, "rb") as f:
     image_dos_header = f.read(0x40)
     IMAGE_DOS_SIGNATURE = 0x5A4D
     
     def read_ushort(data, offset):
      return struct.unpack("<H", data[offset:offset+2])[0]
     
     if read_ushort(image_dos_header, 0) != IMAGE_DOS_SIGNATURE:
      return
     
     def read_dword(data, offset):
      return struct.unpack("<I", data[offset:offset+4])[0]
     
     e_lfanew = read_dword(image_dos_header, 0x3c)
     f.seek(e_lfanew, 0)
     IMAGE_NT_SIGNATURE = 0x4550
     nt_header = f.read(0xf8)  # sizeof(IMAGE_NT_HEADERS32)
     if read_dword(nt_header, 0) != IMAGE_NT_SIGNATURE:
      return 0
     
     number_of_sections = read_ushort(nt_header, 4 + 2)  # IMAGE_NT_HEADER32::.FileHeader::NumberOfSections
     OFFSET_OF_OPTIONAL_HEADER = 0x18
     OFFSET_OF_IMAGE_BASE = 0x1c
     OFFSET_OF_DATA_DIRECTORY = 0x60
     SIZE_OF_DATA_DIRECTORY = 0x8  # in directory, 0 is va, 4 is size
     f.seek(e_lfanew + OFFSET_OF_OPTIONAL_HEADER + read_ushort(nt_header, 0x14), 0)
     SIZE_OF_IMAGE_SECTION_HEADER = 0x28
     sections_binary = f.read(SIZE_OF_IMAGE_SECTION_HEADER * number_of_sections)
     sections = []
     for i in range(0, number_of_sections):
      sections.append({
       "raw": read_dword(sections_binary, i * SIZE_OF_IMAGE_SECTION_HEADER + 0x14),
       "rva": read_dword(sections_binary, i * SIZE_OF_IMAGE_SECTION_HEADER + 0xc)
      })
     image_base = read_dword(nt_header, OFFSET_OF_OPTIONAL_HEADER + OFFSET_OF_IMAGE_BASE)
     
     def raw_to_rva(raw):
      for section in reversed(sections):
       if raw >= section["raw"]:
        return raw - section["raw"] + section["rva"]
      return 0
     
     def rva_to_raw(rva):
      for section in reversed(sections):
       if rva >= section["rva"]:
        return rva - section["rva"] + section["raw"]
      return 0
     
     def raw_to_va(raw):
      return raw_to_rva(raw) + image_base
     
     def va_to_raw(va):
      return rva_to_raw(va - image_base)
     
     IMAGE_DIRECTORY_ENTRY_EXPORT = 0
     f.seek(
      rva_to_raw(
       read_dword(nt_header,
        OFFSET_OF_OPTIONAL_HEADER
        + OFFSET_OF_DATA_DIRECTORY
        + IMAGE_DIRECTORY_ENTRY_EXPORT * SIZE_OF_DATA_DIRECTORY
       )
      ), 0)
     dir_size_remaining = read_dword(nt_header,
      OFFSET_OF_OPTIONAL_HEADER
      + OFFSET_OF_DATA_DIRECTORY
      + IMAGE_DIRECTORY_ENTRY_EXPORT * SIZE_OF_DATA_DIRECTORY
      + 4
     )
     if dir_size_remaining == 0:
      return 0
     SIZE_OF_IMAGE_EXPORT_DIRECTORY = 0x28
     export_dir = f.read(SIZE_OF_IMAGE_EXPORT_DIRECTORY)
     OFFSET_OF_NUMBER_OF_FUNCTIONS = 0x14
     OFFSET_OF_NUMBER_OF_NAMES = 0x18
     OFFSET_OF_ADDRESS_OF_FUNCTIONS = 0x1c
     OFFSET_OF_ADDRESS_OF_NAMES = 0x20
     OFFSET_OF_ADDRESS_OF_NAME_ORDINALS = 0x24
     num_names_counter = read_dword(export_dir, OFFSET_OF_NUMBER_OF_NAMES)
     num_functions_counter = read_dword(export_dir, OFFSET_OF_NUMBER_OF_FUNCTIONS)
     current_lookup_index = 0
     while num_names_counter >= 0 and num_functions_counter >= 0:
      num_names_counter -= 1
      num_functions_counter -= 1
      
      f.seek(
       rva_to_raw(
        read_dword(export_dir, OFFSET_OF_ADDRESS_OF_NAMES) + 4 * current_lookup_index
       ), 0)
      
      name_rva = struct.unpack("<I", f.read(4))[0]
      f.seek(rva_to_raw(name_rva), 0)
      chars = bytearray()
      while True:
       next_char = f.read(1)[0]
       if next_char == 0:
        break
       chars.append(next_char)
      chars = chars.decode("utf-8")
      if chars == "RunInitThread":
       f.seek(
        rva_to_raw(
         read_dword(export_dir, OFFSET_OF_ADDRESS_OF_NAME_ORDINALS + 2 * current_lookup_index)
        ), 0)
       ordinal_unbiased = struct.unpack("<H", f.read(2))[0]
       f.seek(
        rva_to_raw(
         read_dword(export_dir, OFFSET_OF_ADDRESS_OF_FUNCTIONS + 4 * ordinal_unbiased)
        ), 0)
       return struct.unpack("<I", f.read(4))[0]  # RVA, relative to DLL's base
       
      current_lookup_index += 1
    return 0
 
 if not uninject:
  run_init_rva = find_run_init()
  if run_init_rva != 0:
   thread_handle, thread_id = win32process.CreateRemoteThread(process_handle, pywintypes.SECURITY_ATTRIBUTES(), 0, found_addr + run_init_rva, 0, 0)
   if thread_handle == 0:
    retval = "Failed to call RunInitThread on the mod. It probably failed to initialize."
   else:
    wait_result = win32event.WaitForSingleObject(thread_handle, 0xFFFFFFFF)
    if wait_result == WAIT_OBJECT_0:
     retval = True
    else:
     retval = "Failed to wait for RunInitThread to finish working inside the mod. It probably failed to initialize."
 
 if not uninject:
  VirtualFreeEx(int(process_handle), allocated_mem, 0, MEM_RELEASE)
 return retval

# Returns a str in case of error.
# Returns True in case of success.
# May throw exceptions due to running win32gui/api/process functions, run in a try.
def uninject_dll(dll_path):
 return inject_dll(dll_path, uninject=True)

# Returns a str in case of error.
# Returns True in case of success.
# May throw exceptions due to running win32gui/api/process functions, run in a try.
def uninject_multiple_dlls(list_of_dll_paths):
 return inject_multiple_dlls(list_of_dll_paths, uninject=True)

def check_xrd_open():
 return win32gui.FindWindow("LaunchUnrealUWindowsClient", "Guilty Gear Xrd -REVELATOR-") != 0
