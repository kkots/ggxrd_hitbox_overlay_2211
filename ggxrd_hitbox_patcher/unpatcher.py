import os
import struct

# guilty_gear_xrd_exe_path - full path to GuiltyGearXrd.exe.
# Raises Exception if the given GuiltyGearXrd.exe is not a valid EXE or unpatching failed.
# Returns nothing
def unpatch_hitbox_overlay(guilty_gear_xrd_exe_path):
  debug = False
  hardcoded_patch_place_raw = 0x970126
  with open(guilty_gear_xrd_exe_path, "r+b") as file:
    file.seek(hardcoded_patch_place_raw)
    if file.read(1) != b'\xe9':
      if debug:
        print("Is not patched.")
      # isn't patched, nothing to undo
      return
    file.seek(0)
    dos_magic = file.read(2)
    if dos_magic != b"MZ":
      raise Exception("Not a valid EXE.")
    file.seek(0x3c)
    addr = file.read(4)
    nt_header_off = struct.unpack("<I", addr)[0]
    file.seek(nt_header_off)
    nt_magic = file.read(4)
    if nt_magic != b"PE\x00\x00":
      raise Exception("Not a valid EXE.")
    file.seek(nt_header_off + 0x34)
    image_base = struct.unpack("<I", file.read(4))[0]
    file.seek(nt_header_off + 6)
    num_sections = struct.unpack("<H", file.read(2))[0]
    file.seek(nt_header_off + 0x14)
    size_of_optional_header = struct.unpack("<H", file.read(2))[0]
    section_header_off = nt_header_off + 0x18 + size_of_optional_header
    
    sections = []
    class Section:
      def __init__(self, name, start_rva, start_raw):
        self.name = name
        self.start_rva = start_rva
        self.start_raw = start_raw
      def __repr__(self):
        return "(" + self.name + "; RVA: " + hex(self.start_rva) + "; RAW: " + hex(self.start_raw) + ")"
        
    def raw_to_rva(raw_addr):
      for section in reversed(sections):
        if raw_addr >= section.start_raw:
          return raw_addr - section.start_raw + section.start_rva
      return 0
        
    def rva_to_raw(rva_addr):
      for section in reversed(sections):
        if rva_addr >= section.start_rva:
          return rva_addr - section.start_rva + section.start_raw
      return 0
        
    def raw_to_va(raw_addr):
      for section in reversed(sections):
        if raw_addr >= section.start_raw:
          return raw_addr - section.start_raw + section.start_rva + image_base
      return 0
        
    def va_to_raw(va_addr):
      return rva_to_raw(va_addr - image_base)
    
    for section_ind in range(0,num_sections):
      file.seek(section_header_off)
      section_name = file.read(8)
      section_name_length = 0
      for i in range(0,len(section_name)):
        byte_value = section_name[i]
        if byte_value == 0:
          break
        section_name_length += 1
      section_name_utf8 = section_name[0:section_name_length].decode("utf-8")
      file.seek(section_header_off + 0xc)
      section_rva = struct.unpack("<I", file.read(4))[0]
      file.seek(section_header_off + 0x10)
      section_raw_size = struct.unpack("<I", file.read(4))[0]
      file.seek(section_header_off + 0x14)
      section_raw = struct.unpack("<I", file.read(4))[0]
      sections.append(Section(section_name_utf8, section_rva, section_raw))
      section_header_off += 0x28
    
    if debug:
      print(sections)
    
    file.seek(hardcoded_patch_place_raw + 1)
    off = struct.unpack("<i", file.read(4))[0]
    patch_place_va = raw_to_va(hardcoded_patch_place_raw)
    patch_code_va = patch_place_va + 5 + off
    if debug:
      print("patch_code_va: 0x%x" % patch_code_va)
    file.seek(va_to_raw(patch_code_va))
    if file.read(1) != b'\xe8':
      raise Exception("The patch code doesn't start with a call.")
    off = struct.unpack("<i", file.read(4))[0]
    orig_func_va = patch_code_va + 5 + off
    if debug:
      print("orig func: 0x%x" % orig_func_va)
    if file.read(1) != b'\x68':
      raise Exception("The patch code doesn't have a PUSH after the CALL.")
    string_addr_va = struct.unpack("<I", file.read(4))[0]
    if debug:
      print("string addr: 0x%x" % orig_func_va)
    string_len = 0
    
    old_pos = file.tell()
    file.seek(va_to_raw(string_addr_va))
    while string_len < 1000:
      byte_array_of_size_one = file.read(1)
      if byte_array_of_size_one[0] != 0:
        string_len += 1
      else:
        break
    if debug:
      file.seek(va_to_raw(string_addr_va))
      print("string:", file.read(string_len).decode("utf-8"))
    file.seek(old_pos)
    
    if file.read(2) != b'\xff\x15':
      raise Exception("The patch code doesn't have a call through a pointer (presumably LoadLibrary).")
    file.seek(4, os.SEEK_CUR)
    if file.read(1) != b'\xe9':
      raise Exception("The patch code doesn't jump at the end.")
    off = struct.unpack("<i", file.read(4))[0]
    jump_from_va = patch_code_va + 16
    jump_dest_va = jump_from_va + 5 + off
    if jump_dest_va != patch_place_va + 5:
      raise Exception("The patch code doesn't jump back to the patching place.")
    
    total_patch_code_size = (5 # call
      + 5 # push string
      + 6 # ff 15 load library
      + 5)  # jump back
    
    file.seek(va_to_raw(string_addr_va))
    file.write(b'\xCC' * (string_len + 1))
    
    file.seek(hardcoded_patch_place_raw)
    file.write(b'\xe8')
    off = orig_func_va - (patch_place_va + 5)
    file.write(struct.pack("<i", off))
    
    file.seek(va_to_raw(patch_code_va))
    file.write(b'\xCC' * total_patch_code_size)
    
    file.seek(nt_header_off + 0xa0)
    reloc_rva = struct.unpack("<I", file.read(4))[0]
    reloc_size = struct.unpack("<I", file.read(4))[0]
    if debug:
      print("reloc rva: 0x%x; size: 0x%x" % (reloc_rva, reloc_size))
    
    file.seek(rva_to_raw(reloc_rva))
    
    relocs_in_patch_code = []
    class RelocBlock:
      def __init__(self, page_base_va, block_size, where_block_itself_raw):
        self.page_base_va = page_base_va
        self.block_size = block_size  # block_size includes the page base and block size
        self.where_block_itself_raw = where_block_itself_raw
      def __repr__(self):
        return ("(Page: " + hex(self.page_base_va) + "; Size: " + hex(self.block_size)
          + "; Block raw: " + hex(self.where_block_itself_raw) + ")")
    
    class RelocEntry:
      def __init__(self, entry_type, addr_being_relocated_va, where_entry_itself_raw):
        self.entry_type = entry_type
        self.addr_being_relocated_va = addr_being_relocated_va
        self.where_entry_itself_raw = where_entry_itself_raw
      def __repr__(self):
        return ("(Type: " + str(self.entry_type) + "; VA patch: " + hex(self.addr_being_relocated_va)
          + "; Entry raw: " + hex(self.where_entry_itself_raw) + ")")
    
    patch_code_va_end = patch_code_va + total_patch_code_size
    reloc_size_current = reloc_size
    while reloc_size_current > 0:
      where_block_raw = file.tell()
      page_base = struct.unpack("<I", file.read(4))[0]
      block_size = struct.unpack("<I", file.read(4))[0]
      block_size_current = block_size
      padding = (4 - block_size & 3) & 3
      reloc_size_current -= block_size + padding
      block_size_current -= 8
      new_block = None
      while block_size_current > 0:
        where_entry_raw = file.tell()
        entry = struct.unpack("<H", file.read(2))[0]
        entry_type = entry >> 12
        entry_off = entry & 0xFFF
        entry_va = page_base + image_base + entry_off
        if entry_va >= patch_code_va and entry_va < patch_code_va_end:
          if new_block is None:
            new_block = RelocBlock(page_base + image_base, block_size, where_block_raw)
          new_entry = RelocEntry(entry_type, page_base + image_base + entry_off, where_entry_raw)
          relocs_in_patch_code.append((new_block, new_entry))
        block_size_current -= 2
      if padding:
        file.seek(padding, os.SEEK_CUR)
    
    if debug:
      print(relocs_in_patch_code)
    
    unique_blocks = [x for x in set(x[0] for x in relocs_in_patch_code)]
    unique_blocks.sort(key=lambda x: x.where_block_itself_raw)
    for reloc_pair in relocs_in_patch_code:
      file.seek(reloc_pair[1].where_entry_itself_raw)
      file.write(b'\x00\x00')
    
    if unique_blocks:
      block = unique_blocks[-1]
      file.seek(block.where_block_itself_raw + 8)
      block_size_current = block.block_size
      padding = (4 - (block_size_current & 3)) & 3
      block_size_current -= 8
      null_count = 0
      if block_size_current > 0:
        total_supposed_entry_count = block_size_current // 2
      else:
        total_supposed_entry_count = 0
      while block_size_current > 0:
        if file.read(2) == b'\x00\x00':
          null_count += 1
        else:
          null_count = 0
        block_size_current -= 2
      if padding:
        file.seek(padding, os.SEEK_CUR)
      if debug:
        print(f"A spotted reloc block contains {null_count} null entries out of {total_supposed_entry_count}.")
      if null_count > 0:
        bytes_to_castrate_block_by = null_count * 2
        if null_count == total_supposed_entry_count:
          bytes_to_castrate_block_by += 8
        block_offset_in_reloc_section = raw_to_rva(block.where_block_itself_raw) - reloc_rva
        slack = reloc_size - (block_offset_in_reloc_section + block.block_size + padding)
        can_delete = True
        if slack > 0:
          can_delete = False
          file.seek(block.where_block_itself_raw + block.block_size + padding)
          if slack == sum(1 for x in file.read(slack) if x != 0):
            can_delete = True
        else:
          slack = 0
        if can_delete:
          if debug:
            print(f"Reduced reloc size by {bytes_to_castrate_block_by} bytes.")
          file.seek(nt_header_off + 0xa0 + 4)
          file.write(struct.pack("<I", reloc_size - bytes_to_castrate_block_by))
          file.seek(block.where_block_itself_raw)
          file.write(b'\x00' * 8)
    
    if debug:
      print("Unpatched successfully.")
