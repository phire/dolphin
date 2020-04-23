// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "Common/CommonTypes.h"

// all constant buffer attributes must be 16 bytes aligned, so this are the only allowed components:
using float4 = std::array<float, 4>;
using uint4 = std::array<u32, 4>;
using int4 = std::array<s32, 4>;

struct PixelShaderConstants
{
  std::array<int4, 4> colors;
  std::array<int4, 4> kcolors;
  int4 alpha;
  std::array<float4, 8> texdims;
  std::array<int4, 2> zbias;
  std::array<int4, 2> indtexscale;
  std::array<int4, 6> indtexmtx;
  int4 fogcolor;
  int4 fogi;
  float4 fogf;
  std::array<float4, 3> fogrange;
  float4 zslope;
  std::array<float, 2> efbscale;  // .xy

  // Constants from here onwards are only used in ubershaders.
  u32 genmode;                  // .z
  u32 alphaTest;                // .w
  u32 fogParam3;                // .x
  u32 fogRangeBase;             // .y
  u32 dstalpha;                 // .z
  u32 ztex_op;                  // .w
  u32 late_ztest;               // .x (bool)
  u32 rgba6_format;             // .y (bool)
  u32 dither;                   // .z (bool)
  u32 bounding_box;             // .w (bool)
  std::array<uint4, 16> pack1;  // .xy - combiners, .z - tevind, .w - iref
  std::array<uint4, 8> pack2;   // .x - tevorder, .y - tevksel
  std::array<int4, 32> konst;   // .rgba
  // The following are used in ubershaders when using shader_framebuffer_fetch blending
  u32 blend_enable;
  u32 blend_src_factor;
  u32 blend_src_factor_alpha;
  u32 blend_dst_factor;
  u32 blend_dst_factor_alpha;
  u32 blend_subtract;
  u32 blend_subtract_alpha;
};

constexpr size_t PixelShaderConstants_size_excluding_ubershaders = offsetof(PixelShaderConstants, genmode);

struct VertexShaderActiveUniforms {
    u32 posnormalmatrix : 1;
    u32 materials : 1;
    u32 directional_lights : 1;
    u32 num_lights : 4;
    u32 texmatrices : 1;
    u32 transformmatrices : 1;
    u32 normalmatrices : 1;
    u32 posttransformmatrices : 1;
    u32 uber : 1;

    constexpr static VertexShaderActiveUniforms Everything() {
      VertexShaderActiveUniforms ret = {};
      ret.posnormalmatrix = 1;
      ret.materials = 1;
      ret.directional_lights = 1;
      ret.num_lights = 8;
      ret.texmatrices = 1;
      ret.transformmatrices = 1;
      ret.normalmatrices = 1;
      ret.posttransformmatrices = 1;
      ret.uber = 1;

      return ret;
    }
};

struct VertexShaderConstants
{
  float4 pixelcentercorrection;
  std::array<float, 2> viewport;  // .xy
  std::array<float, 2> pad1;      // .zw

  std::array<float4, 4> projection; // always

  std::array<float4, 6> posnormalmatrix; // not !VB_HAS_POSMTXIDX
  std::array<int4, 4> materials; // light matsource / ambsource
  struct Light
  {
    int4 color; // always
    float4 pos; // always
    float4 dir; // LIGHTATTN_SPEC / LIGHTATTN_SPOT
    float4 cosatt; // LIGHTATTN_SPEC / LIGHTATTN_SPOT
    float4 distatt; // LIGHTATTN_SPEC / LIGHTATTN_SPOT
  };
  std::array<Light, 8> lights;
  std::array<float4, 24> texmatrices; // (XF_TEXGEN_REGULAR && !VB_HAS_TEXMTXIDX0 < i)
  std::array<float4, 64> transformmatrices; // VB_HAS_POSMTXIDX || (XF_TEXGEN_REGULAR && VB_HAS_TEXMTXIDX0 < i)
  std::array<float4, 32> normalmatrices; // VB_HAS_POSMTXIDX && VB_HAS_NRMALL
  std::array<float4, 64> posttransformmatrices; // uid_data->dualTexTrans_enabled && texinfo.texgentype == XF_TEXGEN_REGULAR

  // Constants from here onwards are only used in ubershaders.
  u32 components;           // .x
  u32 xfmem_dualTexInfo;    // .y
  u32 xfmem_numColorChans;  // .z
  u32 pad2;                 // .w
  // .x - texMtxInfo, .y - postMtxInfo, [0..1].z = color, [0..1].w = alpha
  std::array<uint4, 8> xfmem_pack1;

  // Write just the fields that are actually used by a given vertex shader to *dest
  size_t WriteActive(u8 *dest, VertexShaderActiveUniforms fmt);
  static size_t GetActiveSize(VertexShaderActiveUniforms fmt);
};

struct GeometryShaderConstants
{
  float4 stereoparams;
  float4 lineptparams;
  int4 texoffset;
};


class vertex_shader_uid_data;
VertexShaderActiveUniforms GetActiveUniforms(const vertex_shader_uid_data* uid_data);