

#include "VideoCommon/PixelShaderGen.h"

#include "VideoCommon/BPMemory.h"
#include "VideoCommon/BoundingBox.h"
#include "VideoCommon/LightingShaderGen.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/RenderState.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/XFMemory.h"  // for texture projection mode


// FIXME: Some of the video card's capabilities (BBox support, EarlyZ support, dstAlpha support)
//        leak into this UID; This is really unhelpful if these UIDs ever move from one machine to
//        another.
PixelShaderUid GetPixelShaderUid()
{
  PixelShaderUid out;

  pixel_shader_uid_data* const uid_data = out.GetUidData();
  uid_data->useDstAlpha = bpmem.dstalpha.enable && bpmem.blendmode.alphaupdate &&
                          bpmem.zcontrol.pixel_format == PEControl::RGBA6_Z24;

  uid_data->genMode_numindstages = bpmem.genMode.numindstages;
  uid_data->genMode_numtevstages = bpmem.genMode.numtevstages;
  uid_data->genMode_numtexgens = bpmem.genMode.numtexgens;
  uid_data->bounding_box = g_ActiveConfig.bBBoxEnable && BoundingBox::active;
  uid_data->rgba6_format =
      bpmem.zcontrol.pixel_format == PEControl::RGBA6_Z24 && !g_ActiveConfig.bForceTrueColor;
  uid_data->dither = bpmem.blendmode.dither && uid_data->rgba6_format;
  uid_data->uint_output = bpmem.blendmode.UseLogicOp();

  u32 numStages = uid_data->genMode_numtevstages + 1;

  const bool forced_early_z =
      bpmem.UseEarlyDepthTest() &&
      (g_ActiveConfig.bFastDepthCalc || bpmem.alpha_test.TestResult() == AlphaTest::UNDETERMINED)
      // We can't allow early_ztest for zfreeze because depth is overridden per-pixel.
      // This means it's impossible for zcomploc to be emulated on a zfrozen polygon.
      && !(bpmem.zmode.testenable && bpmem.genMode.zfreeze);
  const bool per_pixel_depth =
      (bpmem.ztex2.op != ZTEXTURE_DISABLE && bpmem.UseLateDepthTest()) ||
      (!g_ActiveConfig.bFastDepthCalc && bpmem.zmode.testenable && !forced_early_z) ||
      (bpmem.zmode.testenable && bpmem.genMode.zfreeze);

  uid_data->per_pixel_depth = per_pixel_depth;
  uid_data->forced_early_z = forced_early_z;

  if (g_ActiveConfig.bEnablePixelLighting)
  {
    // The lighting shader only needs the two color bits of the 23bit component bit array.
    uid_data->components =
        (VertexLoaderManager::g_current_components & (VB_HAS_COL0 | VB_HAS_COL1)) >> VB_COL_SHIFT;
    uid_data->numColorChans = xfmem.numChan.numColorChans;
    GetLightingShaderUid(uid_data->lighting);
  }

  if (uid_data->genMode_numtexgens > 0)
  {
    for (unsigned int i = 0; i < uid_data->genMode_numtexgens; ++i)
    {
      // optional perspective divides
      uid_data->texMtxInfo_n_projection |= xfmem.texMtxInfo[i].projection << i;
    }
  }

  // indirect texture map lookup
  int nIndirectStagesUsed = 0;
  if (uid_data->genMode_numindstages > 0)
  {
    for (unsigned int i = 0; i < numStages; ++i)
    {
      if (bpmem.tevind[i].IsActive() && bpmem.tevind[i].bt < uid_data->genMode_numindstages)
        nIndirectStagesUsed |= 1 << bpmem.tevind[i].bt;
    }
  }

  uid_data->nIndirectStagesUsed = nIndirectStagesUsed;
  for (u32 i = 0; i < uid_data->genMode_numindstages; ++i)
  {
    if (uid_data->nIndirectStagesUsed & (1 << i))
      uid_data->SetTevindrefValues(i, bpmem.tevindref.getTexCoord(i), bpmem.tevindref.getTexMap(i));
  }

  for (unsigned int n = 0; n < numStages; n++)
  {
    int texcoord = bpmem.tevorders[n / 2].getTexCoord(n & 1);
    bool bHasTexCoord = (u32)texcoord < bpmem.genMode.numtexgens;
    // HACK to handle cases where the tex gen is not enabled
    if (!bHasTexCoord)
      texcoord = bpmem.genMode.numtexgens;

    uid_data->stagehash[n].hasindstage = bpmem.tevind[n].bt < bpmem.genMode.numindstages;
    uid_data->stagehash[n].tevorders_texcoord = texcoord;
    if (uid_data->stagehash[n].hasindstage)
      uid_data->stagehash[n].tevind = bpmem.tevind[n].hex;

    TevStageCombiner::ColorCombiner& cc = bpmem.combiners[n].colorC;
    TevStageCombiner::AlphaCombiner& ac = bpmem.combiners[n].alphaC;
    uid_data->stagehash[n].cc = cc.hex & 0xFFFFFF;
    uid_data->stagehash[n].ac = ac.hex & 0xFFFFF0;  // Storing rswap and tswap later

    if (cc.a == TEVCOLORARG_RASA || cc.a == TEVCOLORARG_RASC || cc.b == TEVCOLORARG_RASA ||
        cc.b == TEVCOLORARG_RASC || cc.c == TEVCOLORARG_RASA || cc.c == TEVCOLORARG_RASC ||
        cc.d == TEVCOLORARG_RASA || cc.d == TEVCOLORARG_RASC || ac.a == TEVALPHAARG_RASA ||
        ac.b == TEVALPHAARG_RASA || ac.c == TEVALPHAARG_RASA || ac.d == TEVALPHAARG_RASA)
    {
      const int i = bpmem.combiners[n].alphaC.rswap;
      uid_data->stagehash[n].tevksel_swap1a = bpmem.tevksel[i * 2].swap1;
      uid_data->stagehash[n].tevksel_swap2a = bpmem.tevksel[i * 2].swap2;
      uid_data->stagehash[n].tevksel_swap1b = bpmem.tevksel[i * 2 + 1].swap1;
      uid_data->stagehash[n].tevksel_swap2b = bpmem.tevksel[i * 2 + 1].swap2;
      uid_data->stagehash[n].tevorders_colorchan = bpmem.tevorders[n / 2].getColorChan(n & 1);
    }

    uid_data->stagehash[n].tevorders_enable = bpmem.tevorders[n / 2].getEnable(n & 1);
    if (uid_data->stagehash[n].tevorders_enable)
    {
      const int i = bpmem.combiners[n].alphaC.tswap;
      uid_data->stagehash[n].tevksel_swap1c = bpmem.tevksel[i * 2].swap1;
      uid_data->stagehash[n].tevksel_swap2c = bpmem.tevksel[i * 2].swap2;
      uid_data->stagehash[n].tevksel_swap1d = bpmem.tevksel[i * 2 + 1].swap1;
      uid_data->stagehash[n].tevksel_swap2d = bpmem.tevksel[i * 2 + 1].swap2;
      uid_data->stagehash[n].tevorders_texmap = bpmem.tevorders[n / 2].getTexMap(n & 1);
    }

    if (cc.a == TEVCOLORARG_KONST || cc.b == TEVCOLORARG_KONST || cc.c == TEVCOLORARG_KONST ||
        cc.d == TEVCOLORARG_KONST || ac.a == TEVALPHAARG_KONST || ac.b == TEVALPHAARG_KONST ||
        ac.c == TEVALPHAARG_KONST || ac.d == TEVALPHAARG_KONST)
    {
      uid_data->stagehash[n].tevksel_kc = bpmem.tevksel[n / 2].getKC(n & 1);
      uid_data->stagehash[n].tevksel_ka = bpmem.tevksel[n / 2].getKA(n & 1);
    }
  }

#define MY_STRUCT_OFFSET(str, elem) ((u32)((u64) & (str).elem - (u64) & (str)))
  uid_data->num_values = (g_ActiveConfig.bEnablePixelLighting) ?
                             sizeof(*uid_data) :
                             MY_STRUCT_OFFSET(*uid_data, stagehash[numStages]);

  AlphaTest::TEST_RESULT Pretest = bpmem.alpha_test.TestResult();
  uid_data->Pretest = Pretest;
  uid_data->late_ztest = bpmem.UseLateDepthTest();

  // NOTE: Fragment may not be discarded if alpha test always fails and early depth test is enabled
  // (in this case we need to write a depth value if depth test passes regardless of the alpha
  // testing result)
  if (uid_data->Pretest == AlphaTest::UNDETERMINED ||
      (uid_data->Pretest == AlphaTest::FAIL && uid_data->late_ztest))
  {
    uid_data->alpha_test_comp0 = bpmem.alpha_test.comp0;
    uid_data->alpha_test_comp1 = bpmem.alpha_test.comp1;
    uid_data->alpha_test_logic = bpmem.alpha_test.logic;

    // ZCOMPLOC HACK:
    // The only way to emulate alpha test + early-z is to force early-z in the shader.
    // As this isn't available on all drivers and as we can't emulate this feature otherwise,
    // we are only able to choose which one we want to respect more.
    // Tests seem to have proven that writing depth even when the alpha test fails is more
    // important that a reliable alpha test, so we just force the alpha test to always succeed.
    // At least this seems to be less buggy.
    uid_data->alpha_test_use_zcomploc_hack =
        bpmem.UseEarlyDepthTest() && bpmem.zmode.updateenable &&
        !g_ActiveConfig.backend_info.bSupportsEarlyZ && !bpmem.genMode.zfreeze;
  }

  uid_data->zfreeze = bpmem.genMode.zfreeze;
  uid_data->ztex_op = bpmem.ztex2.op;
  uid_data->early_ztest = bpmem.UseEarlyDepthTest();
  uid_data->fog_fsel = bpmem.fog.c_proj_fsel.fsel;
  uid_data->fog_fsel = bpmem.fog.c_proj_fsel.fsel;
  uid_data->fog_proj = bpmem.fog.c_proj_fsel.proj;
  uid_data->fog_RangeBaseEnabled = bpmem.fogRange.Base.Enabled;

  BlendingState state = {};
  state.Generate(bpmem);

  if (state.usedualsrc && state.dstalpha && g_ActiveConfig.backend_info.bSupportsFramebufferFetch &&
      !g_ActiveConfig.backend_info.bSupportsDualSourceBlend)
  {
    uid_data->blend_enable = state.blendenable;
    uid_data->blend_src_factor = state.srcfactor;
    uid_data->blend_src_factor_alpha = state.srcfactoralpha;
    uid_data->blend_dst_factor = state.dstfactor;
    uid_data->blend_dst_factor_alpha = state.dstfactoralpha;
    uid_data->blend_subtract = state.subtract;
    uid_data->blend_subtract_alpha = state.subtractAlpha;
  }

  return out;
}

bool pixel_shader_uid_data::HasZtexture() const {
      // depth texture can safely be ignored if the result won't be written to the depth buffer
      // (early_ztest) and isn't used for fog either
      return (per_pixel_depth || fog_fsel) && ztex_op != ZTEXTURE_DISABLE;
  }

void ClearUnusedPixelShaderUidBits(APIType ApiType, const ShaderHostConfig& host_config,
                                   PixelShaderUid* uid)
{
  pixel_shader_uid_data* const uid_data = uid->GetUidData();

  // OpenGL and Vulkan convert implicitly normalized color outputs to their uint representation.
  // Therefore, it is not necessary to use a uint output on these backends. We also disable the
  // uint output when logic op is not supported (i.e. driver/device does not support D3D11.1).
  if (ApiType != APIType::D3D || !host_config.backend_logic_op)
    uid_data->uint_output = 0;

  // If bounding box is enabled when a UID cache is created, then later disabled, we shouldn't
  // emit the bounding box portion of the shader.
  uid_data->bounding_box &= host_config.bounding_box & host_config.backend_bbox;
}

PixelShaderUid OptimiseShaderUid(const PixelShaderUid &uid, bool color_used, bool alpha_used) {
    PixelShaderUid out = uid;
    pixel_shader_uid_data* const uid_data = out.GetUidData();

    // The last pixel stage is always output to framebuffer, so the output register is ignored
    int last_stage = uid_data->genMode_numtevstages;
    uid_data->stagehash[last_stage].cc &= 0x3fffff;
    uid_data->stagehash[last_stage].ac &= 0x3fffff;

    if (uid_data->Pretest == AlphaTest::UNDETERMINED)
        alpha_used = true;

    // TODO: filter out channels which aren't used by rasterizer state
    bool used_color[4] = { color_used, false, false, false };
    bool used_alpha[4] = { alpha_used, false, false, false };
    bool used_prev_indirect = false; // indirect can append the result from the previous indirect stage

    // Start at the last stage and work our way backwards, propergating the usage
    // of color and alpha registers from previous stages.
    for (int stage_id = last_stage; stage_id > 0; stage_id--) {
        auto &stage = uid_data->stagehash[stage_id];

        bool used_tex = stage_id == last_stage && uid_data->HasZtexture(); // ztexture uses last stage
        bool used_ras = false;
        bool used_konst_color = false;
        bool used_konst_alpha = false;

        TevStageCombiner::ColorCombiner cc = { stage.cc };
        TevStageCombiner::AlphaCombiner ac = { stage.ac };

        // Cache status of used alpha, as the color combiner will modify it.
        bool alpha_used = used_alpha[ac.dest];
        used_alpha[ac.dest] = false;

        if (!used_color[cc.dest]) {
            stage.cc = 0;
            stage.ignore_cc = 1;
        } else {
            used_color[cc.dest] = false;
            auto markUsed = [&](u32 input) {
                switch (input) {
                    case 0:
                    case 2:
                    case 4:
                    case 6:
                        used_color[input >> 1] = true;
                        break;
                    case 1:
                    case 3:
                    case 5:
                    case 7:
                        used_alpha[input >> 1] = true;
                        break;
                    case 8:
                    case 9:
                        used_tex = true;
                        break;
                    case 10:
                    case 11:
                        used_ras = true;
                        break;
                    case 14:
                        used_konst_color = true;
                        break;
                }
            };

            markUsed(cc.a);
            markUsed(cc.b);
            markUsed(cc.c);
            markUsed(cc.d);
        }

        if(!alpha_used) {
            stage.ac = 0;
            stage.ignore_ac = 1;
        } else {
            auto markUsed = [&](u32 input) {
                switch (input) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        used_alpha[input] = true;
                        break;
                    case 4:
                        used_tex = true;
                        break;
                    case 5:
                        used_ras = true;
                        break;
                    case 6:
                        used_konst_alpha = true;
                        break;
                }
            };

            u32 alpha_compare_op = ac.shift << 1 | ac.op;

            // alpha_A and alpha_B are unused by compare ops 0-5
            if (ac.bias != TEVBIAS_COMPARE || alpha_compare_op > 5) {
                markUsed(ac.a);
                markUsed(ac.b);
            }
            markUsed(ac.c);
            markUsed(ac.d);
        }

        bool used_indirect = used_prev_indirect;
        used_prev_indirect = false;

        if (!used_tex) {
            stage.tevorders_texcoord = 0;
            stage.tevorders_texmap = 0;
            stage.tevorders_enable = 0;

            stage.tevksel_swap1c = 0;
            stage.tevksel_swap1d = 0;
            stage.tevksel_swap2c = 0;
            stage.tevksel_swap2d = 0;
        } else {
            used_indirect = stage.tevorders_enable;
        }

        if (!used_indirect) {
            stage.hasindstage = 0;
            stage.tevind = 0;
        } else if (stage.hasindstage) {
            TevStageIndirect tevind = { stage.tevind };
            used_prev_indirect = tevind.fb_addprev;
        }

        if (!used_ras) {
            stage.tevorders_colorchan = 0;

            stage.tevksel_swap1a = 0;
            stage.tevksel_swap1b = 0;
            stage.tevksel_swap2a = 0;
            stage.tevksel_swap2b = 0;
        }

        if (!used_konst_color) {
            stage.tevksel_kc = 0;
        }

        if (!used_konst_alpha) {
            stage.tevksel_ka = 0;
        }
    }


    if (!uid_data->HasZtexture() && uid_data->ztex_op != ZTEXTURE_DISABLE)
        uid_data->ztex_op = ZTEXTURE_DISABLE;

    return out;
}