#version 120

//<vertex>
	uniform vec3 uNormal;
	
	varying vec3 vVertexPositionWorldSpace;
	varying vec3 vVertexPosition;	
	varying vec3 vVertexNormal;	

	varying vec3 vPlaneNormal;
	
	
	void main()
	{		
	    vVertexPositionWorldSpace = gl_Vertex.xyz; // World space
	    
		// Transform the vertex position to eye space
		gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex; // ftransform() is deprecated
		vVertexPosition = gl_Position.xyz;
	       
		// Transform the vertex normal to eye space
		vVertexNormal = normalize(gl_NormalMatrix * gl_Normal);
		vPlaneNormal = normalize (gl_NormalMatrix * uNormal);
		
		gl_FrontColor = gl_Color;
		gl_BackColor = vec4(0.0, 0.0, 1.0, 1.0);
		gl_FrontSecondaryColor = gl_SecondaryColor;
		gl_BackSecondaryColor = vec4(0.0, 0.0, 1.0, 1.0);
	}
//</vertex>


//<fragment>
	uniform vec3 uPlanePoint;
	uniform vec3 uNormal;
	uniform int uShadingMode;
		
	varying vec3 vVertexPositionWorldSpace;
	varying vec3 vVertexPosition;	
	varying vec3 vVertexNormal;	

	varying vec3 vPlaneNormal;
	
	const vec4 AMBIENT_BLACK = vec4(0.0, 0.0, 0.0, 1.0);
	const vec4 DEFAULT_BLACK = vec4(0.0, 0.0, 0.0, 0.0);

	void main()
	{		
	
		// discard pixels behind the cutting plane
	    //(v - p) * N
		float isBeforePlane = dot((vVertexPositionWorldSpace - uPlanePoint), uNormal);		
		if(isBeforePlane <= 0.0) {
			discard;
		}	

		gl_FragColor = gl_Color;		
	}
//</fragment>
