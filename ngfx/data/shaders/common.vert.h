#version 450
out gl_PerVertex {
    vec4 gl_Position;   
};

void setPos(vec4 pos) {
	gl_Position = vec4(pos.x, pos.y, 0.5f * ( pos.z + pos.w ), pos.w);
}
