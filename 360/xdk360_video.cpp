/*  SSNES - A Super Nintendo Entertainment System (SNES) Emulator frontend for libsnes.
 *  Copyright (C) 2010-2011 - Hans-Kristian Arntzen
 *
 *  Some code herein may be based on code found in BSNES.
 * 
 *  SSNES is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  SSNES is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with SSNES.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <xtl.h>
#include <xboxmath.h>

#include "../driver.h"
#include "../general.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static const char* g_strPixelShaderProgram =
    " sampler2D tex : register(s0);    "
    " struct PS_IN                     "
    " {                                "
    "     float2 coord : TEXCOORD0;    "
    " };                               "
    "                                  "
    " float4 main(PS_IN in) : COLOR    "
    " {                                "
    "     return tex2D(tex, in.coord); "
    " }                                ";

static const char* g_strVertexShaderProgram =
    " struct VS_IN                            "
    "                                         "
    " {                                       "
    "     float2 pos : POSITION;              "
    "     float2 coord : TEXCOORD0;           "
    " };                                      "
    "                                         "
    " struct VS_OUT                           "
    " {                                       "
    "     float4 pos : POSITION;              "
    "     float2 coord : TEXCOORD0;           "
    " };                                      "
    "                                         "
    " VS_OUT main(VS_IN in)                   "
    " {                                       "
    "     VS_OUT out;                         "
    "     out.pos = float4(in.pos, 0.0, 1.0); "
    "     out.coord = in.coord;               "
    "     return out;                         "
    " }                                       ";

typedef struct DrawVerticeFormats
{
   float x, y;
   float u, v;
} DrawVerticeFormats;

static bool g_quitting;

typedef struct xdk360_video
{
   IDirect3D9* xdk360_device;
   IDirect3DDevice9* xdk360_render_device;
   IDirect3DVertexShader9 *pVertexShader;
   IDirect3DPixelShader9* pPixelShader;
   IDirect3DVertexDeclaration9* pVertexDecl;
   IDirect3DVertexBuffer9* vertex_buf;
   IDirect3DTexture9* lpTexture;
   unsigned frame_count;
} xdk360_video_t;

static void xdk360_gfx_free(void *data)
{
   xdk360_video_t *vid = (xdk360_video_t*)data;
   if (!vid)
      return;

   vid->lpTexture->Release();
   vid->vertex_buf->Release();
   vid->pVertexDecl->Release();
   vid->pPixelShader->Release();
   vid->pVertexShader->Release();
   vid->xdk360_render_device->Release();
   vid->xdk360_device->Release();

   free(vid);
}

static void *xdk360_gfx_init(const video_info_t *video, const input_driver_t **input, void **input_data)
{
   xdk360_video_t *gl = (xdk360_video_t*)calloc(1, sizeof(xdk360_video_t));
   if (!gl)
      return NULL;

   gl->xdk360_device = Direct3DCreate9(D3D_SDK_VERSION);
   if (!gl->xdk360_device)
   {
      free(gl);
      return NULL;
   }

   D3DPRESENT_PARAMETERS d3dpp;
   ZeroMemory(&d3dpp, sizeof(d3dpp));

   d3dpp.BackBufferWidth         = 1280;
   d3dpp.BackBufferHeight        = 720;
   d3dpp.BackBufferFormat        = (D3DFORMAT)MAKESRGBFMT(D3DFMT_A8R8G8B8);
   d3dpp.FrontBufferFormat       = (D3DFORMAT)MAKESRGBFMT(D3DFMT_LE_X8R8G8B8);
   d3dpp.MultiSampleType         = D3DMULTISAMPLE_NONE;
   d3dpp.MultiSampleQuality      = 0;
   d3dpp.BackBufferCount         = 2;
   d3dpp.EnableAutoDepthStencil  = TRUE;
   d3dpp.AutoDepthStencilFormat  = D3DFMT_D24S8;
   d3dpp.SwapEffect              = D3DSWAPEFFECT_DISCARD;
   d3dpp.PresentationInterval    = D3DPRESENT_INTERVAL_ONE;

   gl->xdk360_device->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, 
         &gl->xdk360_render_device);

   ID3DXBuffer* pShaderCodeV = NULL;
   ID3DXBuffer* pShaderCodeP = NULL;
   ID3DXBuffer* pErrorMsg = NULL;

   HRESULT hr = D3DXCompileShader(g_strVertexShaderProgram, (UINT)strlen(g_strVertexShaderProgram),
         NULL, NULL, "main", "vs_2_0", 0, &pShaderCodeV, &pErrorMsg, NULL);

   if (SUCCEEDED(hr))
   {
      hr = D3DXCompileShader(g_strPixelShaderProgram, (UINT)strlen(g_strPixelShaderProgram),
            NULL, NULL, "main", "ps_2_0", 0, &pShaderCodeP, &pErrorMsg, NULL);
   }

   if (FAILED(hr))
   {
      OutputDebugString(pErrorMsg ? (char*)pErrorMsg->GetBufferPointer() : "");
      gl->xdk360_render_device->Release();
      gl->xdk360_device->Release();
      free(gl);
      return NULL;
   }

   gl->xdk360_render_device->CreateVertexShader(pShaderCodeV->GetBufferPointer(), &gl->pVertexShader);
   gl->xdk360_render_device->CreatePixelShader(pShaderCodeP->GetBufferPointer(), &gl->pPixelShader);
   pShaderCodeV->Release();
   pShaderCodeP->Release();

   gl->xdk360_render_device->CreateTexture(512, 512, 1, 0, D3DFMT_LIN_X1R5G5B5,
               0, &gl->lpTexture, NULL);

   gl->xdk360_render_device->CreateVertexBuffer(4 * sizeof(DrawVerticeFormats), 0, 
               0, 0, &gl->vertex_buf, NULL);

   static const DrawVerticeFormats init_verts[] = {
      { 0.0f, 0.0f, 0.0f, 0.0f },
      { 1.0f, 0.0f, 1.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f, 1.0f },
      { 1.0f, 1.0f, 1.0f, 1.0f },
   };

   void *verts_ptr;
   gl->vertex_buf->Lock(0, sizeof(DrawVerticeFormats), &verts_ptr, 0);
   memcpy(verts_ptr, init_verts, sizeof(init_verts));
   gl->vertex_buf->Unlock();

   static const D3DVERTEXELEMENT9 VertexElements[] =
   {
      { 0, 0 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
      { 0, 2 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
      D3DDECL_END()
   };

   gl->xdk360_render_device->CreateVertexDeclaration(VertexElements, &gl->pVertexDecl);

   gl->xdk360_render_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
         0xff000000, 1.0f, 0);

   return gl;
}

static bool xdk360_gfx_frame(void *data, const void *frame,
      unsigned width, unsigned height, unsigned pitch, const char *msg)
{
   xdk360_video_t *vid = (xdk360_video_t*)data;
   vid->frame_count++;

   vid->xdk360_render_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
         0xff000000, 1.0f, 0);

   D3DLOCKED_RECT d3dlr;
   if (SUCCEEDED(vid->lpTexture->LockRect(0, &d3dlr, NULL, D3DLOCK_NOSYSLOCK)))
   {
      for (unsigned y = 0; y < height; y++)
      {
         const uint8_t *in = (const uint8_t*)frame + y * pitch;
         uint8_t *out = (uint8_t*)d3dlr.pBits + y * d3dlr.Pitch;
         memcpy(out, in, width * sizeof(uint16_t));
      }
      vid->lpTexture->UnlockRect(0);
   }

   vid->xdk360_render_device->SetTexture(0, vid->lpTexture);
   vid->xdk360_render_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   vid->xdk360_render_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   vid->xdk360_render_device->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DADDRESS_BORDER);
   vid->xdk360_render_device->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DADDRESS_BORDER);

   vid->xdk360_render_device->SetVertexShader(vid->pVertexShader);
   vid->xdk360_render_device->SetPixelShader(vid->pPixelShader);

   vid->xdk360_render_device->SetVertexDeclaration(vid->pVertexDecl);
   vid->xdk360_render_device->SetStreamSource(0, vid->vertex_buf, 0, sizeof(DrawVerticeFormats));
   vid->xdk360_render_device->SetSampler(0, vid->lpTexture);

   vid->xdk360_render_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
   vid->xdk360_render_device->Present(NULL, NULL, NULL, NULL);

   return true;
}

static void xdk360_gfx_set_nonblock_state(void *data, bool state)
{
   (void)data;
   (void)state;
}

static bool xdk360_gfx_alive(void *data)
{
   (void)data;
   return !g_quitting;
}

static bool xdk360_gfx_focus(void *data)
{
   (void)data;
   return true;
}

const video_driver_t video_xdk360 = {
   xdk360_gfx_init,
   xdk360_gfx_frame,
   xdk360_gfx_set_nonblock_state,
   xdk360_gfx_alive,
   xdk360_gfx_focus,
   NULL,
   xdk360_gfx_free,
   "xdk360"
};

