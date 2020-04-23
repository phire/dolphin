// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/ConstantManager.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/XFMemory.h"

#include <cstring>

template<typename T>
inline void write(u8 *& dest, T data)
{
    std::memcpy(dest, &data, sizeof(T));
    dest += sizeof(T);
}

size_t VertexShaderConstants::WriteActive(u8 *dest, VertexShaderActiveUniforms fmt)
{
    uintptr_t start = reinterpret_cast<uintptr_t>(dest);
    write(dest, pixelcentercorrection);
    write(dest, viewport);
    write(dest, pad1);
    write(dest, projection);

    if (fmt.posnormalmatrix)
        write(dest, posnormalmatrix);

    if (fmt.materials)
        write(dest, materials);

    for (int i = 0; i < fmt.num_lights; i++)
    {
        auto light = lights[i];
        write(dest, light.color);
        write(dest, light.pos);
        if (fmt.directional_lights)
        {
            write(dest, light.dir);
            write(dest, light.cosatt);
            write(dest, light.distatt);
        }
    }

    if (fmt.texmatrices)
        write(dest, texmatrices);
    if (fmt.transformmatrices)
        write(dest, transformmatrices);
    if (fmt.normalmatrices)
        write(dest, normalmatrices);
    if (fmt.posttransformmatrices)
        write(dest, posttransformmatrices);

    if (fmt.uber) {
        write(dest, components);
        write(dest, xfmem_dualTexInfo);
        write(dest, xfmem_numColorChans);
        write(dest, pad2);
        write(dest, xfmem_pack1);
    }

    return reinterpret_cast<uintptr_t>(dest) - start;
}


size_t VertexShaderConstants::GetActiveSize(VertexShaderActiveUniforms fmt)
{
    size_t size = 0;
    size += sizeof(pixelcentercorrection);
    size += sizeof(viewport);
    size += sizeof(pad1);
    size += sizeof(projection);

    if (fmt.posnormalmatrix)
        size += sizeof(posnormalmatrix);

    if (fmt.materials)
        size += sizeof(materials);

    for (int i = 0; i < fmt.num_lights; i++)
    {
        size += sizeof(Light::color);
        size += sizeof(Light::pos);
        if (fmt.directional_lights)
        {
            size += sizeof(Light::dir);
            size += sizeof(Light::cosatt);
            size += sizeof(Light::distatt);
        }
    }

    if (fmt.texmatrices)
        size += sizeof(texmatrices);
    if (fmt.transformmatrices)
        size += sizeof(transformmatrices);
    if (fmt.normalmatrices)
        size += sizeof(normalmatrices);
    if (fmt.posttransformmatrices)
        size += sizeof(posttransformmatrices);

    if (fmt.uber) {
        size += sizeof(components);
        size += sizeof(xfmem_dualTexInfo);
        size += sizeof(xfmem_numColorChans);
        size += sizeof(pad2);
        size += sizeof(xfmem_pack1);
    }

    return size;
}

VertexShaderActiveUniforms GetActiveUniforms(const vertex_shader_uid_data* uid_data)
{
  VertexShaderActiveUniforms has = {0};

  bool has_normals = !!(uid_data->components & VB_HAS_NRMALL);
  if (uid_data->components & VB_HAS_POSMTXIDX)
  {
    // If we have a per vertex posmatrix index then we are using the transform matrices
    has.transformmatrices = 1;

    // and if we also have one or more normal channels enabled, then are using per-vertex normals
    has.normalmatrices = has_normals;
  }
  else
  {
    // otherwise, we use the regular fixed pos/normal matrix for the whole drawcall
    has.posnormalmatrix = 1;
  }

  int max_light = -1;

  for (u32 i = 0; i < uid_data->numTexGens; ++i)
  {
    auto &texgen = uid_data->texMtxInfo[i];
    if (texgen.texgentype == XF_TEXGEN_REGULAR)
    {
      if (uid_data->dualTexTrans_enabled)
        has.posttransformmatrices = 1;
      if (uid_data->components & VB_HAS_TEXMTXIDX0 << i)
        has.transformmatrices = 1;
      else
        has.texmatrices = 1;
    }

    // The emboss map can use directional lights even when they aren't enabled
    if (texgen.texgentype == XF_TEXGEN_EMBOSS_MAP)
    {
      has.directional_lights = 1;
      max_light = std::max(max_light, int(texgen.embosslightshift));
    }
  }

  // If any of the color channels aren't using vertex colors, then we need materials
  if (uid_data->lighting.matsource != 0xF)
     has.materials = 1; // TODO: For even more savings, we could track this per channel

  // If either lighting channel is using LIGHTATTN_SPEC or LIGHTATTN_SPOT, we need directional lights
  if (uid_data->lighting.attnfunc & 5)
    has.directional_lights = 1;

  for (int i = 0; i < 8; i++)
  {
    // Check all 4 color channels in parallel
    if (uid_data->lighting.light_mask & (0x01010101 << i))
    {
      // Light is enabled on at least one color channel
      max_light = std::max(max_light, i);
    }
  }

  // TODO: For even more data savings, we could pack the lights array tighter when there are unused lights
  has.num_lights = max_light + 1;

  return has;
}