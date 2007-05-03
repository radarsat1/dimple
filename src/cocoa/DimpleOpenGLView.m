#import "DimpleOpenGLView.h"
#include <OpenGL/gl.h>
#include "../test.h"

@implementation DimpleOpenGLView


static void drawAnObject ()

{
    glColor3f(1.0f, 0.85f, 0.35f);

	test_draw();

    glEnd();

}


- (void) drawRect: (NSRect) bounds
{
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	drawAnObject();
	glFlush();
}

@end
