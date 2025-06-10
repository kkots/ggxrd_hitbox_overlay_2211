
.MODEL flat

.code

; x86 C/C++ supports inline asm - we don't need a separate .asm file. But what if it inserts some mov ecx,[securityCookie] at the start of the function, before my asm block? How do I read incoming ecx? So I use this instead

; reference to a C function declared in C code - in EndScene.cpp, called "drawQuadExecHook"
; It's a cdecl with 3 args
extrn _drawQuadExecHook:proc

; reference to a C function declared in C code - in EndScene.cpp, called "increaseStunHook"
; It's a cdecl with 2 args
extrn _increaseStunHook:proc

; a __fastcall defined in EndScene.cpp, called jumpInstallNormalJumpHook. It's got name decorations because it's __fastcall
extrn @jumpInstallNormalJumpHook@4:proc
extrn @jumpInstallSuperJumpHook@4:proc

extern _restoreDoubleJumps:dword
extern _restoreAirDash:dword

; a cdecl with 1 arg that returns BOOL, called drawWinsHook, defined in Game.cpp.
; Returns FALSE if you need to jump over and TRUE if you should execute the wins-drawing code
extrn _drawWinsHook:proc

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

_increaseStunHookAsm proc
	PUSH ECX
	PUSH EDX
	PUSH EAX
	PUSH EAX
	PUSH ESI
	call _increaseStunHook
	ADD ESP,08h
	POP EAX
	ADD dword ptr [ESI + 09fc4h],EAX
	POP EDX
	POP ECX
	RET
_increaseStunHookAsm endp

_jumpInstallNormalJumpHookAsm proc
	PUSH ECX
	CALL @jumpInstallNormalJumpHook@4
	POP ECX
	CALL dword ptr[_restoreDoubleJumps]
	RET
_jumpInstallNormalJumpHookAsm endp

_jumpInstallSuperJumpHookAsm proc
	PUSH ECX
	CALL @jumpInstallSuperJumpHook@4
	POP ECX
	CALL dword ptr[_restoreAirDash]
	RET
_jumpInstallSuperJumpHookAsm endp

_drawWinsHookAsm proc
	PUSH EBX
	CALL _drawWinsHook
	ADD ESP,4h
	RET
	; EAX preserved, we return what _drawWinsHook returned
_drawWinsHookAsm endp

end
