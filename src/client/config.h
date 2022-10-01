/*
 * Copyright (c) 2010-2022 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "framework/util/compileXor.h"

 // GENERAL
#define SPRITE_SIZE 32

// APPEARANCES
#define BYTES_IN_SPRITE_SHEET 384 * 384 * 4
#define LZMA_UNCOMPRESSED_SIZE BYTES_IN_SPRITE_SHEET + 122
#define LZMA_HEADER_SIZE LZMA_PROPS_SIZE + 8
#define SPRITE_SHEET_WIDTH_BYTES 384 * 4

// TILE
#define MAX_ELEVATION 24
#define MAX_THINGS 10
#define TRANSPARENT_FLOOR_VIEW_RANGE 2

// CREATURE
#define SHIELD_BLINK_TICKS 500
#define VOLATILE_SQUARE_DURATION 1000
#define ADJUST_CREATURE_INFORMATION_BASED_ON_CROP_SIZE 0 // 1 to enable

// HIGHLIGHT
#define HIGHTLIGHT_FADE_START 60
#define HIGHTLIGHT_FADE_END 120

// WALK SYSTEM
// 1 - Nostalrius 7.2
// 0 - Nekiro TFS1.4-Downgrades and 7.72-downgrade, YurOTS 7.76, OTServ 0.6.4 (8.6), Evolutions (7.6), OTHire 7.2
#define FORCE_NEW_WALKING_FORMULA 0

// FLOOR
#define SEA_FLOOR 7
#define MAX_Z 15
#define UNDERGROUND_FLOOR SEA_FLOOR + 1
#define AWARE_UNDEGROUND_FLOOR_RANGE 2

// TICKS
#define INVISIBLE_TICKS_PER_FRAME 500
#define ITEM_TICKS_PER_FRAME 500
#define EFFECT_TICKS_PER_FRAME 75
#define MISSILE_TICKS_PER_FRAME 75

// TEXT
#define ANIMATED_TEXT_DURATION 1000
#define STATIC_DURATION_PER_CHARACTER 60
#define MIN_STATIC_TEXT_DURATION 3000


// DEBUG
#define ENABLE_CONSOLE 0

#define BOT_PROTECTION 0

// *** ENCRYPTION SYSTEM *** \\

// Should Files be packed in 1 file as zip with password
#define ENCRYPTION_PACKED 0
#define ARCHIVE_PASSWORD XorStr("notWorking...")
/*
moWcecYGCE
9KrXbtSI0i
ASh7WBqpZ6
5ToyrflT3r
yR2cVGfNSR
RdI6Aqm4Cp
JrJzC0kMqs
gxtB85i3SE
g6VEa667HT
zIwPgv2ENS
q5u@2v+dpv)gnSKsyTH5Rv!ATgh%^jFjIqMF)HWHNtbbW$jBtjURs6Q@y8G5pp74
AUyF*vv@74k6Yz%a)$Mt4njwMrrKIYuDVw#s7QZGZx89y^D$6@SmhHNEIhMpHg9&
JW+EA9KAva*AIk23NT%Q@TBRwezT^&wqfdac)CfXUB*S&X^qMz(VIY!GZtBbMa$m
uEF42)JEYa255SY#cSA7pmN(I6J*U(KakRB&F*@kH@PQ6%rKzx(2%M)K@TaTpWxa
TKw9bC^LdHeFU5x@^jrwu@Af4vS9!yxtsT^GP(DE*VvGn3mVTUJbB3De77s3q9$a
B4F9Z#Vzd*MNDnFFbQ&P!pJdNWx^(Vq25GXk*h&62RM@at59d(RL%8ptmdyRek@E
8Q4v5JZPIzKG%EA#Ct5Cd!#!IG#DImhzw5%xygT$UZjxSYR#NnV(+2B2McMTNx4%
*/
// Enable client encryption
#define ENABLE_ENCRYPTION 0
// Enable client encryption maker/builder.
// You can compile it once and use this executable to only encrypt client files once with command --encrypt which will be using password below.
#define ENABLE_ENCRYPTION_BUILDER 0
// for security reasons make sure you are using password with at last 100+ characters
// this was used in resourcemanager.cpp at lines 409 and 435 i replaced that with XOR compilation time encyption which is used by cheat devs
#define ENCRYPTION_PASSWORD XorStr("q5u@2v+dpv)gnSKsyTH5Rv!ATgh%^jFjIqMF)HWHNtbbW$jBtjURs6Q@y8G5pp74AUyF*vv@74k6Yz%a)$Mt4njwMrrKIYuDVw#s7QZGZx89y^D$6@SmhHNEIhMpHg9&JW+EA9KAva*AIk23NT%Q@TBRwezT^&wqfdac)CfXUB*S&X^qMz(VIY!GZtBbMa$muEF42)JEYa255SY#c7pmN(I6J*U(KakRB&F*@kH@PQ6%rKzx(2%M)K@TaTpWxaTKw9bC^LdHeFU5x@^jrwu@Af4vS9!yxtsT^GP(DE*VvGn3mVTUJbB3De77s3q9$aB4F9Z#Vzd*MNDnFFbQ&P!pJdNWx^(Vq25GXk*h&62RM@at59d(RL%8ptmdyRek@E8Q4v5JZPIzKG%EA#Ct5Cd!#!IG#DImhzw5%xygT$UZjxSYR#NnV(+2B2McMTNx4%")

