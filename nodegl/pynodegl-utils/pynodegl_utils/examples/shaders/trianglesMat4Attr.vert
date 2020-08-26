#include "common.vert.h"
precision highp float;
layout (location = 0) in vec3 ngl_position;
layout (location = 1) in mat4 matrix;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;
};   

void main() {
    setPos(ngl_projection_matrix * ngl_modelview_matrix * matrix * vec4(ngl_position, 1.0));
}
