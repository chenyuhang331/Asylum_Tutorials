//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/dxext.h"

#define HELP_TEXT	"Hold left button to rotate camera\nor right button to rotate object\n\n1 - reflection\n2 - single refraction\n3 - chromatic dispersion\n4 - double refraction\n+/- adjust refractive index\n\nRefractive index is: "

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

extern LPDIRECT3DDEVICE9 device;

// tutorial variables
LPD3DXMESH						skymesh			= NULL;
LPD3DXEFFECT					skyeffect		= NULL;
LPD3DXEFFECT					fresnel			= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;

DXObject*						object1			= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				normals			= NULL;
LPDIRECT3DTEXTURE9				positions		= NULL;
LPDIRECT3DCUBETEXTURE9			skytex			= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;

LPDIRECT3DSURFACE9				normalsurf		= NULL;
LPDIRECT3DSURFACE9				positionsurf	= NULL;

bool							isdoublerefract	= false;
state<D3DXVECTOR2>				cameraangle;
state<D3DXVECTOR2>				objectangle;
D3DXVECTOR4						refrindices(1.000293f, 1.25f, 0, 0);

float quadvertices[36] =
{
						-0.5f,						-0.5f, 0, 1,	0, 0,
	(float)screenwidth - 0.5f,						-0.5f, 0, 1,	1, 0,
						-0.5f, (float)screenheight - 0.5f, 0, 1,	0, 1,

						-0.5f, (float)screenheight - 0.5f, 0, 1,	0, 1,
	(float)screenwidth - 0.5f,						-0.5f, 0, 1,	1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1,	1, 1
};

float textvertices[36] =
{
	9.5f,			9.5f,	0, 1,	0, 0,
	521.5f,			9.5f,	0, 1,	1, 0,
	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	512.0f + 9.5f,	0, 1,	1, 1
};

void UpdateText()
{
	std::string str(256, ' ');
	
	sprintf(&str[0], "%s%f", HELP_TEXT, refrindices.y);
	DXRenderText(str.c_str(), text, 512, 512);
}

HRESULT InitScene()
{
	HRESULT hr;
	D3DCAPS9 caps;

	device->GetDeviceCaps(&caps);

	if( caps.VertexShaderVersion < D3DVS_VERSION(3, 0) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0) )
	{
		MYERROR("This demo requires Shader Model 3.0 capable video card");
		return E_FAIL;
	}

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateVertexDeclaration(elem, &quaddecl));

	object1 = new DXObject(device);

	//if( !object1->Load("../media/meshes/teapot.X") )
	if( !object1->Load("../media/meshes/knot.X") )
	{
		MYERROR("Could not load object1");
		return E_FAIL;
	}

	MYVALID(D3DXLoadMeshFromXA("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));
	MYVALID(D3DXCreateCubeTextureFromFileA(device, "../media/textures/sky7.dds", &skytex));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));

	MYVALID(DXCreateEffect("../media/shaders/fresneleffects.fx", device, &fresnel));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", device, &skyeffect));

	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normals, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &positions, NULL));
	MYVALID(device->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	
	normals->GetSurfaceLevel(0, &normalsurf);
	positions->GetSurfaceLevel(0, &positionsurf);

	UpdateText();

	cameraangle = D3DXVECTOR2(-0.25f, 0.25f);
	objectangle = D3DXVECTOR2(0, 0);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	delete object1;
	
	SAFE_RELEASE(normalsurf);
	SAFE_RELEASE(positionsurf);

	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(skytex);
	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(fresnel);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(normals);
	SAFE_RELEASE(positions);
	SAFE_RELEASE(text);
	SAFE_RELEASE(quaddecl);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	switch( wparam )
	{
	case 0x31:
		fresnel->SetTechnique("reflection");
		isdoublerefract = false;
		break;

	case 0x32:
		fresnel->SetTechnique("singlerefraction");
		isdoublerefract = false;
		break;

	case 0x33:
		fresnel->SetTechnique("dispersion");
		isdoublerefract = false;
		break;

	case 0x34:
		fresnel->SetTechnique("doublerefraction");
		isdoublerefract = true;
		break;

	case VK_ADD:
		refrindices.y = min(refrindices.y + 0.05f, 3.0f);
		UpdateText();
		break;

	case VK_SUBTRACT:
		refrindices.y = max(refrindices.y - 0.05f, 1.1f);
		UpdateText();
		break;

	default:
		break;
	}
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 velocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;
	objectangle.prev = objectangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += velocity * 0.004f;

	if( mousedown == 2 )
		objectangle.curr -= velocity * 0.004f;

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
		cameraangle.curr.y = 1.5f;

	if( cameraangle.curr.y <= -1.5f )
		cameraangle.curr.y = -1.5f;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	// camera
	D3DXMATRIX		view, proj, viewproj;
	D3DXMATRIX		skyworld, inv;
	D3DXMATRIX		world;

	D3DXVECTOR4		lightPos;
	D3DXVECTOR3		eye(0, 0, -2.5f);
	D3DXVECTOR3		look(0, 0, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);
	D3DXVECTOR2		orient2	= objectangle.smooth(alpha);

	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	D3DXMatrixScaling(&inv, 0.3f, 0.3f, 0.3f);
	//D3DXMatrixScaling(&inv, 0.5f, 0.5f, 0.5f);
	D3DXMatrixRotationYawPitchRoll(&world, orient2.x, orient2.y, 0);
	D3DXMatrixMultiply(&world, &inv, &world);

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

		// render sky
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		D3DXMatrixScaling(&skyworld, 20, 20, 20);
		skyeffect->SetMatrix("matWorld", &skyworld);

		D3DXMatrixIdentity(&skyworld);
		skyeffect->SetMatrix("matWorldSky", &skyworld);

		skyeffect->SetMatrix("matViewProj", &viewproj);
		skyeffect->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		skyeffect->Begin(0, 0);
		skyeffect->BeginPass(0);
		{
			device->SetTexture(0, skytex);
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();

		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// render scene
		D3DXMatrixInverse(&inv, NULL, &world);

		fresnel->SetMatrix("matViewProj", &viewproj);
		fresnel->SetMatrix("matWorld", &world);
		fresnel->SetMatrix("matWorldInv", &inv);
		fresnel->SetVector("eyePos", (D3DXVECTOR4*)&eye);
		fresnel->SetVector("refractiveindices", &refrindices);

		if( isdoublerefract )
		{
			LPDIRECT3DSURFACE9 oldtarget = NULL;

			device->GetRenderTarget(0, &oldtarget);
			device->SetRenderTarget(0, normalsurf);
			device->SetRenderTarget(1, positionsurf);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

			fresnel->SetTechnique("geombuffer");

			fresnel->Begin(0, 0);
			fresnel->BeginPass(0);
			{
				object1->Draw(DXObject::Opaque|DXObject::Transparent);
			}
			fresnel->EndPass();
			fresnel->End();

			device->SetRenderTarget(0, oldtarget);
			device->SetRenderTarget(1, 0);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

			fresnel->SetTechnique("doublerefraction");
			oldtarget->Release();
		}

		fresnel->Begin(0, 0);
		fresnel->BeginPass(0);
		{
			device->SetTexture(0, skytex);

			if( isdoublerefract )
			{
				device->SetTexture(1, normals);
				device->SetTexture(2, positions);
			}

			object1->Draw(DXObject::Opaque|DXObject::Transparent);
		}
		fresnel->EndPass();
		fresnel->End();

		device->SetTexture(1, 0);
		device->SetTexture(2, 0);

		// render text
		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetTexture(0, text);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, 6 * sizeof(float));

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************