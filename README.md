# KoopaEngine

3D Programming has never been easier!

![demo](koopaEngineDemo1Trim4.gif)

```c
#include "KoopaEngine.h"

int main()
{
	KoopaEngine ke = KoopaEngine();	


	while (!ke.shouldCloseWindow())
	{
		ke.BeginFrame();

		ke.ClearScreen(Vec4(0.2f, 0.2f, 0.2f, 0.4f));

		//LIGHTING----------------------------------------------------------------------------------
		ke.DrawPointLight(Vec3(0.0f, 4.0f, 0.0f), //pos
				Vec3(1.0f, 1.0f, 1.0f),   //color
				5.0f			  //intensity
		);

		ke.DrawPointLight(Vec3(4.0f, 1.0f, cosf(ke.GetCurrentFrame())),
				Vec3(1.0f, 1.0f, 0.5f),
				2.0f
		);

		ke.DrawDirLight(Vec3(1.0f, -1.0f, 1.0f), //direction
				Vec3(1.0f, 1.0f, 1.0f),
				0.5
		);

		ke.DrawLightsDebug(); //show where light sources are

		//GEOMETRY----------------------------------------------------------------------------------
		ke.SetCurrentDiffuseTexture("wood.png");
		ke.SetCurrentNormalTexture("brickwall_normal.jpg");

		ke.DrawPlane(
			Vec3(0.0f, -0.5f, 0.0f),	//pos
			Vec2(20.0f, 20.0f),		//size
			Vec4(0.0f, 1.0f, 0.0f, 0.0f)	//rot
		);

		ke.DrawTriangle(
			Vec3(0.0f, 0.0f, 0.0f),
			Vec4(0.0f, 1.0f, 0.0f, 0.0f)
		);
		
		ke.SetCurrentDiffuseTexture("brickwall.jpg");
		
		ke.DrawCube(
			Vec3(2.0f, 2.0f, 0.0f),
			Vec3(1.0f, 2.0f, 1.0f),
			Vec4(0.0f, 1.0f, 1.0f, 150.0f * ke.GetCurrentFrame())
		);

		ke.DrawCube(
			Vec3(0.0f, 0.0f, 0.0f),
			Vec3(1.0f, 1.0f, 1.0f),
			Vec4(0.0f, 1.0f, 0.0f, 20.0f * ke.GetCurrentFrame())
		);

		ke.EndFrame();
	}

	return 0;
}
```

