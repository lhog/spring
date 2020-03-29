#version 150 compatibility

struct Data {
	vec2 mapUV;
};

in Data dataGS;

void main() {
	//discard;
	gl_FragColor = vec4(dataGS.mapUV.xy, 1.0, 1.0);
}