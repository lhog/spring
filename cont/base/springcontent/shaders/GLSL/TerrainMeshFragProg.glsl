#version 150 compatibility

struct Data {
	vec2 mapUV;
};

in Data dataTES;

void main() {
	gl_FragColor = vec4(dataTES.mapUV.xy, 1.0, 1.0);
}