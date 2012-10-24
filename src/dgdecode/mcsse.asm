; mcsse.asm : i re-wrote (and re-macroized) the whole stuff in nasm - MarcFD 17 nov 2002

; *****************************************************************************
; * motionmmxext.c : MMX EXT motion compensation module for vlc
; *****************************************************************************
; * Copyright (C) 2001 VideoLAN
; * $Id: motionmmxext.c,v 1.16 2002/02/15 13:32:53 sam Exp $
; *
; * Authors: Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
; *          Michel Lespinasse <walken@zoy.org>
; *
; * This program is free software; you can redistribute it and/or modify
; * it under the terms of the GNU General Public License as published by
; * the Free Software Foundation; either version 2 of the License, or
; * (at your option) any later version.
; *
; * This program is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; * GNU General Public License for more details.
; *
; * You should have received a copy of the GNU General Public License
; * along with this program; if not, write to the Free Software
; * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
; *****************************************************************************/

; *****************************************************************************
; * mcsse.c : Created by vlad59 - 05/04/2002
; * Based on the VideoLan project
; *****************************************************************************
; * Change list :
; *
; * - Renamed from motionmmxext.c to mcsse.c (I prefer 8.3 files)
; * - Changes the nasm code to masm
; * - Changes to parameters of the inlined function to accept the offs parameter
; * - Some cleanup of unused function
; * - Removed emms (we don't need that)
; *
; * Todo :
; *
; * - Change this file to pure asm
; * - Or simply try the __fastcall mode
; *
; *****************************************************************************/

bits 32


section .data

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

%macro INIT 0
    push esi
    push edi
    mov         edi, [esp + 8 + 4]  ; dest
    mov         esi, [esp + 8 + 8]  ; ref
    mov         edx, [esp + 8 + 12] ; stride
    mov         eax, [esp + 8 + 20] ; height
%endmacro

%macro INIT2 0
    push ecx
    push esi
    push edi
    mov         edi, [esp + 12 + 4]  ; dest
    mov         esi, [esp + 12 + 8]  ; ref
    mov         edx, [esp + 12 + 12] ; stride
    mov         ecx, esi
    add         ecx, [esp + 12 + 16] ; offs
    mov         eax, [esp + 12 + 20] ; height
%endmacro

%macro EXIT 0
    pop edi
    pop esi
    ret
%endmacro

%macro EXIT2 0
    pop edi
    pop esi
    pop ecx
    ret
%endmacro

%macro MCxyAC 2 ; src off
    movq mm0, [%1]
    movq mm1, [%2+1]
    movq mm7, mm0
    movq mm2, [%1+1]
    pxor mm7, mm1
    movq mm3, [%2]
    movq mm6, mm2
    pxor mm6, mm3
    pavgb mm0, mm1
    pavgb mm2, mm3
    por mm7, mm6
    movq mm6, mm0
    pxor mm6, mm2
    pand mm7, mm6
    pand mm7, [mask_one]
    pavgb mm0, mm2
    psubusb mm0, mm7
%endmacro

%macro MCxyFAST 2 ; src off
    movq mm0, [%1]
    movq mm1, [%2+1]
    movq mm2, [%1+1]
    movq mm3, [%2]
    pavgb mm0, mm1
    pavgb mm2, mm3
    pavgb mm0, mm2
    psubusb mm0, [mask_one]
%endmacro

mask_one db 1,1,1,1,1,1,1,1

align 16

section .text

align 16
cglobal MC_put_8_mmxext
MC_put_8_mmxext

    INIT
.mc0
    movq mm0, [esi]
    movq [edi], mm0
    add esi, edx
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_put_16_mmxext
MC_put_16_mmxext

    INIT
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    add esi, edx
    movq [edi], mm0
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_avg_8_mmxext
MC_avg_8_mmxext

    INIT
.mc0
    movq mm0, [esi]
    pavgb mm0, [edi]
    add esi, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_avg_16_mmxext
MC_avg_16_mmxext

    INIT
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    pavgb mm0, [edi]
    pavgb mm1, [edi+8]
    movq [edi], mm0
    add esi, edx
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_put_x8_mmxext
MC_put_x8_mmxext

    INIT
.mc0
    movq mm0, [esi]
    pavgb mm0, [esi+1]
    add esi, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT



align 16
cglobal MC_put_y8_mmxext
MC_put_y8_mmxext

    INIT2
.mc0
    movq mm0, [esi]
    pavgb mm0, [ecx]
    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_put_x16_mmxext
MC_put_x16_mmxext

    INIT
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    pavgb mm0, [esi+1]
    pavgb mm1, [esi+9]
    movq [edi], mm0
    add esi, edx
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_put_y16_mmxext
MC_put_y16_mmxext

    INIT2
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    pavgb mm0, [ecx]
    pavgb mm1, [ecx+8]
    movq [edi], mm0
    add esi, edx
    add ecx, edx
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_x8_mmxext
MC_avg_x8_mmxext

    INIT
.mc0
    movq mm0, [esi]
    pavgb mm0, [esi+1]
    pavgb mm0, [edi]
    add esi, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_avg_y8_mmxext
MC_avg_y8_mmxext

    INIT2
.mc0
    movq mm0, [esi]
    pavgb mm0, [ecx]
    pavgb mm0, [edi]
    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_x16_mmxext
MC_avg_x16_mmxext

    INIT
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    pavgb mm0, [esi+1]
    pavgb mm1, [esi+9]
    pavgb mm0, [edi]
    pavgb mm1, [edi+8]
    add esi, edx
    movq [edi], mm0
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT

align 16
cglobal MC_avg_y16_mmxext
MC_avg_y16_mmxext

    INIT2
.mc0
    movq mm0, [esi]
    movq mm1, [esi+8]
    pavgb mm0, [ecx]
    pavgb mm1, [ecx+8]
    pavgb mm0, [edi]
    pavgb mm1, [edi+8]
    add esi, edx
    add ecx, edx
    movq [edi], mm0
    movq [edi+8], mm1
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_put_xy8_mmxext_AC
MC_put_xy8_mmxext_AC

    INIT2
.mc0

    MCxyAC esi, ecx

    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_put_xy8_mmxext_FAST
MC_put_xy8_mmxext_FAST

    INIT2
.mc0

    MCxyFAST esi, ecx

    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2



align 16
cglobal MC_put_xy16_mmxext_AC
MC_put_xy16_mmxext_AC

    INIT2
.mc0

    MCxyAC esi, ecx

    movq [edi], mm0

    MCxyAC esi+8, ecx+8

    add esi, edx
    add ecx, edx
    movq [edi+8], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_put_xy16_mmxext_FAST
MC_put_xy16_mmxext_FAST

    INIT2
.mc0

    MCxyFAST esi, ecx

    movq [edi], mm0

    MCxyFAST esi+8, ecx+8

    add esi, edx
    add ecx, edx
    movq [edi+8], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_xy8_mmxext_AC
MC_avg_xy8_mmxext_AC

    INIT2
.mc0

    MCxyAC esi, ecx

    movq mm1, [edi]
    pavgb mm0, mm1

    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_xy8_mmxext_FAST
MC_avg_xy8_mmxext_FAST

    INIT2
.mc0

    MCxyFAST esi, ecx

    movq mm1, [edi]
    pavgb mm0, mm1

    add esi, edx
    add ecx, edx
    movq [edi], mm0
    add edi, edx
    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_xy16_mmxext_AC
MC_avg_xy16_mmxext_AC

    INIT2
.mc0

    MCxyAC esi, ecx

    movq mm1, [edi]
    pavgb mm0, mm1
    movq [edi], mm0

    MCxyAC esi+8, ecx+8

    movq mm1, [edi+8]
    pavgb mm0, mm1
    add esi, edx
    add ecx, edx
    movq [edi+8], mm0
    add edi, edx

    dec eax
    jnz .mc0
    EXIT2

align 16
cglobal MC_avg_xy16_mmxext_FAST
MC_avg_xy16_mmxext_FAST

    INIT2
.mc0

    MCxyFAST esi, ecx

    movq mm1, [edi]
    pavgb mm0, mm1
    movq [edi], mm0

    MCxyFAST esi+8, ecx+8

    movq mm1, [edi+8]
    pavgb mm0, mm1
    add esi, edx
    add ecx, edx
    movq [edi+8], mm0
    add edi, edx

    dec eax
    jnz .mc0
    EXIT2
