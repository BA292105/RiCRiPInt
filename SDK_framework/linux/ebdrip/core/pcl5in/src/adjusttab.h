/** \file
 * \ingroup pcl5
 *
 * $HopeName: COREpcl_pcl5!src:adjusttab.h(EBDSDK_P.1) $
 *
 * Copyright (C) 2008-2009 Global Graphics Software Ltd. All rights reserved.
 * Global Graphics Software Ltd. Confidential Information.
 */

#ifndef __ADJUSTTAB_H__
#define __ADJUSTTAB_H__ (1)

/*
 * Sm_stick_adjust gives the adjustment in current position
 * that will cause an LO 5 stick character to have centered
 * blackwidth, not just centered character cell.
 *
 * For example, the exclamation point is defined basically as a vertical
 * pole at the left edge of the character cell. It's center as defined is
 * at (1,16). This 15 x units to the left of the default (16,16) center,
 * so its Sm_stick_adjust is (15,0).
 *
 *      Sm_stick_adjust value = (16,16) minus center_as_defined;
 *
 */

const int8 Sm_stick_adjust[2][256][2] = {
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,
  0,  0,        /* 31 - last control char */
  16, 16,          /*  32 */
  15,  0,          /*  33 */
  0,-15,          /*  34 */
  0,  0,          /*  35 */
  0,  0,          /*  36 */
  0,  0,          /*  37 */
  0,  0,          /*  38 */
  -1,-15,          /*  39 */
  -12,  0,          /*  40 */
  12,  0,          /*  41 */
  0,  0,          /*  42 */
  0,  0,          /*  43 */
  14, 17,          /*  44 */
  0,  0,          /*  45 */
  -1, 15,          /*  46 */
  0,  0,          /*  47 */
  1,  0,          /*  48 */
  -2,  0,          /*  49 */
  0,  0,          /*  50 */
  0,  0,          /*  51 */
  0,  0,          /*  52 */
  0,  0,          /*  53 */
  0,  0,          /*  54 */
  0,  0,          /*  55 */
  0,  0,          /*  56 */
  0,  0,          /*  57 */
  14,  4,          /*  58 */
  14,  6,          /*  59 */
  0,  0,          /*  60 */
  0,  0,          /*  61 */
  0,  0,          /*  62 */
  5,  0,          /*  63 */
  0,  0,          /*  64 */
  0,  0,          /*  65 */
  0,  0,          /*  66 */
  0,  0,          /*  67 */
  0,  0,          /*  68 */
  0,  0,          /*  69 */
  0,  0,          /*  70 */
  0,  0,          /*  71 */
  0,  0,          /*  72 */
  0,  0,          /*  73 */
  0,  0,          /*  74 */
  0,  0,          /*  75 */
  0,  0,          /*  76 */
  0,  0,          /*  77 */
  0,  0,          /*  78 */
  0,  0,          /*  79 */
  0,  0,          /*  80 */
  0,  0,          /*  81 */
  0,  0,          /*  82 */
  0,  0,          /*  83 */
  0,  0,          /*  84 */
  0,  0,          /*  85 */
  0,  0,          /*  86 */
  0,  0,          /*  87 */
  0,  0,          /*  88 */
  0,  0,          /*  89 */
  0,  0,          /*  90 */
  -11,  0,          /*  91 */
  0,  0,          /*  92 */
  11,  0,          /*  93 */
  2,-17,          /*  94 */
  -8, 21,          /*  95 */
  2,-18,          /*  96 */
  2,  4,          /*  97 */
  2,  0,          /*  98 */
  2,  4,          /*  99 */
  2,  0,          /* 100 */
  2,  4,          /* 101 */
  1,  0,          /* 102 */
  2,  8,          /* 103 */
  2,  0,          /* 104 */
  0,  0,          /* 105 */
  4,  4,          /* 106 */
  2,  0,          /* 107 */
  0,  0,          /* 108 */
  0,  4,          /* 109 */
  2,  4,          /* 110 */
  2,  4,          /* 111 */
  2,  8,          /* 112 */
  2,  8,          /* 113 */
  2,  4,          /* 114 */
  2,  4,          /* 115 */
  2,  0,          /* 116 */
  2,  4,          /* 117 */
  2,  4,          /* 118 */
  0,  4,          /* 119 */
  2,  4,          /* 120 */
  2,  8,          /* 121 */
  2,  4,          /* 122 */
  -10,  0,          /* 123 */
  16,  0,          /* 124 */
  10,  0,          /* 125 */
  2,-17,          /* 126 */
  16, 16,          /* 127 */
  0,-18,          /* 128 */
  2,-17,          /* 129 */
  2,-18,          /* 130 */
  2,-17,          /* 131 */
  0,-23,          /* 132 */
  0,-26,          /* 133 */
  2,-15,          /* 134 */
  2,-18,          /* 135 */
  0,-25,          /* 136 */
  -2,-26,          /* 137 */
  4,-26,          /* 138 */
  0,-25,          /* 139 */
  0,-25,          /* 140 */
  2,-17,          /* 141 */
  0,  5,          /* 142 */
  16, 16,          /* 143 */
  16, 16,          /* 144 */
  16, 16,          /* 145 */
  16, 16,          /* 146 */
  16, 16,          /* 147 */
  16, 16,          /* 148 */
  16, 16,          /* 149 */
  16, 16,          /* 150 */
  16, 16,          /* 151 */
  16, 16,          /* 152 */
  16, 16,          /* 153 */
  16, 16,          /* 154 */
  16, 16,          /* 155 */
  16, 16,          /* 156 */
  16, 16,          /* 157 */
  16, 16,          /* 158 */
  16, 16,          /* 159 */
  16, 16,          /* 160 */
  0,  0,          /* 161 */
  0,  0,          /* 162 */
  0,  0,          /* 163 */
  0,  0,          /* 164 */
  0,  0,          /* 165 */
  0,  0,          /* 166 */
  0,  0,          /* 167 */
  -2,-26,          /* 168 */
  4,-26,          /* 169 */
  0,-25,          /* 170 */
  0,-23,          /* 171 */
  0,-25,          /* 172 */
  0,  0,          /* 173 */
  0,  0,          /* 174 */
  0,  0,          /* 175 */
  0,-22,          /* 176 */
  0,  0,          /* 177 */
  2,  8,          /* 178 */
  0,-26,          /* 179 */
  0,  0,        /* use unaccented C (67) value */
  2,  4,        /* use unaccented c (99) value */
  0,  0,          /* 182 */
  2,  4,          /* 183 */
  -15,  8,          /* 184 */
  5,  8,          /* 185 */
  0,  0,          /* 186 */
  0,  0,          /* 187 */
  2, -2,          /* 188 */
  0,  0,          /* 189 */
  0,  2,          /* 190 */
  2,  5,          /* 191 */
  2,  4,          /* 192 */
  2,  4,          /* 193 */
  2,  4,          /* 194 */
  2,  4,          /* 195 */
  2,  4,          /* 196 */
  2,  4,          /* 197 */
  2,  4,          /* 198 */
  2,  4,          /* 199 */
  2,  4,          /* 200 */
  2,  4,          /* 201 */
  2,  4,          /* 202 */
  2,  4,          /* 203 */
  2,  4,          /* 204 */
  2,  4,          /* 205 */
  2,  4,          /* 206 */
  2,  4,          /* 207 */
  0,  0,          /* 208 */
  0,  0,          /* 209 */
  0,  0,          /* 210 */
  0,  0,          /* 211 */
  2,  4,          /* 212 */
  0,  0,          /* 213 */
  2,  4,          /* 214 */
  0,  4,          /* 215 */
  0,  0,          /* 216 */
  0,  0,          /* 217 */
  0,  0,          /* 218 */
  0,  0,          /* 219 */
  0,  0,          /* 220 */
  0,  0,          /* 221 */
  0,  0,          /* 222 */
  0,  0,          /* 223 */
  0,  0,          /* 224 */
  0,  0,          /* 225 */
  2,  4,          /* 226 */
  0,  0,          /* 227 */
  0,  0,          /* 228 */
  0,  0,          /* 229 */
  0,  0,          /* 230 */
  0,  0,          /* 231 */
  0,  0,          /* 232 */
  0,  0,          /* 233 */
  2,  4,          /* 234 */
  0,  0,          /* 235 */
  2,  4,          /* 236 */
  0,  0,          /* 237 */
  0,  0,          /* 238 */
  2,  8,          /* 239 */
  0,  4,          /* 240 */
  2,  4,          /* 241 */
  -1, -1,          /* 242 */
  0,  8,          /* 243 */
  0,  0,          /* 244 */
  1,  2,          /* 245 */
  0,  0,          /* 246 */
  0,  0,          /* 247 */
  0,  0,          /* 248 */
  0, -3,          /* 249 */
  0, -3,          /* 250 */
  0,  4,          /* 251 */
  0,  4,          /* 252 */
  0,  4,          /* 253 */
  0, -1,          /* 254 */
  16, 16,          /* 255 */
  /* end of FIXED_SPACE characters */


  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,-18,0,0,0,0,0,0,0,0,0,-19,
  0,0,0,0,0,0,0,0,0,21,0,0,0,17,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,5,0,9,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,-18,0,24,0,-19,0,4,0,0,0,5,
  0,0,0,5,0,0,0,10,0,0,0,0,0,5,0,0,0,0,0,5,
  0,5,0,5,0,10,0,10,0,5,0,5,0,1,0,5,0,5,0,5,
  0,5,0,10,0,5,0,0,0,0,0,0,0,0,0,0,0,-19,0,-18,
  0,-25,0,-19,0,-26,0,-24,0,-17,0,-16,0,-27,0,-3,0,-18,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-19,0,-19,
  0,-18,0,-25,0,-26,0,1,0,1,0,1,0,-24,0,0,0,5,0,-27,
  0,5,0,10,0,0,0,5,0,10,0,10,0,0,0,1,0,-2,0,0,
  0,5,0,8,0,4,0,5,0,5,0,5,0,4,0,5,0,5,0,5,
  0,4,0,5,0,5,0,5,0,4,0,5,0,5,0,5,0,0,0,-3,
  0,0,0,0,0,4,0,-3,0,5,0,5,0,0,0,-3,0,0,0,1,
  0,0,0,-3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,5,0,0,0,5,0,1,0,0,0,10,
  0,8,0,5,0,0,0,10,0,0,0,0,0,0,0,0,0,0,0,-3,
  0,-3,0,5,0,5,0,5,0,0,0,0,

  /* end of ARC characters */
}; /* end of Sm_stick_adjust */

#endif /* !__ADJUSTTAB_H__ */

/* Log stripped */
