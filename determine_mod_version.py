import os
from os import path
import struct
import hashlib

# The only functions meant for use outside the module are:
# read_hitbox_overlay_dll_version,
# read_hitbox_injector_version,
# is_patched_exe,
# is_patched_bat

# dll_path - full path to ggxrd_hitbox_overlay.dll.
# Returns version in format "6.0".
# Returns None if failed to get the version.
# Raises an Exception in case the DLL is invalid.
def read_hitbox_overlay_dll_version(dll_path):
  ver = read_version_info(dll_path)
  if ver is None:
    return lookup_hash(dll_path, 1)
  ar = ver.split(".")
  if len(ar) != 4:
    raise Exception("Incorrect version format.")
  if ar[1] == "0" and ar[3] == "0":
    return ".".join((ar[0], ar[2]))
  else:
    return ".".join((ar[1], ar[3]))
    
# injector_exe_path - full path to ggxrd_hitbox_injector.exe.
# Returns version in format "6.0" for old versions or "mk1", "mk2" and so on for new versions.
# At some point the Injector changed its version numbering scheme to mk1/2/3/etc, hence the difference.
# Returns None if failed to get the version.
# Raises an Exception in case the EXE is invalid.
def read_hitbox_injector_version(injector_exe_path):
  ver = read_version_info(injector_exe_path)
  if ver is None:
    return lookup_hash(injector_exe_path, 2)
  if ver == "0.6.0.15":
    return "mk1"
  ar = ver.split(".")
  if len(ar) != 4:
    raise Exception("Incorrect version format.")
  return "mk" + ar[0]

# guilty_gear_exe_path - the full path to GuiltyGearXrd.exe.
# Returns whether ggxrd_hitbox_patcher.exe was ever used on the EXE or not.
# This function does not check if ggxrd_hitbox_overlay.dll is present in Binaries\Win32, which necessary for
# the patch to actually work.
def is_patched_exe(guilty_gear_exe_path):
  with open(guilty_gear_exe_path, "rb") as file:
    file.seek(0x970126)
    return file.read(1) == b'\xe9'

# bat_path - the full path to the BootGGXrd.bat
# This function does not determine the correctness of patching, only whether
# there're signs that patching may have been likely done to the .BAT file.
# This function does not check if ggxrd_hitbox_injector.exe is actually present in Binaries\Win32 folder,
# which is necessary for this method of patching to actually work.
# This function does not check if ggxrd_hitbox_overlay.dll is present in Binaries\Win32, which necessary for
# any patch to actually work.
def is_patched_bat(bat_path):
  with open(bat_path, "rt") as file:
    while True:
      line = file.readline()
      is_final = not line or line[-1] != "\n"
      if line and line[-1] == "\n":
        line = line[0: len(line) - 1]
      line = line.strip().replace("\t", " ")
      while line.find("  ") != -1:
        line = line.replace("  ", " ")
      line = line.lower()
      parts = line.split(" ")
      if parts[0] == "ggxrd_hitbox_injector" or len(parts) > 1 and parts[0] == "start" and parts[1] == "ggxrd_hitbox_injector":
        return True
      if is_final:
        break
  return False
  
hash_table = [
('1.0',
'f39364ddfe57859788ed579f6e607f59e3f14f83',
'6d18ec1e731a3b37f7ea8769a5d21c7e31992aef'),
('1.1',
'1d171f18d615d5fab54f3ee80112f2378ddad4b5',
'6d18ec1e731a3b37f7ea8769a5d21c7e31992aef'),
('1.2',
'332badd70236cc571924fb74695944d930a03342',
'80f0bd0751379f6267d1173a40097281dfc2e4dd'),
('1.3',
'dee040524600658a91e7952327b3673214eb46f2',
'80f0bd0751379f6267d1173a40097281dfc2e4dd'),
('1.4',
'12461ef04b4bdcdb7ae6b5bc97fc694fd18fe9f4',
'6a86ed1e167319f4ee30ece39c481bb3c3e3cce2'),
('1.5',
'd91e98afde7d8e6d1c766a923505046cd8b22485',
'6a86ed1e167319f4ee30ece39c481bb3c3e3cce2'),
('1.6',
'a9cca8e80f1c53b42397ae4e5ac8239c8f3b5be2',
'2026264378ecd07369e5a7d81e742c478ac67394'),
('1.7',
'0005c6e8597de7835b780868e85b7eefb194b9c6',
'2026264378ecd07369e5a7d81e742c478ac67394'),
('1.8',
'a8b55f8e6954acd9f6a6c8e10f7828bb18d22602',
'2026264378ecd07369e5a7d81e742c478ac67394'),
('1.9',
'be8ba4a92f540a804c88db87169912a0bd840bdd',
'2026264378ecd07369e5a7d81e742c478ac67394'),
('2.0',
'be8ba4a92f540a804c88db87169912a0bd840bdd',
'3009fc413a6bdb8fe669b31ee4f49ad2abd34f45'),
('2.1',
'901a1f742d229490911791feaaf376a6b4c7519b',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.2',
'88f1a935dacc1ccf94f4b0ca073608b1c6d37ad0',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.3',
'1c0c87cc8bf8e5bf9448fbd38f24294e392f70fd',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.4',
'744ebca88edac845fb611b4c3f9acb0142cbc151',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.7',
'afec0f0e5db3aab6970a694bff8ea1b5cdac58e2',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.8',
'2a551292721d1f66262ccdbed2be8539c362f0ba',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('2.9',
'54f4b05819a16f6f02b04b1ea838e7f9a53bbd57',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('3.0',
'cbcd63c26f3e1bfcbab3dbb07416da746cd5bc52',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('3.1',
'c7fb97a35ae1ba9cccf2bedf4327f52c5c0a232e',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('3.2',
'e81a3e6b77e7488bf2e8289a1a01af39725c237b',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('3.3',
'60f66c23039b19fb6dc485af3f3925fd0646c791',
'bfefbd43b2a655af4b15dd3cf2e036553156d3f8'),
('3.4',
'30be51e8ff20da9951c2f12d603f3deaec96b94d',
'34dc6777b17a756c92d8818dc52a24bd2b6ae4a2'),
('3.5',
'2fb6433a0ffa516f56f80cccf53b7417fe8db16e',
'34dc6777b17a756c92d8818dc52a24bd2b6ae4a2'),
('3.6',
'24e0aa7daf8f79d885b4a0b963736eada6d5398f',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('3.7',
'5b4f14b4ea791031327f9969b1b4dcc735d8579b',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('3.8',
'ddbb00183c1993518fa9dbf86efbeffc22e08442',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('3.9',
'522987d87ae9c7420dcf8074b398396deff28ab3',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.0',
'afa6dd8ba09a35c2309fe25ecfed5cc3de38dd0e',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.1',
'7b7a0d15595bea4098eeb8bf33417f83a3e8179d',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.2',
'b7bcf0ff9eafceb69427354a336700c35ac762fb',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.3',
'5d77aba2efe3517dd36163481f89105861ca93fa',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.4',
'6d0d0089470e445069e8a08473b8f64f111428fa',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.5',
'a686fed76c93b1155e1a19313ae42b44f66aa9bc',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('4.6',
'c4ab988a626156d6ede0da7071f7d7894f9a1f4a',
'd9cc38fc3666e29a5d0ca76288061dcab7992e1c'),
('5.0',
'f0204ce5e081a0e2c2d773467053fdaaef9624af',
'2973a4f3b84fe67c1f32da66e458af53e1155165'),
('6.0',
'7b10495c36daec839b7f416255fe5eb252e6d628',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.1',
'a2813cde23f628f7ce4cdb01e0adbc175b08740b',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.2',
'37758819acf165288babefe1be38e01fc438e6be',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.3',
'98941a2c253cb847bd7896ca153f128317fd49e7',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.4',
'a42c4fbc184cd018dad9822f15fe9ef5e80dba44',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.5',
'c3b96ca28406e75b1bce2b37459daa9c634ba6c8',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.6',
'7428ce6a88b300b68899d5fdb81e3318ecd26649',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.7',
'f13a5125058e3afd2d842e6822fdca9126d3536d',
'78bc2c001875abc0d07c2ad7b4e7552fee85dcaa'),
('6.8',
'54804160fc71ca37f6896d99f36c8efd593f6704',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.9',
'70a52d0b9e59c0276fb75816e6fa405ffa143e65',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.10',
'dbb063e800642f26ae4192ae9972e665eae369d7',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.11',
'4f781e9c227cf2e5e0f708c631ea87933d3192d4',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.12',
'4597dd384275df16a74afb9a7e415372e85a8adc',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.13',
'f579b9102a75bce6dcad8d94a1320273ddd9f30c',
'cab0145a1d88c5d315b3173b87697d3c5750bc34'),
('6.14',
'62e5fb1df95619754e66608140d82c6d517a6ee3',
'cab0145a1d88c5d315b3173b87697d3c5750bc34')
]

def hash_file(file_path):
  hasher = hashlib.sha1()
  size = path.getsize(file_path)
  with open(file_path, "rb") as file:
    while size > 0:
      if size >= 64:
        to_read = 64
        size -= 64
      else:
        to_read = size
        size = 0
      bytes = file.read(to_read)
      hasher.update(bytes)
  return hasher.hexdigest()

# obj_path - path to ggxrd_hitbox_overlay.dll or ggxrd_hitbox_injector.exe, or another EXE or DLL with a VS_VERSION_INFO resource.
# Returns the version string in format "1.0.1.0"
# On error raises exception.
# If there is no error, but VS_VERSION_INFO isn't found, returns None.
def read_version_info(obj_path):
  with open(obj_path, "rb") as file:
    dos_magic = file.read(2)
    if dos_magic != b"MZ":
      raise Exception("Not a valid DLL or EXE.")
    file.seek(0x3c)
    addr = file.read(4)
    nt_header_off = struct.unpack("<I", addr)[0]
    file.seek(nt_header_off)
    nt_magic = file.read(4)
    if nt_magic != b"PE\x00\x00":
      raise Exception("Not a valid DLL or EXE.")
    file.seek(nt_header_off + 6)
    num_sections = struct.unpack("<H", file.read(2))[0]
    file.seek(nt_header_off + 0x14)
    size_of_optional_header = struct.unpack("<H", file.read(2))[0]
    section_header_off = nt_header_off + 0x18 + size_of_optional_header
    for section_ind in range(0,num_sections):
      file.seek(section_header_off)
      section_name = file.read(8)
      if section_name == b'.rsrc\x00\x00\x00':
        file.seek(section_header_off + 0xc)
        section_rva = struct.unpack("<I", file.read(4))[0]
        file.seek(section_header_off + 0x14)
        section_addr = struct.unpack("<I", file.read(4))[0]  # raw
        file.seek(section_addr + 0xc)
        num_entries_by_name = struct.unpack("<H", file.read(2))[0]
        num_entries_by_id = struct.unpack("<H", file.read(2))[0]
        for top_entry_ind in range(0,num_entries_by_id):
          file.seek(section_addr + 0x10 + num_entries_by_name * 0x8 + top_entry_ind * 0x8 + 0x4)
          entry_off = struct.unpack("<I", file.read(4))[0]
          entry_is_dir = entry_off >> 31
          entry_off &= 0x7FFFFFFF
          entry_error = False
          while entry_is_dir:
            file.seek(section_addr + entry_off + 0xc)
            entry_subentry_num_by_name = struct.unpack("<H", file.read(2))[0]
            entry_subentry_num_by_id = struct.unpack("<H", file.read(2))[0]
            if entry_subentry_num_by_name != 0 or entry_subentry_num_by_id != 1:
              entry_error = True
              break
            file.seek(4, os.SEEK_CUR)  # the entry follows immediately, skip its first 4 bytes, which hold its id
            entry_off = struct.unpack("<I", file.read(4))[0]
            entry_is_dir = entry_off >> 31
            entry_off &= 0x7FFFFFFF
          if entry_error:
            continue
          file.seek(section_addr + entry_off)
          leaf_rva = struct.unpack("<i", file.read(4))[0]
          leaf_size = struct.unpack("<I", file.read(4))[0]
          file.seek(leaf_rva - section_rva + section_addr)
          vs_version_info_size = struct.unpack("<H", file.read(2))[0]
          if vs_version_info_size > 0x34:
            vs_version_info_initial_data = file.read(0x24)
            if vs_version_info_initial_data == b'\x34\x00\x00\x00V\x00S\x00_\x00V\x00E\x00R\x00S\x00I\x00O\x00N\x00_\x00I\x00N\x00F\x00O\x00\x00\x00':
              file.seek(2, os.SEEK_CUR)
              vs_version_info_signature_and_struct_version = file.read(8)
              if vs_version_info_signature_and_struct_version == b'\xbd\x04\xef\xfe\x00\x00\x01\x00':
                version_numbers = ["0"]*4
                for version_number_index in range(0,4):
                  cur_ver_num = struct.unpack("<H", file.read(2))[0]
                  version_numbers[version_number_index ^ 1] = str(cur_ver_num)
                return ".".join(version_numbers)
        break
      section_header_off += 0x28
      
def lookup_hash(file_path, tuple_element_index):
  file_hash = hash_file(file_path)
  for elem in hash_table:
    if elem[tuple_element_index] == file_hash:
      return elem[0]
  return None