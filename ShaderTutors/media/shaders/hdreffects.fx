
sampler2D sampler0 : register(s0);
sampler2D sampler1 : register(s1);
sampler2D sampler2 : register(s2);
sampler2D sampler3 : register(s3);
sampler2D sampler4 : register(s4);
sampler2D sampler5 : register(s5);

float2	texelsize;
float	targetluminance = 0.03f;
float	elapsedtime;

int		starpass	= 0;
int		stardir		= 0;

static const float2 staroffsets[4] =
{
	{ -1, -1 },
	{  1, -1 },
	{ -1,  1 },
	{  1,  1 }
};

static const float2 avglumoffsets3x3[9] =
{
	{ -1, -1 }, { 1, -1 }, { -1, 1 },
	{  1, 1 }, { 0, 0 }, { 1, 0 },
	{ -1, 0 }, { 0, 1 }, { 0, -1 }
};

static const float2 avglumoffsets4x4[16] =
{
	{ -1.5f, -1.5f }, { -0.5f, -1.5f }, {  0.5f, -1.5f }, {  1.5f, -1.5f },
	{ -1.5f, -0.5f }, { -0.5f, -0.5f }, {  0.5f, -0.5f }, {  1.5f, -0.5f },
	{ -1.5f, 0.5f }, { -0.5f, 0.5f }, {  0.5f, 0.5f }, {  1.5f, 0.5f },
	{ -1.5f, 1.5f }, { -0.5f, 1.5f }, {  0.5f, 1.5f }, {  1.5f, 1.5f }
};

static const float2 offsetsx[9] =
{
	{ -4, 0 },
	{ -3, 0 },
	{ -2, 0 }, 
	{ -1, 0 },
	{ 0,  0 },
	{ 1,  0 },
	{ 2,  0 },
	{ 3,  0 }, 
	{ 4,  0 },
};

static const float2 offsetsy[9] =
{
	{ 0, -4 },
	{ 0, -3 },
	{ 0, -2 }, 
	{ 0, -1 },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 }, 
	{ 0,  4 },
};

// gaussian weights
static const float weights[9] =
{
	1.85548e-006f,
	0.000440052f,
	0.0218759f,
	0.227953f,
	0.497894f,
	0.227953f,
	0.0218759f,
	0.000440052f,
	1.85548e-006f,
};

// =======================================================================
//
// Luminance adaptation
//
// =======================================================================

void ps_avglum(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	float3 sample;
	float3 lumvec = float3(0.2125f, 0.7154f, 0.0721f);
	float sum = 0;

	for( int i = 0; i < 9; ++i )
	{
		sample = tex2D(sampler0, tex + texelsize * avglumoffsets3x3[i]);
		sum += log(dot(sample, lumvec) + 0.0001f);
	}

	sum *= 0.1111f;
	color0 = float4(sum, sum, sum, 1);
}

void ps_avglumdownsample(
	in	 float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	float sum = 0;

	for( int i = 0; i < 16; ++i )
		sum += tex2D(sampler0, tex + texelsize * avglumoffsets4x4[i]).r;

	sum *= 0.0625f;
	color0 = float4(sum, sum, sum, 1);
}

void ps_avglumfinal(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	float sum = 0;

	for( int i = 0; i < 16; ++i )
		sum += tex2D(sampler0, tex + texelsize * avglumoffsets4x4[i]).r;

	sum = exp(sum * 0.0625f);
	color0 = float4(sum, sum, sum, 1);
}

void ps_adaptluminance(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	float adaptedlum = tex2D(sampler0, float2(0.5f, 0.5f));
	float targetlum = tex2D(sampler1, float2(0.5f, 0.5f));

	float newadaptation = adaptedlum + (targetlum - adaptedlum) * (1 - pow(0.98f, 30 * elapsedtime));
	color0 = float4(newadaptation, newadaptation, newadaptation, 1);
}

// =======================================================================
//
// Bloom related shaders
//
// =======================================================================

void ps_brightpass(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR)
{
	float4 color = tex2D(sampler0, tex);
	float exposure = targetluminance / tex2D(sampler5, float2(0.5f, 0.5f)).r;

	color = color * (exposure * 0.002f) - 0.002f;
	color0 = min(max(color, 0), 16384.0f);

	color0.a = 1;
}

void ps_downsample(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	color0 = 0;

	// 4x4 cross filter
	color0 += tex2D(sampler0, tex + texelsize * float2(-1, -1));
	color0 += tex2D(sampler0, tex + texelsize * float2(-1, 1));
	color0 += tex2D(sampler0, tex + texelsize * float2(1, -1));
	color0 += tex2D(sampler0, tex + texelsize * float2(1, 1));

	color0 *= 0.25f;
	color0.a = 1;
}

void ps_blurx(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	color0 = 0;

	for( int i = 0; i < 9; ++i )
		color0 += tex2D(sampler0, tex + texelsize * offsetsx[i]) * weights[i];

	color0.a = 1;
}

void ps_blury(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	color0 = 0;

	for( int i = 0; i < 9; ++i )
		color0 += tex2D(sampler0, tex + texelsize * offsetsy[i]) * weights[i];

	color0.a = 1;
}

void ps_blurcombine(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR)
{
	float4 c0 = tex2D(sampler0, tex) * 7;
	float4 c1 = tex2D(sampler1, tex) * 5;
	float4 c2 = tex2D(sampler2, tex) * 3.5f;
	float4 c3 = tex2D(sampler3, tex) * 2.5f;
	float4 c4 = tex2D(sampler4, tex) * 1.75f;

	color0 = c0 + c1 + c2 + c3 + c4;
	color0.a = 1;
}

// =======================================================================
//
// Afterimage
//
// =======================================================================

void ps_afterimage(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR)
{
	float4 prev = tex2D(sampler0, tex);
	float4 current = tex2D(sampler1, tex);

	color0 = prev * 0.998f + current * 0.011f;
	color0 = min(max(color0, 0), 256);

	color0.a = 1;
}

// =======================================================================
//
// Star
//
// =======================================================================

void ps_star(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	color0 = 0;

	float b = pow(4, starpass);
	float a = 0.9f;

	float2 off = staroffsets[stardir];
	float2 stex;

	for( int i = 0; i < 4; ++i )
	{
		stex = tex + b * i * texelsize * off;

		color0 +=
			tex2D(sampler0, stex) * pow(a, b * i);
	}

	color0.a = 1;
}

void ps_starcombine(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR)
{
	float4 c0 = tex2D(sampler0, tex);
	float4 c1 = tex2D(sampler1, tex);
	float4 c2 = tex2D(sampler2, tex);
	float4 c3 = tex2D(sampler3, tex);

	color0 = c0 + c1 + c2 + c3;
	color0.a = 1;
}

// =======================================================================
//
// Lens flare
//
// =======================================================================

void ps_ghost(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float2 a = (tex - 0.5f) * -1.724f;
	float2 b = (tex - 0.5f) * -2.845f;
	float2 c = (tex - 0.5f) * -1.0f;
	float2 d = (tex - 0.5f) * -1.957f;
	float2 e = (tex - 0.5f) * -2.147f;
	float2 f = (tex - 0.5f) * -4.0f;
	float2 g = (tex - 0.5f) * -1.794f;
	
	float sa = saturate(1 - dot(a, a) * 3);
	float sb = saturate(1 - dot(b, b) * 3);
	float sc = saturate(1 - dot(c, c) * 3);
	float sd = saturate(1 - dot(d, d) * 3);
	float se = saturate(1 - dot(e, e) * 3);
	float sf = saturate(1 - dot(f, f) * 3);
	float sg = saturate(1 - dot(g, g) * 3);
	
	a += 0.5f;
	b += 0.5f;
	c += 0.5f;
	d += 0.5f;
	e += 0.5f;
	f += 0.5f;
	g += 0.5f;

	color = tex2D(sampler0, a) * sa * float4(0.250f, 0.175f, 0.125f, 1);
	color += tex2D(sampler1, b) * sb * float4(0.131f, 0.187f, 0.131f, 1);
	color += tex2D(sampler0, c) * sc * float4(0.103f, 0.103f, 0.103f, 1);
	color += tex2D(sampler1, d) * sd * float4(0.2f, 0.2f, 0.250f, 1);
	color += tex2D(sampler0, e) * se * float4(0.101f, 0.050f, 0.050f, 1);
	color += tex2D(sampler1, f) * sf * float4(0.102f, 0.102f, 0.102f, 1);
	color += tex2D(sampler2, g) * sg * float4(0.248f, 0.248f, 0.248f, 1);
	
	color.a = 1;
}

// =======================================================================
//
// Final combine
//
// =======================================================================

void ps_final(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR)
{
	float4 base = tex2D(sampler0, tex);
	float4 bloom = tex2D(sampler1, tex);
	float4 star = tex2D(sampler2, tex);
	float4 ghost = tex2D(sampler3, tex);
	float4 after = tex2D(sampler4, tex);

	float exposure = targetluminance / tex2D(sampler5, float2(0.5f, 0.5f)).r;

	float4 r1 = (bloom + star + ghost + after) * 6;
	float4 r0 = base * exposure + r1;

	tex -= 0.5f;
	float v = 1 - dot(tex, tex);

	r0 *= v * v * v;

	color0 = r0;
	color0.a = 1;
}

// =======================================================================
//
// Techniques
//
// =======================================================================

technique brightpass
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_brightpass();
	}
}

technique downsample
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_downsample();
	}
}

technique avglum
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_avglum();
	}
}

technique avglumdownsample
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_avglumdownsample();
	}
}

technique avglumfinal
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_avglumfinal();
	}
}

technique adaptluminance
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_adaptluminance();
	}
}

technique blurx
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_blurx();
	}
}

technique blury
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_blury();
	}
}

technique star
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_star();
	}
}

technique blurcombine
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_blurcombine();
	}
}

technique ghost
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_ghost();
	}
}

technique afterimage
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_afterimage();
	}
}

technique starcombine
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_starcombine();
	}
}

technique final
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_final();
	}
}
