#pragma once

#define _USE_MATH_DEFINES


#include "volumeshop.h"

#include <list>
#include <set>
#include <map>

#include <iostream>
#include <fstream>
#include <string>


#include "RendererPlugin.h"
#include "PluginInstance.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Exception.h"
#include "ResourceTexture2D.h"

class RendererMeshSplitting : public Renderer, public PluginInstance<RendererPlugin>
{

public:

	RendererMeshSplitting(Plugin & pluPlugin) : PluginInstance(pluPlugin), m_uWidth(1), m_uHeight(1), m_bPicking(false), 
		m_shaShader(pluPlugin.GetPluginDirectory()+"plugin_renderer_meshsplitting.glsl"), m_bUpdateGroups(true), 
		m_fraFramebuffer(m_uWidth,m_uHeight), m_texColorTexture0(GL_RGBA,GL_RGBA16F_ARB), m_texColorTexture1(GL_RGBA,GL_RGBA16F_ARB), 
		m_texColorTexture2(GL_RGBA,GL_RGBA16F_ARB), m_texColorTexture3(GL_RGBA,GL_RGBA16F_ARB), m_texColorTexture4(GL_RGBA,GL_RGBA16F_ARB), 
		m_texColorTexture5(GL_RGBA,GL_RGBA16F_ARB), m_texDepthTexture(GL_DEPTH_COMPONENT), m_bMouseDownInWindow(false)
	{
		m_modVariantObserver.connect(this,&RendererMeshSplitting::propertyModified);
		m_modMeshObserver.connect(this,&RendererMeshSplitting::meshModified);

		GetPlugin().GetProperty("Projection Transformation").require(Variant::TypeMatrix());
		GetPlugin().GetProperty("Projection Transformation").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Viewing Transformation").require(Variant::TypeMatrix());
		GetPlugin().GetProperty("Viewing Transformation").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Mesh").require(Variant::TypeHandle());
		GetPlugin().GetProperty("Mesh").addObserver(&m_modMeshObserver);

		GetPlugin().GetProperty("Mode").require((Variant::TypeOption(), Variant("Default"), Variant("Normal"), Variant("Fancy"), Variant("Wireframe")));
		GetPlugin().GetProperty("Mode").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Stylized").require(Variant::TypeBoolean(false));
		GetPlugin().GetProperty("Stylized").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Color").require(Variant::TypeColor());
		GetPlugin().GetProperty("Color").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Opacity").require(Variant::TypeFloat(1.0f, 0.0f, 1.0f));
		GetPlugin().GetProperty("Opacity").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Selection").require(Variant::TypeString());
		GetPlugin().GetProperty("Selection").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Plane Translation").require(Variant::TypeVector(Vector(0.0f, 0.0f, 0.0f)));
		GetPlugin().GetProperty("Plane Translation").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Plane Rotation Vector").require(Variant::TypeVector(Vector(1.0f, 0.0f, 0.0f)));
		GetPlugin().GetProperty("Plane Rotation Vector").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Plane Rotation Angle").require(Variant::TypeFloat(0.0f, -180.0f, 180.0f));
		GetPlugin().GetProperty("Plane Rotation Angle").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Plane Color").require(Variant::TypeColor(Color(0.0f, 0.0f, 1.0f, 0.2f)));
		GetPlugin().GetProperty("Plane Color").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Plane Scale").require(Variant::TypeVector(Vector(1.0f, 1.0f, 1.0f)));
		GetPlugin().GetProperty("Plane Scale").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Offset").require(Variant::TypeFloat(0.5f, 0.0f, 10.0f));
		GetPlugin().GetProperty("Offset").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Speed").require(Variant::TypeFloat(1.0f, 1.0f, 10.0f));
		GetPlugin().GetProperty("Speed").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("DynamicOffset").require(Variant::TypeBoolean(false));
		GetPlugin().GetProperty("DynamicOffset").addObserver(&m_modVariantObserver);

		GetPlugin().GetProperty("Interior Shading Mode").require((Variant::TypeOption(), Variant("No shading"), Variant("Flat red shading"), Variant("Padded phong shading"), Variant("Caved phong shading")));
		GetPlugin().GetProperty("Interior Shading Mode").addObserver(&m_modMeshObserver);

		Handle hanMesh = GetPlugin().GetProperty("Mesh");
		TriangleMesh *pMesh = hanMesh.GetResource<TriangleMesh>();

		if (pMesh)
			updateGroups(*pMesh);

		ideal_offset[0]=GetPlugin().GetProperty("Offset");
		ideal_offset[1]=-((float)GetPlugin().GetProperty("Offset"));

		current_offset[0]=0.0f;
		current_offset[1]=0.0f;

		GetPlugin().GetProperty("Groups").addObserver(&m_modVariantObserver);
};

virtual ~RendererMeshSplitting()
{
};

virtual void idle()
{
};

virtual void display(Canvas & canCanvas)
{


	GLuint queryID[]={0,0};
	GLuint queryState;

	glGenQueries(2, queryID);


	const Matrix matProjectionTransformation = GetPlugin().GetProperty("Projection Transformation");
	const Matrix matViewingTransformation = GetPlugin().GetProperty("Viewing Transformation");


	Handle hanMesh = GetPlugin().GetProperty("Mesh");
	TriangleMesh *pMesh = hanMesh.GetResource<TriangleMesh>();

	if (!pMesh)
		return;

	const Matrix matMeshTransformation = pMesh->GetProperty("Transformation",Matrix());

	updateGroups(*pMesh);


	canCanvas.bind();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	// define the projection matrix

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(matProjectionTransformation.Get());

	// define the model view matrix

	glMatrixMode(GL_MODELVIEW);


	// Read values from user input
	const Vector vecPlaneTranslation = GetPlugin().GetProperty("Plane Translation");
	const Vector vecPlaneRotationVector = GetPlugin().GetProperty("Plane Rotation Vector");
	const float vecPlaneRotationAngle = GetPlugin().GetProperty("Plane Rotation Angle");
	const Color vecPlaneColor = GetPlugin().GetProperty("Plane Color");
	const Vector vecPlaneScaling = GetPlugin().GetProperty("Plane Scale");

	//rotate plane normal when plane rotates
	Vector planeNormal(0.0f, 0.0f, 1.0f);
	glLoadIdentity();
	glRotatef(vecPlaneRotationAngle, vecPlaneRotationVector.GetX(), vecPlaneRotationVector.GetY(), vecPlaneRotationVector.GetZ());
	float arr[16] = {0.0f};
	glGetFloatv(GL_MODELVIEW_MATRIX, arr);
	Matrix rotPlaneNormalMatrix(arr);
	planeNormal = rotPlaneNormalMatrix.GetRotated(planeNormal);
	planeNormal.normalize();

	//float offset=current_offset;

	GetOffset(*pMesh);
	float *offset=current_offset;



	//translate model with offset
	Vector modelTranslationA = planeNormal * offset[0];
	Vector modelTranslationB = planeNormal * offset[1];


	// define the lighting

	const float vfPosition[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	const float vfAmbient[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	const float vfDiffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	const float vfSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glLoadIdentity();
	glLightfv(GL_LIGHT0, GL_POSITION, vfPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, vfAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, vfDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, vfSpecular);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);


	Color colColor = GetPlugin().GetProperty("Color");
	colColor.SetNormalizedAlpha(float(GetPlugin().GetProperty("Opacity")));
	glColor4ubv(colColor.Get());
	glSecondaryColor3ub(255,255,255);

	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);

	if (GLEW_ARB_multisample)
	{
		glEnable( GL_MULTISAMPLE_ARB );
		glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE_ARB );
	}

	//shading mode
	const std::string sShadingMode = GetPlugin().GetProperty("Interior Shading Mode");

	//shading mode 0 does nothing in the shader
	int iShadingMode = 0;
	if(sShadingMode.compare("Flat red shading") == 0) {
		iShadingMode = 1;
	} else if(sShadingMode.compare("Caved phong shading") == 0) {
		iShadingMode = 3;
	} else if(sShadingMode.compare("Padded phong shading") == 0) {
		iShadingMode = 2;
	}


	//DRAW MODEL

	// update model view matrix after light position has been set
	glLoadIdentity();
	glMultMatrixf(matViewingTransformation.Get());
	glMultMatrixf(matMeshTransformation.Get());

	glBeginQuery(GL_SAMPLES_PASSED, queryID[0]);

	// Selected part(s)
	m_shaShader.SetOption("overlay","true");
	m_shaShader.SetOption("split","false");
	m_shaShader.bind();
	glUniform1i(m_shaShader.GetUniformLocation("uShadingMode"), iShadingMode);
	renderMesh(*pMesh, true, true);
	m_shaShader.release();
	glEndQuery(GL_SAMPLES_PASSED);
	queryState=0;

	

	while(queryState != GL_TRUE)
	{
		glGetQueryObjectuiv(queryID[0], GL_QUERY_RESULT_AVAILABLE, &queryState);
	}
	//Das Ergebnis aufschreiben
	glGetQueryObjectuiv(queryID[0], GL_QUERY_RESULT, &occlusions[0]);	
	
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// first half of model
	glPushMatrix();
	glTranslatef(modelTranslationA.GetX(), modelTranslationA.GetY(), modelTranslationA.GetZ());
	m_shaShader.SetOption("overlay","false");
	m_shaShader.SetOption("split","true");
	m_shaShader.bind();
	glUniform3f(m_shaShader.GetUniformLocation("uNormal"), planeNormal.GetX(), planeNormal.GetY(), planeNormal.GetZ());
	glUniform3f(m_shaShader.GetUniformLocation("uPlanePoint"), 0.0f + vecPlaneTranslation.GetX(), 0.0f + vecPlaneTranslation.GetY(), 0.0f + vecPlaneTranslation.GetZ());
	glUniform1i(m_shaShader.GetUniformLocation("uShadingMode"), iShadingMode);
	renderMesh(*pMesh, false, false);
	m_shaShader.release();

	// second half of model
	glPopMatrix();
	glPushMatrix();
	glTranslatef(modelTranslationB.GetX(), modelTranslationB.GetY(), modelTranslationB.GetZ());
	m_shaShader.SetOption("overlay","false");
	m_shaShader.SetOption("split","true");
	m_shaShader.bind();
	glUniform3f(m_shaShader.GetUniformLocation("uNormal"), -planeNormal.GetX(), -planeNormal.GetY(), -planeNormal.GetZ());
	glUniform3f(m_shaShader.GetUniformLocation("uPlanePoint"), 0.0f + vecPlaneTranslation.GetX(), 0.0f + vecPlaneTranslation.GetY(), 0.0f + vecPlaneTranslation.GetZ());
	glUniform1i(m_shaShader.GetUniformLocation("uShadingMode"), iShadingMode);
	renderMesh(*pMesh, false, false);
	m_shaShader.release();

	glBeginQuery(GL_SAMPLES_PASSED, queryID[1]);


	// Selected part(s)
	glPopMatrix();
	m_shaShader.SetOption("overlay","false");
	m_shaShader.SetOption("split","false");
	m_shaShader.bind();
	glUniform1i(m_shaShader.GetUniformLocation("uShadingMode"), iShadingMode);
	renderMesh(*pMesh, true, false);
	m_shaShader.release();

	glEndQuery(GL_SAMPLES_PASSED);
	queryState=0;
	while(queryState != GL_TRUE)
	{
		glGetQueryObjectuiv(queryID[1], GL_QUERY_RESULT_AVAILABLE, &queryState);
	}
	//Das Ergebnis aufschreiben
	glGetQueryObjectuiv(queryID[1], GL_QUERY_RESULT, &occlusions[1]);

	//printf("Samples passed: %i %i\n",occlusions[0],occlusions[1]);
	//DRAW MODEL END

	
	// DRAW PLANE
	glLoadIdentity();
	glLoadMatrixf(matViewingTransformation.Get());
	glTranslatef(vecPlaneTranslation.GetX(), vecPlaneTranslation.GetY(), vecPlaneTranslation.GetZ());
	glRotatef(vecPlaneRotationAngle, vecPlaneRotationVector.GetX(), vecPlaneRotationVector.GetY(), vecPlaneRotationVector.GetZ());
	glScalef(vecPlaneScaling.GetX(), vecPlaneScaling.GetY(), vecPlaneScaling.GetZ());

	glColor4f(vecPlaneColor.GetNormalizedRed(), vecPlaneColor.GetNormalizedGreen(), vecPlaneColor.GetNormalizedBlue(), vecPlaneColor.GetNormalizedAlpha());

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glVertex3f(-1, -1, 0);
	glVertex3f( 1, -1, 0);
	glVertex3f( 1,  1, 0);
	glVertex3f(-1,  1, 0);
	glEnd();

	glDeleteQueries(2, queryID);

	//DRAW PLANE END

	canCanvas.release();

	if(ideal_offset!=current_offset)
		GetPlugin().update();

};



virtual void overlay(Canvas & canCanvas)
{
	if (!m_bPicking)
		return;

	Handle hanMesh = GetPlugin().GetProperty("Mesh");
	TriangleMesh *pMesh = hanMesh.GetResource<TriangleMesh>();

	if (!pMesh)
		return;


	updateGroups(*pMesh);

	canCanvas.bind();

	const std::string strGroupName = GetGroupName(*pMesh, m_vecPickingPosition);
	GetPlugin().GetProperty("Selection") = strGroupName;
	m_bPicking = false;

	canCanvas.release();
};

virtual void reshape(const unsigned int uWidth, const unsigned int uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
	GetPlugin().update();
};

virtual bool button(const Controller & conController)
{
	// mouse down
	if (conController.GetActiveButton() == 0 && conController.GetActiveButtonState() && !m_bMouseDownInWindow)
	{
		m_bMouseDownInWindow = true;
		m_kLastMouseDownPosition = conController.GetActiveCursorPosition();
	}
	// mouse up
	else if (conController.GetActiveButton() == 0 && !conController.GetActiveButtonState() && m_bMouseDownInWindow)
	{
		m_bMouseDownInWindow = false;
		const Vector mouseReleasePosition = conController.GetActiveCursorPosition();
		// picking only occurs if mouse down and up happend at the same position (plus/minus a small amount)
		float epsilon = 0.001f;
		if ((mouseReleasePosition.GetX() > m_kLastMouseDownPosition.GetX()-epsilon && mouseReleasePosition.GetX() < m_kLastMouseDownPosition.GetX()+epsilon) &&
			(mouseReleasePosition.GetY() > m_kLastMouseDownPosition.GetY()-epsilon && mouseReleasePosition.GetY() < m_kLastMouseDownPosition.GetY()+epsilon) ) 
		{
			m_bPicking = true;
			m_vecPickingPosition = conController.GetActiveCursorPosition();
			GetPlugin().update(UPDATEFLAG_OVERLAY);
		}
	}

	return false;
};

virtual bool cursor(const Controller & conController)
{
	return false;
};

virtual const std::string GetButtonRole(const unsigned int uIndex) const
{
	switch (uIndex)
	{
	case 0:
		return "Pick";
	}

	return std::string();
};

virtual const std::string GetCursorRole(const unsigned int uIndex) const
{
	switch (uIndex)
	{
	case 0:
		return "Select";
	}

	return std::string();
};

protected:

	virtual void propertyModified(const Variant & varVariant, const Observable::Event & eveEvent)
	{
		bool dynamic=GetPlugin().GetProperty("DynamicOffset");

		if(dynamic){
			ideal_offset[0]=0.0f;
			ideal_offset[1]=0.0f;
			GetPlugin().update();
		}
	};

	virtual void meshModified (const Variant & varVariant, const Observable::Event & eveEvent)
	{
		if (!eveEvent.IsType<PropertyContainer::Event>())
			m_bUpdateGroups = true;

		GetPlugin().update();
	}

	virtual void updateGroups (const TriangleMesh & mesMesh)
	{
		if (m_bUpdateGroups)
		{
			if (GetPlugin().HasProperty("Groups"))
			{
				Variant & varOldVariant = GetPlugin().GetProperty("Groups");
				Variant varVariant = Variant::TypeMap();

				for (TriangleMesh::Iterator i(mesMesh);!i.IsAtEnd();++i)
				{
					const std::string & strGroupName = (*i).GetGroupName();
					Variant & varGroup = varVariant[strGroupName].require(Variant::TypeBoolean(true));

					if (varOldVariant.HasElement(strGroupName))
						varGroup = varOldVariant[strGroupName];
				}

				GetPlugin().GetProperty("Groups").swap(varVariant);
			}

			m_bUpdateGroups = false;
		}
	};

	virtual void setupAttributes(const TriangleMesh & mesMesh)
	{
		if (mesMesh.HasAttributes(TriangleMesh::ATTRIBUTETYPE_POSITION))
		{
			const AbstractAttributeArray & attPositions = mesMesh.GetAttributes(TriangleMesh::ATTRIBUTETYPE_POSITION);

			if (!attPositions.IsEmpty())
			{
				if (attPositions.GetElementComponents() >= 2 && attPositions.GetElementComponents() <= 4)
				{
					if (attPositions.GetElementOpenGLType() == GL_SHORT || attPositions.GetElementOpenGLType() == GL_INT || attPositions.GetElementOpenGLType() == GL_FLOAT || attPositions.GetElementOpenGLType() == GL_DOUBLE)
					{
						glVertexPointer(attPositions.GetElementComponents(),attPositions.GetElementOpenGLType(),0,attPositions.Get());
						glEnableClientState(GL_VERTEX_ARRAY);
					}
				}
			}
		}

		if (mesMesh.HasAttributes(TriangleMesh::ATTRIBUTETYPE_NORMAL))
		{
			const AbstractAttributeArray & attNormals = mesMesh.GetAttributes(TriangleMesh::ATTRIBUTETYPE_NORMAL);

			if (!attNormals.IsEmpty())
			{
				if (attNormals.GetElementComponents() == 3)
				{
					if (attNormals.GetElementOpenGLType() == GL_BYTE || attNormals.GetElementOpenGLType() == GL_SHORT || attNormals.GetElementOpenGLType() == GL_INT || attNormals.GetElementOpenGLType() == GL_FLOAT || attNormals.GetElementOpenGLType() == GL_DOUBLE)
					{
						glNormalPointer(attNormals.GetElementOpenGLType(),0,attNormals.Get());
						glEnableClientState(GL_NORMAL_ARRAY);
						glEnable(GL_NORMALIZE);
					}
				}
			}
		}

		if (mesMesh.HasAttributes(TriangleMesh::ATTRIBUTETYPE_TEXTURECOORDINATE))
		{
			const AbstractAttributeArray & attTextureCoordinates = mesMesh.GetAttributes(TriangleMesh::ATTRIBUTETYPE_TEXTURECOORDINATE);

			if (!attTextureCoordinates.IsEmpty())
			{
				if (attTextureCoordinates.GetElementComponents() >= 1 && attTextureCoordinates.GetElementComponents() <= 4)
				{
					if (attTextureCoordinates.GetElementOpenGLType() == GL_SHORT || attTextureCoordinates.GetElementOpenGLType() == GL_INT || attTextureCoordinates.GetElementOpenGLType() == GL_FLOAT || attTextureCoordinates.GetElementOpenGLType() == GL_DOUBLE)
					{
						glTexCoordPointer(attTextureCoordinates.GetElementComponents(),attTextureCoordinates.GetElementOpenGLType(),0,attTextureCoordinates.Get());
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					}
				}
			}
		}

		if (mesMesh.HasAttributes(TriangleMesh::ATTRIBUTETYPE_COLOR))
		{
			const AbstractAttributeArray & attColors = mesMesh.GetAttributes(TriangleMesh::ATTRIBUTETYPE_COLOR);

			if (!attColors.IsEmpty())
			{
				if (attColors.GetElementComponents() >= 3 && attColors.GetElementComponents() <= 4)
				{
					if (attColors.GetElementOpenGLType() == GL_BYTE || attColors.GetElementOpenGLType() == GL_UNSIGNED_BYTE || attColors.GetElementOpenGLType() == GL_SHORT || attColors.GetElementOpenGLType() == GL_UNSIGNED_SHORT || attColors.GetElementOpenGLType() == GL_INT || attColors.GetElementOpenGLType() == GL_UNSIGNED_INT || attColors.GetElementOpenGLType() == GL_FLOAT || attColors.GetElementOpenGLType() == GL_DOUBLE)
					{
						glColorPointer(attColors.GetElementComponents(),attColors.GetElementOpenGLType(),0,attColors.Get());
						glEnableClientState(GL_COLOR_ARRAY);
					}
				}
			}
		}
	};

	virtual bool renderGroup(const TriangleMesh::Iterator & iteGroup, bool bTransparency, Shader *pShader = NULL)
	{
		const float  fOpacity = GetPlugin().GetProperty("Opacity",Variant::TypeFloat(1.0f, 0.0f, 1.0f));
		const Variant varMaterials = iteGroup.GetMesh().GetProperty("Materials",Variant::TypeMap());

		const TrianglePrimitiveArray & indices = iteGroup.GetGroupPrimitives();

		if (!indices.IsEmpty())
		{
			if (indices.GetElementOpenGLType() == GL_UNSIGNED_BYTE || indices.GetElementOpenGLType() == GL_UNSIGNED_SHORT || indices.GetElementOpenGLType() == GL_UNSIGNED_INT)
			{
				const std::string & strMaterial = iteGroup.GetGroupMaterial();

				Color colAmbient(0.2f,0.2f,0.2f,1.0f);
				Color colDiffuse(0.8f,0.8f,0.8f,1.0f);
				Color colSpecular(0.0f,0.0f,0.0f,1.0f);
				Color colEmission(0.0f,0.0f,0.0f,0.0f);
				float fShininess = 0.0f;

				bool bAmbient = false;
				bool bDiffuse = false;
				bool bSpecular = false;
				bool bEmission = false;
				bool bShininess = false;

				ImageResource *pDiffuseTexture = NULL;				
				float fAlpha = fOpacity;

				if (varMaterials.HasElement(strMaterial))
				{
					const Variant & varCurrentMaterial = varMaterials[strMaterial];

					if (varCurrentMaterial.HasElement("Ambient Color"))
					{
						colAmbient = varCurrentMaterial["Ambient Color"];
						bAmbient = true;
					}

					if (varCurrentMaterial.HasElement("Diffuse Color"))
					{
						colDiffuse = varCurrentMaterial["Diffuse Color"];
						bDiffuse = true;
					}

					if (varCurrentMaterial.HasElement("Specular Color"))
					{
						colSpecular = varCurrentMaterial["Specular Color"];
						bSpecular = true;
					}

					if (varCurrentMaterial.HasElement("Emission Color"))
					{
						colEmission = varCurrentMaterial["Emission Color"];
						bEmission = true;
					}

					if (varCurrentMaterial.HasElement("Shininess"))
					{
						fShininess = varCurrentMaterial["Shininess"];
						bShininess = true;
					}

					if (varCurrentMaterial.HasElement("Diffuse Texture"))
					{
						Handle hanTexture = varCurrentMaterial["Diffuse Texture"];

						if (hanTexture.IsValid())
							pDiffuseTexture = hanTexture.GetResource<ImageResource>();
					}
				}

				//fAlpha = std::min(fAlpha,colAmbient.GetNormalizedAlpha());
				fAlpha = std::min(fAlpha,colDiffuse.GetNormalizedAlpha());
				//fAlpha = std::min(fAlpha,colSpecular.GetNormalizedAlpha());
				//fAlpha = std::min(fAlpha,colEmission.GetNormalizedAlpha());

				if (fAlpha > 0.0f)
				{
					if (!bTransparency && fAlpha < 1.0 )
						return false;

					glPushAttrib(GL_LIGHTING_BIT);
					bool bMaterial = false;

					if (bAmbient)
					{
						const float vfAmbient[] = { colAmbient.GetNormalizedRed(), colAmbient.GetNormalizedGreen(), colAmbient.GetNormalizedBlue(), colAmbient.GetNormalizedAlpha() };
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, vfAmbient);
						bMaterial = true;
					}

					if  (bDiffuse)
					{
						const float vfDiffuse[] = { colDiffuse.GetNormalizedRed(), colDiffuse.GetNormalizedGreen(), colDiffuse.GetNormalizedBlue(), colDiffuse.GetNormalizedAlpha() };
						glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, vfDiffuse);
						bMaterial = true;
					}

					if (bSpecular)
					{
						const float vfSpecular[] = { colSpecular.GetNormalizedRed(), colSpecular.GetNormalizedGreen(), colSpecular.GetNormalizedBlue(), colSpecular.GetNormalizedAlpha() };
						glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, vfSpecular);
						bMaterial = true;
					}

					if (bEmission)
					{
						const float vfEmission[] = { colEmission.GetNormalizedRed(), colEmission.GetNormalizedGreen(), colEmission.GetNormalizedBlue(), colEmission.GetNormalizedAlpha() };
						glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, vfEmission);
						bMaterial = true;
					}

					if (bShininess)
					{
						glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, fShininess);
						bMaterial = true;
					}

					int iDiffuseTextureUnit = -1;

					if (pDiffuseTexture)
					{
						glActiveTexture(GL_TEXTURE0);
						glEnable(GL_TEXTURE_2D);
						ResourceTexture::Get<ImageTexture2D>(pDiffuseTexture).bind();
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

						if (pShader)
						{
							const int iUniformLocation_bDiffuseTexture = pShader->GetUniformLocation("bDiffuseTexture");

							if (iUniformLocation_bDiffuseTexture != -1)
								glUniform1iARB(iUniformLocation_bDiffuseTexture,true);


							const int iUniformLocation_samDiffuseTexture = pShader->GetUniformLocation("samDiffuseTexture");

							if (iUniformLocation_samDiffuseTexture != -1)
								glUniform1iARB(iUniformLocation_samDiffuseTexture,0);
						}

						iDiffuseTextureUnit = GL_TEXTURE0;
					}
					else
					{
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D,0);
						glDisable(GL_TEXTURE_2D);

						if (pShader)
						{
							const int iUniformLocation_bDiffuseTexture = pShader->GetUniformLocation("bDiffuseTexture");

							if (iUniformLocation_bDiffuseTexture != -1)
								glUniform1iARB(iUniformLocation_bDiffuseTexture,false);


							const int iUniformLocation_samDiffuseTexture = pShader->GetUniformLocation("samDiffuseTexture");

							if (iUniformLocation_samDiffuseTexture != -1)
								glUniform1iARB(iUniformLocation_samDiffuseTexture,0);
						}

						iDiffuseTextureUnit = -1;
					}

					if (bMaterial)
						glDisable(GL_COLOR_MATERIAL);
					else
					{
						glColorMaterial(GL_FRONT_AND_BACK,GL_DIFFUSE);
						glEnable(GL_COLOR_MATERIAL);
					}

					//glColor4f(1.0, 1.0, 0.0, 1.0);
					glDrawElements(GL_TRIANGLES,indices.GetArrayVertices(),indices.GetElementOpenGLType(),indices.Get());

					if (iDiffuseTextureUnit != -1)
					{
						glActiveTexture(iDiffuseTextureUnit);
						ResourceTexture::Get<ImageTexture2D>(pDiffuseTexture).release();
					}

					glPopAttrib();
				}
			}
		}

		return true;
	};

	virtual void renderMesh(const TriangleMesh & mesMesh, bool selected, bool overlay, Shader *pShader = NULL)
	{
		const Matrix matProjectionTransformation = GetPlugin().GetProperty("Projection Transformation");
		const Matrix matViewingTransformation = GetPlugin().GetProperty("Viewing Transformation");
		const Matrix matMeshTransformation = mesMesh.GetProperty("Transformation",Matrix());


		const Matrix matTransformation = matProjectionTransformation * matViewingTransformation * matMeshTransformation;
		const std::string strSelection = GetPlugin().GetProperty("Selection");
		const Variant & varGroups  = GetPlugin().GetProperty("Groups");

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

		setupAttributes(mesMesh);	

		std::multimap<float,TriangleMesh::Iterator> mapRemaining;

		unsigned int uCount = 0;

		for (TriangleMesh::Iterator i(mesMesh);!i.IsAtEnd();++i)
		{
			TriangleMesh::Iterator iteGroup = *i;

			const std::string & strGroupName = iteGroup.GetGroupName();
			const std::string & strSelection = GetPlugin().GetProperty("Selection");
			const bool bEnabled = varGroups[strGroupName];
			if(overlay)
			{
				Color grColor ((int)(uCount/256/256), (uCount/256)%256, uCount%256);
				++uCount;
				glUniform3f(m_shaShader.GetUniformLocation("uGroupColor"), (float)grColor.GetRed()/255.0f, (float)grColor.GetGreen()/255.0f, (float)grColor.GetBlue()/255.0f);
			}

			if (bEnabled && !(selected ^ (strGroupName.compare(strSelection)==0)))
			{
				if (!renderGroup(iteGroup,false,pShader))
				{
					const Box & boxBounds = iteGroup.GetGroupBounds();
					const Vector vecMinimum = boxBounds.GetMinimum(matTransformation);
					mapRemaining.insert(std::make_pair(vecMinimum.GetZ(),iteGroup));
				}
			}

		}

		for (std::multimap<float,TriangleMesh::Iterator>::reverse_iterator j = mapRemaining.rbegin(); j != mapRemaining.rend(); j++)
		{
			const TriangleMesh::Iterator & iteGroup = j->second;
			renderGroup(iteGroup,true,pShader);
		}

		glPopClientAttrib();
		glPopAttrib();

	};

	virtual const std::string GetGroupName(const TriangleMesh & mesMesh, const Vector & vecPosition)
	{
		const int iX = int(0.5f*(vecPosition.GetX()+1.0f) * float(m_uWidth));
		const int iY = int(0.5f*(vecPosition.GetY()+1.0f) * float(m_uHeight));

		const Matrix matProjectionTransformation = GetPlugin().GetProperty("Projection Transformation");
		const Matrix matViewingTransformation = GetPlugin().GetProperty("Viewing Transformation");
		const Matrix matMeshTransformation = mesMesh.GetProperty("Transformation",Matrix());

		const Vector vecPlaneTranslation = GetPlugin().GetProperty("Plane Translation");
		const Vector vecPlaneRotationVector = GetPlugin().GetProperty("Plane Rotation Vector");
		const float vecPlaneRotationAngle = GetPlugin().GetProperty("Plane Rotation Angle");
		//const float offset = GetPlugin().GetProperty("Offset");

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(matProjectionTransformation.Get());

		glMatrixMode(GL_MODELVIEW);

		//rotate plane normal when plane rotates
		Vector planeNormal(0.0f, 0.0f, 1.0f);
		glLoadIdentity();
		glRotatef(vecPlaneRotationAngle, vecPlaneRotationVector.GetX(), vecPlaneRotationVector.GetY(), vecPlaneRotationVector.GetZ());
		float arr[16] = {0.0f};
		glGetFloatv(GL_MODELVIEW_MATRIX, arr);
		Matrix rotPlaneNormalMatrix(arr);
		planeNormal = rotPlaneNormalMatrix.GetRotated(planeNormal);
		planeNormal.normalize();

		//translate model with offset
		Vector modelTranslationA = planeNormal * current_offset[0];
		Vector modelTranslationB = planeNormal * current_offset[1];


		glLoadIdentity();

		glLoadMatrixf(matViewingTransformation.Get());
		glMultMatrixf(matMeshTransformation.Get());

		glEnable(GL_SCISSOR_TEST);
		glScissor(iX,iY,1,1);

		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

		glEnable(GL_DEPTH_TEST);
		glClearDepth(1.0);
		glDepthFunc(GL_LESS);

		glDisable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		TriangleMesh::Iterator i(mesMesh);
		std::vector<std::string> vecGroups;
		unsigned int uCount = 0;
		Variant & varVariant = GetPlugin().GetProperty("Groups");

		for (;!i.IsAtEnd();++i)
		{
			const std::string & strGroupName = (*i).GetGroupName();
			const bool bEnabled = varVariant[strGroupName];
			if(bEnabled)
				vecGroups.push_back(strGroupName);
		}

		//DRAW MODEL

		// Selected part(s)
		m_shaShader.SetOption("overlay","true");
		m_shaShader.SetOption("split","false");
		m_shaShader.bind();
		renderMesh(mesMesh, true, true);
		m_shaShader.release();

		// first half of model
		glPushMatrix();
		glTranslatef(modelTranslationA.GetX(), modelTranslationA.GetY(), modelTranslationA.GetZ());
		m_shaShader.SetOption("overlay","true");
		m_shaShader.SetOption("split","true");
		m_shaShader.bind();
		glUniform3f(m_shaShader.GetUniformLocation("uNormal"), planeNormal.GetX(), planeNormal.GetY(), planeNormal.GetZ());
		glUniform3f(m_shaShader.GetUniformLocation("uPlanePoint"), 0.0f + vecPlaneTranslation.GetX(), 0.0f + vecPlaneTranslation.GetY(), 0.0f + vecPlaneTranslation.GetZ());
		renderMesh(mesMesh, false, true);
		m_shaShader.release();

		// second half of model
		glPopMatrix();
		glTranslatef(modelTranslationB.GetX(), modelTranslationB.GetY(), modelTranslationB.GetZ());
		m_shaShader.SetOption("overlay","true");
		m_shaShader.SetOption("split","true");
		m_shaShader.bind();
		glUniform3f(m_shaShader.GetUniformLocation("uNormal"), -planeNormal.GetX(), -planeNormal.GetY(), -planeNormal.GetZ());
		glUniform3f(m_shaShader.GetUniformLocation("uPlanePoint"), 0.0f + vecPlaneTranslation.GetX(), 0.0f + vecPlaneTranslation.GetY(), 0.0f + vecPlaneTranslation.GetZ());
		renderMesh(mesMesh, false, true);
		m_shaShader.release();

		//DRAW MODEL END

		float vuResult[4];
		std::string strGroupName;

		glReadPixels(iX, iY, 1, 1, GL_RGBA, GL_FLOAT, vuResult);

		if (vuResult[3] > 0.0)
		{
			const unsigned int uResult = (unsigned int)((vuResult[0]*256*256 + vuResult[1]*256 + vuResult[2])*256);

			if (uResult < vecGroups.size())
				strGroupName = vecGroups[uResult];
		}


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glPopClientAttrib();
		//glPopAttrib();

		//GetOffset(mesMesh);

		return strGroupName;
	};

	virtual const void GetOffset(TriangleMesh &mesh)
	{

		bool dynamic=GetPlugin().GetProperty("DynamicOffset");
		float speed;

		if(!dynamic){
			ideal_offset[0]=GetPlugin().GetProperty("Offset");
			ideal_offset[1]=-((float)GetPlugin().GetProperty("Offset"));
			speed=GetPlugin().GetProperty("Speed");
		}else{

			//	/*const Matrix matViewingTransformation = GetPlugin().GetProperty("Viewing Transformation");
			//	const Matrix matMeshTransformation = mesh.GetProperty("Transformation",Matrix());

			//	const Matrix modelView=matViewingTransformation*matMeshTransformation;

			//	Box selectbounds=Box();

			//	for (TriangleMesh::Iterator i(mesh);!i.IsAtEnd();++i)
			//	{
			//		TriangleMesh::Iterator iteGroup = *i;

			//		const std::string & strGroupName = iteGroup.GetGroupName();
			//		const std::string & strSelection = GetPlugin().GetProperty("Selection");

			//		if (strGroupName.compare(strSelection)==0){
			//			selectbounds+=iteGroup.GetGroupBounds();
			//		}
			//	}
			//	selectbounds.transform(modelView);

			//	Vector xMin, xMax;

			//	xMin=selectbounds.GetCorner(0,0,0);
			//	xMax=xMin;

			//	for(int i=0;i<2;i++){
			//		for(int j=0;j<2;j++){
			//			for(int k=0;k<2;k++){
			//				Vector corner=selectbounds.GetCorner(i,j,k);
			//				if(corner.GetX() < xMin.GetX())
			//					xMin=corner;
			//				if(corner.GetX() > xMin.GetX())
			//					xMax=corner;
			//			}
			//		}
			//	}
			//
			//	Vector viewpoint=-(modelView*Vector(0.0f,0.0f,0.0f));
			//	Vector tomin= xMin-viewpoint;
			//	Vector planeNormal=Vector(0.0f,1.0f,0.0f).GetCross(Vector(tomin.GetX(),tomin.GetY(),tomin.GetZ()));*/

			float ratio = occlusions[0]- occlusions[1];
		
			if (ratio>0){
				ratio/=occlusions[0];
				ideal_offset[0]=15.0f;
				ideal_offset[1]=-15.0f;
				speed=GetPlugin().GetProperty("Speed");
				speed*=ratio;
			}else{
				if(ideal_offset[0]==15.0f){
					speed=GetPlugin().GetProperty("Speed");
					ideal_offset[0]=current_offset[0];
					ideal_offset[1]=current_offset[1];
					return;
				}else{
					if(ideal_offset[0]==current_offset[0]&&ideal_offset[1]==current_offset[1])
						return;

					speed=GetPlugin().GetProperty("Speed");
					ideal_offset[0]=0.0f;
					ideal_offset[1]=0.0f;
				}

			}
		}


		if(ideal_offset[0]==current_offset[0]&&ideal_offset[1]==current_offset[1])
			return;

		
		for(int i=0;i<2;i++){
			float distance=ideal_offset[i]-current_offset[i];
			float d_offset=abMax(distance*speed/50.0f,0.0000f);

			if(this->abs(distance)<0.001f)
				current_offset[i]= ideal_offset[i];

			current_offset[i]+=d_offset;
		}
	};

	float abs(float x){
		if(x<0.0f)
			return -x;
		return x;
	};

	float abMax(float x, float y){
		if(x>0){
			if(y>x)
				return y;
			return x;
		}else
		{
			if(-y<x)
				return -y;
			return x;
		}
	};

private:

	unsigned int m_uWidth;
	unsigned int m_uHeight;

	bool m_bUpdateGroups;
	ModifiedObserver m_modVariantObserver;
	ModifiedObserver m_modMeshObserver;
	Shader m_shaShader;

	Framebuffer m_fraFramebuffer;
	Texture2D m_texColorTexture0;
	Texture2D m_texColorTexture1;
	Texture2D m_texColorTexture2;
	Texture2D m_texColorTexture3;
	Texture2D m_texColorTexture4;
	Texture2D m_texColorTexture5;
	Texture2D m_texDepthTexture;

	Vector m_vecPickingPosition;
	bool m_bPicking;

	bool m_bMouseDownInWindow;
	Vector m_kLastMouseDownPosition;
	float ideal_offset[2];
	float current_offset[2];
	GLuint occlusions[2];

};
