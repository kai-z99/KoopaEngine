# KoopaEngine

3D Programming has never been easier!

### PBR/IBL Support
![demo1](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoPBR1.gif)
![demo2](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoPBR2.gif)

### Cascaded/Variance Shadow Mapping, Terrain, Fog
![demo2](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoGeneric.gif)

### Forward+ Rendering
![demo3](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoFwdPlus.gif)

### Particles
![demo3](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoParticle.gif)

### Fake Subsurface Scattering
![demo3](https://github.com/kai-z99/KoopaEngine/blob/master/demo/koopaEngineDemoSSS.gif)

### And much more...


## Example Code
```c
#include "KoopaEngine.h"

#include <random>

int main()
{
	KoopaEngine ke = KoopaEngine();

	std::vector<const char*> faces =
	{
		"../ShareLib/Resources/interstellar/xneg.png",
		"../ShareLib/Resources/interstellar/xpos.png",
		"../ShareLib/Resources/interstellar/ypos.png",
		"../ShareLib/Resources/interstellar/yneg.png",
		"../ShareLib/Resources/interstellar/zneg.png",
		"../ShareLib/Resources/interstellar/zpos.png"
	};

	ke.SetSkybox(faces);
	ke.SetDrawLightsDebug(true);
	ke.SetCameraExposure(0.7f);
	ke.SetCameraSpeed(30.0f);
	ke.SetFogColor(Vec3(0.9f, 0.5f, 0.2f));
	ke.SetExpFogDensity(0.22f);
	ke.SetFogType(EXPONENTIAL_SQUARED);
	ke.SetAmbientLighting(0.05f);
	ke.SetLinearFogStart(70.4f);
	ke.SetBloomThreshold(3.0f);

	
	//ke.CreateParticleEmitter(30, 1024, Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	//ke.CreateParticleEmitter(30, 1024 * 128, Vec3(5.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	

	while (!ke.shouldCloseWindow())
	{
		ke.BeginFrame();
		float t = ke.GetCurrentFrame();	

		ke.ClearScreen(Vec4(0.0f, 0.0f, 0.0f, 0.4f));
		
		
		ke.DrawPointLight(
			Vec3(0.0f, 5 + 5 * sin(t), -1.0f),	//position
			Vec3(1.0f, 1.0f, 1.0f),			//col
			30.0f,					//range
			3.5f,					//intensity
			1					//cast shadows
		);		
		
		ke.DrawPointLight(Vec3(1.0f, 1 + sinf(t), -5.0f), Vec3(0.1f, 0.8f, 1.0f), 50.0f, 1.0f, 1);
		ke.DrawPointLight(Vec3(-4.0f, 1 + cosf(t), 1.0f), Vec3(0.1f, 1.8f, 0.5f), 40.0f, 1.0f, 0);
		
		//LOWER PLANE
		//ke.SetCurrentDiffuseTexture("Resources/marble_tile.jpg");
		ke.SetCurrentDiffuseTexture(Vec3(1.0f, 1.0f, 1.0f));
		ke.SetCurrentBaseSpecular(0.1f);
		//ke.SetCurrentNormalTexture("Resources/marble_tile_normal.jpg");
		ke.DrawPlane(
			Vec3(0, -0.5f, 0),			//pos
			Vec2(2000.0f, 2000.0f),			//size
			Vec4(0.0f, 1.0f, 0.0f, 0.0f)		//rot
		);
		ke.ResetMaterial();

		//ke.DrawDirLight(Vec3(-1, -1.0f, -1), Vec3(1.0f, 1.0f, 1.0f), 2.0f, 1);
		
		//TERRAIN
		ke.SetCurrentDiffuseTexture("Resources/rock.png");
		ke.SetCurrentBaseSpecular(0.0f);
		ke.DrawTerrain("Resources/iceland_heightmap.png", Vec3(0, -20, 0), Vec3(0.3, 0.3, 0.3));
		ke.ResetMaterial();
		
		//LOWER PLANE
		ke.SetCurrentDiffuseTexture("Resources/marble_tile.jpg");
		ke.SetCurrentDiffuseTexture(Vec3(1.0f, 1.0f, 1.0f));
		ke.SetCurrentBaseSpecular(0.1f);
		ke.SetCurrentNormalTexture("Resources/marble_tile_normal.jpg");
		ke.DrawPlane(
			Vec3(0, -0.5f, 0),		//pos
			Vec2(20.0f,20.0f),		//size
			Vec4(0.0f, 1.0f, 0.0f, 0.0f)	//rot
		);
		ke.ResetMaterial();

		//UPPER PLANE
		ke.SetCurrentDiffuseTexture(Vec3(1.0f, 1.0f, 1.0f));
		ke.SetCurrentBaseSpecular(0.1f);
		ke.SetCurrentNormalTexture("Resources/brickwall_normal.jpg");
		ke.DrawPlane(
			Vec3(0, 7.5f, -10.0f),		//pos 
			Vec2(20.0f, 20.0f),		//size
			Vec4(1.0f, 0.0f, 0.0f, 90.0f)	//rot
		);
		ke.ResetMaterial();
								
		//CUBE 1
		ke.SetCurrentDiffuseTexture("Resources/marble_tile.jpg");
		ke.SetCurrentNormalTexture("Resources/brickwall_normal.jpg");
		//ke.SetCurrentSpecularTexture("Resources/brickwall_specular.jpg");
		ke.DrawCube(
			Vec3(5.0f, 2.0f, 0.0f),
			Vec3(1.0f, 2.0f, 1.0f),
			Vec4(0.0f, 1.0f, 1.0f, 150.0f * t)
		);
		
		//CUBE 2 (using prev material)
		ke.DrawCube(
			Vec3(0.0f, 0.0f, -4.0f),
			Vec3(0.3f, 6.0f, 0.3f),
			Vec4(0.0f, 1.0f, 0.0f, 20.0f)
		);
		ke.ResetMaterial();
					
		//SPHERE
		
		ke.SetCurrentDiffuseTexture("Resources/rock.png");
		ke.SetCurrentNormalTexture("Resources/rock_normal.png");
		ke.DrawSphere(
			Vec3(0, 2, -4),
			Vec3(1, 1, 1),
			Vec4(0, 1, 0, 30 * t)
		);
		ke.ResetMaterial();
		
		
		ke.DrawModel("Resources/woodDragon/dragon.obj", true, Vec3(0.0f, 0.45f, 0.0f),
			{ 1.0f, 1.0f, 1.0f }, Vec4(0, 1, 0, 10 * t));
		
		
		ke.DrawModel("Resources/animeChar/animeChar.obj", true, Vec3(-2.5f, -0.5f, 0.0f),
			{ 2.0f, 2.0f, 2.0f }, Vec4(0, 1, 0, 10 * t));
		
		ke.EndFrame();
	}

	return 0;
}
```

