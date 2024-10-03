
.MODEL flat

.code

; x86 C/C++ supports inline asm - we don't need a separate .asm file. But what if it inserts some mov ecx,[securityCookie] at the start of the function, before my asm block? How do I read incoming ecx? So I use this instead

; reference to a C function declared in C code - in EndScene.cpp, called "drawQuadExecHook"
; It's a cdecl with 3 args
extrn _drawQuadExecHook:proc

; caller clears stack. ecx - first arg, esp+4,esp+8 - second and third args
; Runs on the main thread
_drawQuadExecHookAsm proc
  push ebp
  lea ebp,[esp+4]
  mov eax,dword ptr[ebp+8]
  push eax
  mov eax,dword ptr[ebp+4]
  push eax
  push ecx
  call _drawQuadExecHook
  add esp,0Ch
  pop ebp
  ret
_drawQuadExecHookAsm endp

; cdecl esp+4 is the pointer to the original function,
;       esp+8 is the ecx arg
;       esp+0Ch is the first stack arg
;       esp+10h is the second stack arg
; Runs on the main thread
_call_orig_drawQuadExec proc
  mov edx,dword ptr[esp+4]
  mov ecx,dword ptr[esp+8]
  mov eax,dword ptr[esp+0Ch]
  mov dword ptr[esp+4],eax
  mov eax,dword ptr[esp+10h]
  mov dword ptr[esp+8],eax
  ; cdecl ecx - first arg, esp+4,esp+8 - second and third args
  jmp edx
_call_orig_drawQuadExec endp

end
