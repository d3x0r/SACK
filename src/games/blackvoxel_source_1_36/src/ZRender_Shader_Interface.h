#ifndef ZRENDER_SHADER_DEFINED
#define ZRENDER_SHADER_DEFINED

#include <stdhdrs.h>
#include <image3d.h>

//#include "ZRender_Interface.h"

class ZRender_Shader_Interface 
{
protected:
	int glProgramId;
	class ZRender_Interface *render;
public:
	PImageShaderTracker shader;

};

#endif