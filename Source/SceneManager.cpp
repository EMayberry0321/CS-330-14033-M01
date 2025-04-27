///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/plastic_dark_seamless.jpg", "plastic");

	bReturn = CreateGLTexture(
		"textures/wood_knots_seamlessr.jpg", "wood");

	bReturn = CreateGLTexture(
		"textures/greywall.jpg", "wall");

	bReturn = CreateGLTexture(
		"textures/rubber_circles_seamless.jpg", "pad");

	bReturn = CreateGLTexture(
		"textures/screen_wallpaper_2.jpg", "pad2");

	bReturn = CreateGLTexture(
		"textures/PCscreen.jpg", "screen");

	bReturn = CreateGLTexture(
		"textures/blackmetal.jpg", "metal");

	bReturn = CreateGLTexture(
		"textures/motherboard.jpg", "mb");

	bReturn = CreateGLTexture(
		"textures/Riolu.jpg", "PKMN");

	bReturn = CreateGLTexture(
		"textures/rainbowFade.jpg", "rgb");

	bReturn = CreateGLTexture(
		"textures/motherboardback.jpeg", "mbb");

	bReturn = CreateGLTexture(
		"textures/blue.jpg", "blue");

	bReturn = CreateGLTexture(
		"textures/pink.jpg", "pink");

	bReturn = CreateGLTexture(
		"textures/Keyboardtop.jpg", "keyboard");

	bReturn = CreateGLTexture(
		"textures/RAMside.jpg", "ram");

	bReturn = CreateGLTexture(
		"textures/blackplasticmaterial.jpg", "blackpl");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 25.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wallMaterial.ambientStrength = 0.2f;
	wallMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	wallMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wallMaterial.shininess = 0.0;
	wallMaterial.tag = "walls";

	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	grapeMaterial.ambientStrength = 0.1f;
	grapeMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.3f);
	grapeMaterial.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	grapeMaterial.shininess = 0.5;
	grapeMaterial.tag = "plastic";

	m_objectMaterials.push_back(grapeMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", 13.5f, 15.79f, 1.9f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.949f, 0.184f, 0.863f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.949f, 0.184f, 0.863f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 15.0f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -13.5f, 15.79f, 1.9f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.949f, 0.184f, 0.863f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.949f, 0.184f, 0.863f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 15.0f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 3.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderDeskandwalls();
	RenderKeyboardandmat();
	RenderMouse();
	RenderPCexterior();
	RenderPCinterior();
	RenderMonitor();
	RenderGlass();
}

/***********************************************************
 *  RenderDeskandwalls()
 *
 *  This method is called to render the shapes for the desk
 *  and wall objects.
 ***********************************************************/
void SceneManager::RenderDeskandwalls()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.549, 0.549, 0.549, 1);
	SetShaderTexture("wall");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("walls");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 10.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.549, 0.549, 0.549, 1);
	SetShaderTexture("wall");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("walls");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.658, 0.616, 0.439, 1);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}


/***********************************************************
 *  RenderKeyboardandmat()
 *
 *  This method is called to render the shapes for the mat
 *  and keyboard objects.
 ***********************************************************/
void SceneManager::RenderKeyboardandmat()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(31.0f, 0.1f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.1f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("pad");
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("pad2");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 0.5f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 0.3f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("keyboard");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 14.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 0.3f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.01f, 0.49f, 5.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 0.3f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}

/***********************************************************
 *  RenderMouse()
 *
 *  This method is called to render the shapes for the mouse
 *  object.
 ***********************************************************/
void SceneManager::RenderMouse()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.3f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.2f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("blackpl");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.5f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("blackpl");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.67f, 3.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.95f, 4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.15f, 0.7f, 4.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.14f, 0.7f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.1f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.949, 0.184, 0.863, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

/***********************************************************
 *  RenderMonitor()
 *
 *  This method is called to render the shapes for the monitor
 *  object.
 ***********************************************************/
void SceneManager::RenderMonitor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.0f, 0.5f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.3f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.15f, 6.3f, 1.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 3.3f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.85f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 5.95f, -2.85f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(12.0f, 0.1f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 5.95f, -2.223f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("screen");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(12.25f, 0.25f, 6.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 5.95f, -2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
	
/***********************************************************
 *  RenderPCexterior()
 *
 *  This method is called to render the shapes for the PC
 *  exterior objects.
 ***********************************************************/
void SceneManager::RenderPCexterior()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//PC FOOT FL ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.63f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.1f, 8.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//PC FOOT BL ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.63f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.0f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//PC FOOT FR ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.63f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 0.0f, 8.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//PC FOOT BR ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.63f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 0.0f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//PC BOTTOM ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.3f, 14.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 0.71f, 1.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC TOP ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.3f, 14.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 10.79f, 1.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC BACK ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.3f, 9.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 5.75f, -5.349f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC SIDE 1 ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.5f, 0.3f, 9.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.35f, 5.75f, 2.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC MOTHERBOARD ***************
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.25f, 0.25f, 7.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.3f, 6.3f, -1.55f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("mb");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC FRONT ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.404f, 0.3f, 9.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 5.75f, 8.649f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderPCinterior()
 *
 *  This method is called to render the shapes for the PC
 *  interior objects.
 ***********************************************************/
void SceneManager::RenderPCinterior()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// CPU Cooler ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 1.3f, 1.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.3f, 7.4f, -1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// CPU COOLER SCREEN ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.96f, 7.4f, -1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("PKMN");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//PC COOLER COVER ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.1f, 7.4f, -1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	//PC RAM STICK L ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.8f, 7.49f, 0.03f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("ram");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC RAM STICK R ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.8f, 7.49f, 0.58f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("ram");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC RAM STICK RGB R ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 4.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.2f, 7.49f, 0.58f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("rgb");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC RAM STICK RGB L ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 4.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.2f, 7.49f, 0.03f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("rgb");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC MOTHERBOARD BACKPLATE ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 4.6f, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.8f, 7.65f, -4.58f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("mbb");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC MOTHERBOARD BACKPLATE P2 ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 4.7f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.86f, 7.65f, -4.58f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//PC FRONT FAN M ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 1.2f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 5.75f, 8.649f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("pink");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	//PC FRONT FAN T ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 1.2f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 9.0f, 8.649f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("blue");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	//PC FRONT FAN B ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 1.2f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 2.5f, 8.649f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("blue");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//PC PSU BLOCK ***************
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 2.0f, 12.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.8f, 1.7f, 0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.071, 0.071, 0.071, 1);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	}

	/***********************************************************
 *  RenderGlass()
 *
 *  This method is called to render the shapes for the PC
 *  glass objects.
 ***********************************************************/
	void SceneManager::RenderGlass()
	{
		// declare the variables for the transformations
		glm::vec3 scaleXYZ;
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ;

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.5f, 0.3f, 9.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.65f, 5.75f, 2.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.404f, 0.05f, 9.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 5.75f, 9.289f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
