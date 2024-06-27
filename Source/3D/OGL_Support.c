#include <3ds.h>

#include <GL/picaGL.h>
#include <GL/glu.h>

static void sceneRender(void)
{
	
	glColor3f(1.0f, 0.0f, 0.0f);

	glBegin(GL_TRIANGLES);
	  glTexCoord2f(0.0f, 0.0f);
	  glVertex3f(-1.0f, -1.0f, 0.0f );
	  glTexCoord2f(0.0f, 1.0f);
	  glVertex3f(-1.0f,  1.0f, 0.0f );
	  glTexCoord2f(1.0f, 1.0f);
	  glVertex3f( 1.0f,  1.0f, 0.0f );
	glEnd();
}

int OGL_DrawScene(void*)
{
	//Initialize the LCD framebuffers
	gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false); 

	//Initialize the openGL context
	pglInit();

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glViewport(0,0, 400, 240);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_TEXTURE_2D);

	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(80.0f, 400.f/240.0f, 0.01f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float angle = 0;

	while (aptMainLoop())
	{
		gspWaitForVBlank();

		hidScanInput();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		glTranslatef(0.0, 0, -2.5f);
		glRotatef(angle , 0, 0, 1);

		sceneRender();
		
		pglSwapBuffers();

		angle += 0.5f;
	}

	pglExit();
	gfxExit();
	
	return 0;
}